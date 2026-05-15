#pragma once
#include "GameStructures.h"
#include <iostream>
#include <vector>
#include <chrono>

class ConsoleEngine {
private:
    HANDLE hConsole;
    HANDLE hInput;
    COORD screenSize;
    CHAR_INFO* screenBuffer;
    CHAR_INFO* prevScreenBuffer;  // For dirty checking
    SMALL_RECT writeRegion;
    bool mouseEnabled;
    
public:
    ConsoleEngine(int width, int height);
    ~ConsoleEngine();
    
    // Khởi tạo và cleanup
    bool Initialize();
    void Cleanup();
    
    // Vẽ và hiển thị
    void ClearScreen();
    void SetPixel(int x, int y, char character, Color foreground, Color background = Color::BLACK);
    void DrawString(int x, int y, const std::string& text, Color color = Color::WHITE);
    void DrawLine(Point2D start, Point2D end, char character, Color color);
    void DrawCircle(Point2D center, float radius, char character, Color color);
    void DrawRect(int x, int y, int width, int height, char character, Color color);
    void FillRect(int x, int y, int width, int height, char character, Color color);
    void Present();
    
    // Input handling
    bool IsKeyPressed(int virtualKey);
    bool IsKeyDown(int virtualKey);
    bool IsSpacePressed(); // Special handling for space key
    COORD GetMousePosition();
    bool IsMouseButtonPressed(int button); // 0 = left, 1 = right
    
    // Utility
    void SetCursorVisible(bool visible);
    void SetTitle(const std::string& title);
    void PlayBeep(int frequency, int duration);
    
    // Animation helpers
    void DrawProgressBar(int x, int y, int width, float percentage, Color fillColor, Color bgColor);
    void DrawHealthBar(int x, int y, int width, float health, float maxHealth);
    
    // Screen dimensions
    int GetWidth() const { return screenSize.X; }
    int GetHeight() const { return screenSize.Y; }
    
    // Color utilities
    WORD GetColorAttribute(Color foreground, Color background = Color::BLACK);
    
    // Frame timing
    void LimitFPS(int targetFPS);
    float GetDeltaTime();
    
private:
    std::chrono::high_resolution_clock::time_point lastFrameTime;
    float deltaTime;
    
    // Input state tracking
    bool keyStates[256];
    bool prevKeyStates[256];
    COORD mousePos;
    bool mouseButtons[2];
    bool prevMouseButtons[2];
    
    void UpdateInputStates();
    bool IsValidPosition(int x, int y) const;
};

// Inline implementations for simple functions
inline bool ConsoleEngine::IsValidPosition(int x, int y) const {
    return x >= 0 && x < screenSize.X && y >= 0 && y < screenSize.Y;
}

inline WORD ConsoleEngine::GetColorAttribute(Color foreground, Color background) {
    return (static_cast<WORD>(background) << 4) | static_cast<WORD>(foreground);
}
