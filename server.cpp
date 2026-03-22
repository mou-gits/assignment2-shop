#include "crow/crow.h"
#include "crow/json.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>

using namespace std;

// --------------------
// Global cart storage
// productId -> quantity
// --------------------
unordered_map<int, int> cart;


string getCartClass()
{
    if (cart.empty())
        return "";
    return "cart-active";
}

// --------------------
// Helper: read a file
// --------------------
string readFile(const string& path)
{
    ifstream file(path, ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// --------------------
// Helper: replace placeholders in html
// --------------------
void replacePlaceholder(string& html, const string& key, const string& value)
{
    size_t pos = 0;
    while ((pos = html.find(key, pos)) != string::npos)
    {
        html.replace(pos, key.length(), value);
        pos += value.length();
    }
}

// --------------------
// Helper: format prices to 2 decimals
// --------------------
string formatPrice(double value)
{
    ostringstream out;
    out << fixed << setprecision(2) << value;
    return out.str();
}

// --------------------
// Helper: load product catalog json
// --------------------
crow::json::rvalue loadProductsJson()
{
    string jsonText = readFile("data/product.json");
    if (jsonText.empty())
    {
        return crow::json::load("[]");
    }

    return crow::json::load(jsonText);
}

// --------------------
// Helper: check whether product id exists
// --------------------
bool productExists(int productId)
{
    auto products = loadProductsJson();
    if (!products)
    {
        return false;
    }

    for (int i = 0; i < static_cast<int>(products.size()); i++)
    {
        if (products[i].has("id") && products[i]["id"].i() == productId)
        {
            return true;
        }
    }

    return false;
}

// --------------------
// Helper: save cart to file
// Format:
// productId,quantity
// --------------------
void saveCartToFile()
{
    ofstream file("data/order.txt");
    if (!file.is_open())
    {
        return;
    }

    for (const auto& item : cart)
    {
        file << item.first << "," << item.second << "\n";
    }
}

// --------------------
// Helper: load cart from file
// --------------------
void loadCartFromFile()
{
    cart.clear();

    ifstream file("data/order.txt");
    if (!file.is_open())
    {
        return;
    }

    string line;
    while (getline(file, line))
    {
        if (line.empty())
        {
            continue;
        }

        stringstream ss(line);
        string idStr;
        string qtyStr;

        if (getline(ss, idStr, ',') && getline(ss, qtyStr))
        {
            try
            {
                int productId = stoi(idStr);
                int quantity = stoi(qtyStr);

                if (quantity > 0 && productExists(productId))
                {
                    cart[productId] = quantity;
                }
            }
            catch (...)
            {
                // ignore bad lines
            }
        }
    }
}

// --------------------
// Helper: extract quantity from POST body
// body example: quantity=2
// --------------------
int getPostedQuantity(const string& body)
{
    int quantity = 1;

    size_t pos = body.find("quantity=");
    if (pos != string::npos)
    {
        string qtyStr = body.substr(pos + 9);

        // trim off anything after &
        size_t ampPos = qtyStr.find('&');
        if (ampPos != string::npos)
        {
            qtyStr = qtyStr.substr(0, ampPos);
        }

        try
        {
            quantity = stoi(qtyStr);
        }
        catch (...)
        {
            quantity = 1;
        }
    }

    return quantity;
}

int main()
{
    crow::SimpleApp app;

    loadCartFromFile();

    // --------------------
    // Home page
    // --------------------
    CROW_ROUTE(app, "/")([]() {
        string html = readFile("templates/index.html");
        
        if (html.empty())
        {
            crow::response res(404);
            res.set_header("Content-Type", "text/plain");
            res.write("index.html not found");
            return res;
        }

        replacePlaceholder(html, "{{CART_CLASS}}", getCartClass());

        crow::response res(200);
        res.set_header("Content-Type", "text/html");
        res.write(html);
        return res;
    });

    // --------------------
    // CSS files
    // --------------------
    CROW_ROUTE(app, "/css/<string>")([](const string& filename) {
        string path = "static/css/" + filename;
        string content = readFile(path);

        if (content.empty())
        {
            return crow::response(404, "CSS file not found");
        }

        crow::response res(200);
        res.set_header("Content-Type", "text/css");
        res.write(content);
        return res;
    });

    // --------------------
    // JavaScript files
    // --------------------
    CROW_ROUTE(app, "/js/<string>")([](const string& filename) {
        string path = "static/js/" + filename;
        string content = readFile(path);

        if (content.empty())
        {
            return crow::response(404, "JS file not found");
        }

        crow::response res(200);
        res.set_header("Content-Type", "application/javascript");
        res.write(content);
        return res;
    });

    // --------------------
    // Image files
    // --------------------
    CROW_ROUTE(app, "/img/<string>")([](const string& filename) {
        string path = "static/img/" + filename;
        string content = readFile(path);

        if (content.empty())
        {
            return crow::response(404, "Image file not found");
        }

        crow::response res(200);

        if (filename.size() >= 4 && filename.substr(filename.size() - 4) == ".png")
        {
            res.set_header("Content-Type", "image/png");
        }
        else if (
            (filename.size() >= 4 && filename.substr(filename.size() - 4) == ".jpg") ||
            (filename.size() >= 5 && filename.substr(filename.size() - 5) == ".jpeg"))
        {
            res.set_header("Content-Type", "image/jpeg");
        }
        else if (filename.size() >= 4 && filename.substr(filename.size() - 4) == ".gif")
        {
            res.set_header("Content-Type", "image/gif");
        }
        else
        {
            res.set_header("Content-Type", "application/octet-stream");
        }

        res.body = content;
        return res;
    });

    // --------------------
    // Product page
    // --------------------
    CROW_ROUTE(app, "/product/<int>")([](int productId) {
        string html = readFile("templates/product.html");
        if (html.empty())
        {
            return crow::response(404, "product.html not found");
        }
        replacePlaceholder(html, "{{CART_CLASS}}", getCartClass());

        auto products = loadProductsJson();
        if (!products)
        {
            return crow::response(500, "Invalid JSON format");
        }

        int totalProducts = static_cast<int>(products.size());

        for (int i = 0; i < totalProducts; i++)
        {
            const auto& product = products[i];

            if (product.has("id") && product["id"].i() == productId)
            {
                string name = product.has("name") ? string(product["name"].s()) : "No Name";
                string description = product.has("description") ? string(product["description"].s()) : "No Description";
                string image = product.has("image") ? string(product["image"].s()) : "placeholder.png";

                double price = 0.0;
                if (product.has("price"))
                {
                    price = product["price"].d();
                }

                // image in JSON may be "mouse.png" or "static/img/mouse.png"
                string imageFile = image;
                size_t slashPos = image.find_last_of("/\\");
                if (slashPos != string::npos)
                {
                    imageFile = image.substr(slashPos + 1);
                }

                string prevButton;
                string nextButton;

                if (i > 0)
                {
                    int prevId = products[i - 1]["id"].i();
                    prevButton = "<a href=\"/product/" + to_string(prevId) + "\" class=\"nav-btn\">&larr; Previous Product</a>";
                }
                else
                {
                    prevButton = "<span class=\"nav-btn disabled\">&larr; Previous Product</span>";
                }

                if (i < totalProducts - 1)
                {
                    int nextId = products[i + 1]["id"].i();
                    nextButton = "<a href=\"/product/" + to_string(nextId) + "\" class=\"nav-btn\">Next Product &rarr;</a>";
                }
                else
                {
                    nextButton = "<span class=\"nav-btn disabled\">Next Product &rarr;</span>";
                }

                replacePlaceholder(html, "{{ID}}", to_string(productId));
                replacePlaceholder(html, "{{NAME}}", name);
                replacePlaceholder(html, "{{DESCRIPTION}}", description);
                replacePlaceholder(html, "{{PRICE}}", formatPrice(price));
                replacePlaceholder(html, "{{IMAGE}}", imageFile);
                replacePlaceholder(html, "{{PREV_BUTTON}}", prevButton);
                replacePlaceholder(html, "{{NEXT_BUTTON}}", nextButton);

                crow::response res(200);
                res.set_header("Content-Type", "text/html");
                res.write(html);
                return res;
            }
        }

        return crow::response(404, "Product not found");
    });

    // --------------------
    // Add to cart
    // --------------------
    CROW_ROUTE(app, "/add-to-cart/<int>").methods("POST"_method)
    ([](const crow::request& req, int productId) {
        if (!productExists(productId))
        {
            return crow::response(404, "Product not found");
        }

        int quantity = getPostedQuantity(req.body);
        if (quantity < 1)
        {
            quantity = 1;
        }

        cart[productId] += quantity;
        saveCartToFile();

        crow::response res(302);
        res.set_header("Location", "/cart");
        return res;
    });

    // --------------------
    // Cart page
    // --------------------
    CROW_ROUTE(app, "/cart")([]() {
        string html = readFile("templates/cart.html");

        if (html.empty())
        {
            return crow::response(404, "cart.html not found");
        }

        replacePlaceholder(html, "{{CART_CLASS}}", getCartClass());

        auto products = loadProductsJson();
        if (!products)
        {
            return crow::response(500, "Invalid JSON format");
        }

        string cartRows;
        double subtotal = 0.0;

        if (cart.empty())
        {
            cartRows = "<p>Your cart is empty.</p>";
        }
        else
        {
            cartRows += "<table class='cart-table'>";
            cartRows += "<tr><th>Product Name</th><th>Description</th><th>Quantity</th><th>Price</th><th>Action</th></tr>";

            for (const auto& item : cart)
            {
                int productId = item.first;
                int quantity = item.second;

                for (int i = 0; i < static_cast<int>(products.size()); i++)
                {
                    const auto& product = products[i];

                    if (product.has("id") && product["id"].i() == productId)
                    {
                        string name = product.has("name") ? string(product["name"].s()) : "No Name";
                        string description = product.has("description") ? string(product["description"].s()) : "No Description";

                        double price = 0.0;
                        if (product.has("price"))
                        {
                            price = product["price"].d();
                        }

                        double lineTotal = price * quantity;
                        subtotal += lineTotal;

                        cartRows += "<tr>";
                        cartRows += "<td>" + name + "</td>";
                        cartRows += "<td>" + description + "</td>";

                        cartRows += "<td>";
                        cartRows += "<form action='/update-cart/" + to_string(productId) + "' method='post' class='qty-form'>";
                        cartRows += "<input type='number' name='quantity' min='0' value='" + to_string(quantity) + "'>";
                        cartRows += "<button type='submit'>Update</button>";
                        cartRows += "</form>";
                        cartRows += "</td>";

                        cartRows += "<td>$ " + formatPrice(lineTotal) + "</td>";

                        cartRows += "<td>";
                        cartRows += "<form action='/remove-from-cart/" + to_string(productId) + "' method='post'>";
                        cartRows += "<button type='submit'>Remove</button>";
                        cartRows += "</form>";
                        cartRows += "</td>";

                        cartRows += "</tr>";
                        break;
                    }
                }
            }

            cartRows += "</table>";
        }

        double hst = subtotal * 0.13;
        double total = subtotal + hst;

        replacePlaceholder(html, "{{CART_ROWS}}", cartRows);
        replacePlaceholder(html, "{{SUBTOTAL}}", formatPrice(subtotal));
        replacePlaceholder(html, "{{HST}}", formatPrice(hst));
        replacePlaceholder(html, "{{TOTAL}}", formatPrice(total));

        crow::response res(200);
        res.set_header("Content-Type", "text/html");
        res.write(html);
        return res;
    });

    // --------------------
    // Update cart quantity
    // --------------------
    CROW_ROUTE(app, "/update-cart/<int>").methods("POST"_method)
    ([](const crow::request& req, int productId) {
        if (!productExists(productId))
        {
            return crow::response(404, "Product not found");
        }

        int quantity = getPostedQuantity(req.body);

        if (quantity <= 0)
        {
            cart.erase(productId);
        }
        else
        {
            cart[productId] = quantity;
        }

        saveCartToFile();

        crow::response res(302);
        res.set_header("Location", "/cart");
        return res;
    });

    // --------------------
    // Remove from cart
    // --------------------
    CROW_ROUTE(app, "/remove-from-cart/<int>").methods("POST"_method)
    ([](int productId) {
        cart.erase(productId);
        saveCartToFile();

        crow::response res(302);
        res.set_header("Location", "/cart");
        return res;
    });

    app.port(8080).multithreaded().run();
    return 0;
}