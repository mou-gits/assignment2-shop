FROM ubuntu:22.04

RUN apt-get update && apt-get install -y g++ make libasio-dev

WORKDIR /app

COPY . .

RUN g++ -std=c++17 server.cpp -Iinclude -o server -pthread

EXPOSE 8080

CMD ["./server"]
