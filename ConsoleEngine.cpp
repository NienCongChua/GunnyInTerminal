#include "ConsoleEngine.h"
#include <conio.h>
#include <algorithm>
#include <thread>
#include <cstring>
#include <iostream>

ConsoleEngine::ConsoleEngine(int width, int height)
    : screenSize({static_cast<SHORT>(width), static_cast<SHORT>(height)}),
      mouseEnabled(true), deltaTime(0.0f) {

    int bufferSize = width * height;
    screenBuffer = new CHAR_INFO[bufferSize];
    prevScreenBuffer = new CHAR_INFO[bufferSize];
    writeRegion = {0, 0, static_cast<SHORT>(width - 1), static_cast<SHORT>(height - 1)};

    // Initialize buffers
    memset(screenBuffer, 0, bufferSize * sizeof(CHAR_INFO));
    memset(prevScreenBuffer, 0, bufferSize * sizeof(CHAR_INFO));

    // Initialize input states
    memset(keyStates, 0, sizeof(keyStates));
    memset(prevKeyStates, 0, sizeof(prevKeyStates));
    memset(mouseButtons, 0, sizeof(mouseButtons));
    memset(prevMouseButtons, 0, sizeof(prevMouseButtons));

    lastFrameTime = std::chrono::high_resolution_clock::now();
}

ConsoleEngine::~ConsoleEngine() {
    Cleanup();
    delete[] screenBuffer;
    delete[] prevScreenBuffer;
}

bool ConsoleEngine::Initialize() {
    // Get console handles
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    hInput = GetStdHandle(STD_INPUT_HANDLE);

    if (hConsole == INVALID_HANDLE_VALUE || hInput == INVALID_HANDLE_VALUE) {
        return false;
    }

    // Set console mode for mouse input (optimized)
    DWORD mode;
    GetConsoleMode(hInput, &mode);
    mode |= ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS;
    mode &= ~ENABLE_QUICK_EDIT_MODE; // Disable quick edit to capture mouse
    SetConsoleMode(hInput, mode);

    // Set console screen buffer size first
    COORD bufferSize = screenSize;
    SetConsoleScreenBufferSize(hConsole, bufferSize);

    // Set console window size
    SMALL_RECT windowSize = {0, 0, static_cast<SHORT>(screenSize.X - 1), static_cast<SHORT>(screenSize.Y - 1)};
    SetConsoleWindowInfo(hConsole, TRUE, &windowSize);

    // Hide cursor and optimize console
    SetCursorVisible(false);

    // Set console to use faster output mode
    DWORD consoleMode;
    GetConsoleMode(hConsole, &consoleMode);
    consoleMode |= ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;
    SetConsoleMode(hConsole, consoleMode);

    // Clear screen initially
    ClearScreen();

    return true;
}

void ConsoleEngine::Cleanup() {
    SetCursorVisible(true);
    
    // Restore console mode
    DWORD mode;
    GetConsoleMode(hInput, &mode);
    mode |= ENABLE_QUICK_EDIT_MODE;
    SetConsoleMode(hInput, mode);
}

void ConsoleEngine::ClearScreen() {
    for (int i = 0; i < screenSize.X * screenSize.Y; ++i) {
        screenBuffer[i].Char.AsciiChar = ' ';
        screenBuffer[i].Attributes = GetColorAttribute(Color::WHITE, Color::BLACK);
    }
}

void ConsoleEngine::SetPixel(int x, int y, char character, Color foreground, Color background) {
    if (!IsValidPosition(x, y)) return;
    
    int index = y * screenSize.X + x;
    screenBuffer[index].Char.AsciiChar = character;
    screenBuffer[index].Attributes = GetColorAttribute(foreground, background);
}

void ConsoleEngine::DrawString(int x, int y, const std::string& text, Color color) {
    for (size_t i = 0; i < text.length() && x + static_cast<int>(i) < screenSize.X; ++i) {
        SetPixel(x + static_cast<int>(i), y, text[i], color);
    }
}

void ConsoleEngine::DrawLine(Point2D start, Point2D end, char character, Color color) {
    int x0 = static_cast<int>(start.x);
    int y0 = static_cast<int>(start.y);
    int x1 = static_cast<int>(end.x);
    int y1 = static_cast<int>(end.y);
    
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    
    while (true) {
        SetPixel(x0, y0, character, color);
        
        if (x0 == x1 && y0 == y1) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void ConsoleEngine::DrawCircle(Point2D center, float radius, char character, Color color) {
    int cx = static_cast<int>(center.x);
    int cy = static_cast<int>(center.y);
    int r = static_cast<int>(radius);
    
    for (int angle = 0; angle < 360; angle += 5) {
        float rad = angle * DEG_TO_RAD;
        int x = cx + static_cast<int>(r * cos(rad));
        int y = cy + static_cast<int>(r * sin(rad));
        SetPixel(x, y, character, color);
    }
}

void ConsoleEngine::DrawRect(int x, int y, int width, int height, char character, Color color) {
    // Top and bottom lines
    for (int i = 0; i < width; ++i) {
        SetPixel(x + i, y, character, color);
        SetPixel(x + i, y + height - 1, character, color);
    }
    
    // Left and right lines
    for (int i = 0; i < height; ++i) {
        SetPixel(x, y + i, character, color);
        SetPixel(x + width - 1, y + i, character, color);
    }
}

void ConsoleEngine::FillRect(int x, int y, int width, int height, char character, Color color) {
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            SetPixel(x + i, y + j, character, color);
        }
    }
}

void ConsoleEngine::Present() {
    WriteConsoleOutput(hConsole, screenBuffer, screenSize, {0, 0}, &writeRegion);
}

bool ConsoleEngine::IsKeyPressed(int virtualKey) {
    if (virtualKey < 0 || virtualKey >= 256) return false;
    UpdateInputStates();
    return keyStates[virtualKey] && !prevKeyStates[virtualKey];
}

bool ConsoleEngine::IsKeyDown(int virtualKey) {
    if (virtualKey < 0 || virtualKey >= 256) return false;
    UpdateInputStates();
    return keyStates[virtualKey];
}

bool ConsoleEngine::IsSpacePressed() {
    // Simple and reliable space detection
    UpdateInputStates();

    // Check if space was just pressed (not held down)
    bool justPressed = keyStates[VK_SPACE] && !prevKeyStates[VK_SPACE];

    // Also check direct GetAsyncKeyState for immediate response
    bool currentlyPressed = (GetAsyncKeyState(VK_SPACE) & 0x8001) == 0x8001;

    return justPressed || currentlyPressed;
}

COORD ConsoleEngine::GetMousePosition() {
    UpdateInputStates();
    return mousePos;
}

bool ConsoleEngine::IsMouseButtonPressed(int button) {
    UpdateInputStates();
    if (button < 0 || button > 1) return false;
    return mouseButtons[button] && !prevMouseButtons[button];
}

void ConsoleEngine::UpdateInputStates() {
    // Copy current states to previous
    memcpy(prevKeyStates, keyStates, sizeof(keyStates));
    memcpy(prevMouseButtons, mouseButtons, sizeof(mouseButtons));

    // Update keyboard states using GetAsyncKeyState
    for (int i = 0; i < 256; ++i) {
        keyStates[i] = (GetAsyncKeyState(i) & 0x8000) != 0;
    }

    // Also process console input events
    INPUT_RECORD inputRecord;
    DWORD eventsRead;

    while (PeekConsoleInput(hInput, &inputRecord, 1, &eventsRead) && eventsRead > 0) {
        ReadConsoleInput(hInput, &inputRecord, 1, &eventsRead);

        if (inputRecord.EventType == KEY_EVENT) {
            KEY_EVENT_RECORD keyEvent = inputRecord.Event.KeyEvent;
            if (keyEvent.bKeyDown) {
                keyStates[keyEvent.wVirtualKeyCode] = true;

                // Special handling for space key to ensure it works
                if (keyEvent.wVirtualKeyCode == VK_SPACE || keyEvent.uChar.AsciiChar == ' ') {
                    keyStates[VK_SPACE] = true;
                    keyStates[32] = true; // ASCII space
                }
            }
        }
        else if (inputRecord.EventType == MOUSE_EVENT) {
            MOUSE_EVENT_RECORD mouseEvent = inputRecord.Event.MouseEvent;
            mousePos = mouseEvent.dwMousePosition;

            mouseButtons[0] = (mouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) != 0;
            mouseButtons[1] = (mouseEvent.dwButtonState & RIGHTMOST_BUTTON_PRESSED) != 0;
        }
    }
}

void ConsoleEngine::SetCursorVisible(bool visible) {
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = visible;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

void ConsoleEngine::SetTitle(const std::string& title) {
    SetConsoleTitleA(title.c_str());
}

void ConsoleEngine::PlayBeep(int frequency, int duration) {
    Beep(frequency, duration);
}

void ConsoleEngine::DrawProgressBar(int x, int y, int width, float percentage, Color fillColor, Color bgColor) {
    int fillWidth = static_cast<int>(width * std::clamp(percentage, 0.0f, 1.0f));

    for (int i = 0; i < width; ++i) {
        char ch = i < fillWidth ? '#' : '-';
        Color color = i < fillWidth ? fillColor : bgColor;
        SetPixel(x + i, y, ch, color);
    }
}

void ConsoleEngine::DrawHealthBar(int x, int y, int width, float health, float maxHealth) {
    float percentage = health / maxHealth;
    Color color = percentage > 0.6f ? Color::GREEN : 
                  percentage > 0.3f ? Color::YELLOW : Color::RED;
    DrawProgressBar(x, y, width, percentage, color, Color::DARK_GRAY);
}

void ConsoleEngine::LimitFPS(int targetFPS) {
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto frameDuration = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - lastFrameTime);
    
    auto targetDuration = std::chrono::microseconds(1000000 / targetFPS);
    
    if (frameDuration < targetDuration) {
        auto sleepTime = targetDuration - frameDuration;
        std::this_thread::sleep_for(sleepTime); // Sleep for the remaining time to reach the target FPS
        currentTime = std::chrono::high_resolution_clock::now();
    }
    
    deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
    lastFrameTime = currentTime;
}

float ConsoleEngine::GetDeltaTime() {
    return deltaTime;
}
