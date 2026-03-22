@echo off
setlocal

echo ============================================
echo Docker setup for Assignment 2
echo ============================================

REM --------------------------------
REM Check required files/folders
REM --------------------------------
if not exist include (
    echo ERROR: include folder not found.
    pause
    exit /b 1
)

if not exist include\crow\crow.h (
    echo ERROR: include\crow\crow.h not found.
    echo.
    echo Please download Crow from GitHub, extract it,
    echo and copy the entire include\crow folder into your project.
    echo.
    pause
    exit /b 1
)

if not exist server.cpp (
    echo ERROR: server.cpp not found.
    pause
    exit /b 1
)

REM --------------------------------
REM Check Docker availability
REM --------------------------------
docker version >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Docker is not running or not installed correctly.
    echo Open Docker Desktop and wait until it is fully started.
    pause
    exit /b 1
)

REM --------------------------------
REM Create Dockerfile
REM --------------------------------
echo Creating Dockerfile...

(
echo FROM ubuntu:22.04
echo.
echo RUN apt-get update ^&^& apt-get install -y g++ make libasio-dev
echo.
echo WORKDIR /app
echo.
echo COPY . .
echo.
echo RUN g++ -std=c++17 server.cpp -Iinclude -o server -pthread
echo.
echo EXPOSE 8080
echo.
echo CMD ["./server"]
) > Dockerfile

if %errorlevel% neq 0 (
    echo ERROR: Failed to create Dockerfile.
    pause
    exit /b 1
)

echo Dockerfile created successfully.

REM --------------------------------
REM Build Docker image
REM --------------------------------
echo Building Docker image...
docker build -t assignment2-shop .

if %errorlevel% neq 0 (
    echo ERROR: Docker build failed.
    pause
    exit /b 1
)

echo Docker image built successfully.

REM --------------------------------
REM Run container
REM --------------------------------
echo Starting container on port 8080...
docker run -p 8080:8080 assignment2-shop

pause
endlocal