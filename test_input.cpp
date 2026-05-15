#include <windows.h>
#include <iostream>
#include <conio.h>

int main() {
    std::cout << "Input Test - Press keys to test input detection\n";
    std::cout << "Press ESC to exit\n\n";
    
    while (true) {
        // Test GetAsyncKeyState
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            std::cout << "ESC pressed - exiting\n";
            break;
        }
        
        if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
            std::cout << "ENTER pressed\n";
            Sleep(200); // Prevent spam
        }
        
        if (GetAsyncKeyState(VK_UP) & 0x8000) {
            std::cout << "UP arrow pressed\n";
            Sleep(200);
        }
        
        if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
            std::cout << "DOWN arrow pressed\n";
            Sleep(200);
        }
        
        // Test _kbhit and _getch
        if (_kbhit()) {
            char ch = _getch();
            std::cout << "Key pressed: " << static_cast<int>(ch) << " ('" << ch << "')\n";
        }
        
        Sleep(50); // Small delay to prevent high CPU usage
    }
    
    return 0;
}
