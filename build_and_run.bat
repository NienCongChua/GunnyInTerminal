@echo off
echo ========================================
echo    Building Gunny Game
echo ========================================

REM Clean previous build
if exist gunny.exe del gunny.exe
if exist *.o del *.o

REM Build the game
echo Compiling...
g++ -std=c++17 -Wall -Wextra -O2 main.cpp ConsoleEngine.cpp Terrain.cpp GameEngine.cpp -o gunny.exe

REM Check if build was successful
if exist gunny.exe (
    echo.
    echo ========================================
    echo    Build Successful!
    echo ========================================
    echo.
    echo Starting Gunny Game...
    echo.
    echo Game Controls:
    echo - Menu: Arrow keys to navigate, Enter to select
    echo - Game: A/D for angle, W/S for power, Space to fire
    echo - Mouse: Click to aim, Right-click to fire
    echo - Q/E to change weapons, P to pause
    echo.
    pause
    
    REM Run the game
    gunny.exe
    
    echo.
    echo Game ended. Press any key to exit...
    pause >nul
) else (
    echo.
    echo ========================================
    echo    Build Failed!
    echo ========================================
    echo.
    echo Please check the error messages above.
    echo Make sure you have MinGW-w64 installed and in your PATH.
    echo.
    pause
)
