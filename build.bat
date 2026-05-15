@echo off
echo Building Gunny Game...
g++ -std=c++17 -Wall -Wextra -O2 main.cpp ConsoleEngine.cpp Terrain.cpp GameEngine.cpp -o gunny.exe
if exist gunny.exe (
    echo Build successful! Run gunny.exe to play.
) else (
    echo Build failed! Check error messages above.
)
pause
