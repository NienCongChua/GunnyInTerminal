#include "ConsoleEngine.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#ifdef _WIN32
#include <conio.h>
#else
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace {
#ifndef _WIN32
    termios g_originalTermios{};
    int g_originalInputFlags = 0;
    bool g_terminalConfigured = false;
    std::string g_inputBuffer;
    std::array<std::chrono::steady_clock::time_point, 256> g_keyHoldUntil{};
    bool g_mousePressedThisFrame[2] = {false, false};

    constexpr auto kLinuxKeyHoldDuration = std::chrono::milliseconds(100);

    int AnsiColorCode(int color, bool background) {
        static const int foregroundCodes[] = {
            30, 34, 32, 36, 31, 35, 33, 37,
            90, 94, 92, 96, 91, 95, 93, 97
        };
        static const int backgroundCodes[] = {
            40, 44, 42, 46, 41, 45, 43, 47,
            100, 104, 102, 106, 101, 105, 103, 107
        };

        color = std::clamp(color, 0, 15);
        return background ? backgroundCodes[color] : foregroundCodes[color];
    }

    void AppendAnsiColor(std::string& output, WORD attributes) {
        int foreground = attributes & 0x0F;
        int background = (attributes >> 4) & 0x0F;
        output += "\033[";
        output += std::to_string(AnsiColorCode(foreground, false));
        output += ';';
        output += std::to_string(AnsiColorCode(background, true));
        output += 'm';
    }
#endif
}

ConsoleEngine::ConsoleEngine(int width, int height)
#ifdef _WIN32
    : hConsole(INVALID_HANDLE_VALUE),
      hInput(INVALID_HANDLE_VALUE),
      screenSize({static_cast<SHORT>(width), static_cast<SHORT>(height)}),
#else
    : screenSize({static_cast<SHORT>(width), static_cast<SHORT>(height)}),
#endif
      screenBuffer(nullptr),
      prevScreenBuffer(nullptr),
      writeRegion({0, 0, static_cast<SHORT>(width - 1), static_cast<SHORT>(height - 1)}),
      mouseEnabled(true),
      deltaTime(0.0f) {

    int bufferSize = width * height;
    screenBuffer = new CHAR_INFO[bufferSize];
    prevScreenBuffer = new CHAR_INFO[bufferSize];

    std::memset(screenBuffer, 0, bufferSize * sizeof(CHAR_INFO));
    std::memset(prevScreenBuffer, 0, bufferSize * sizeof(CHAR_INFO));
    std::memset(keyStates, 0, sizeof(keyStates));
    std::memset(prevKeyStates, 0, sizeof(prevKeyStates));
    std::memset(mouseButtons, 0, sizeof(mouseButtons));
    std::memset(prevMouseButtons, 0, sizeof(prevMouseButtons));

    mousePos = {0, 0};
    lastFrameTime = std::chrono::high_resolution_clock::now();
}

ConsoleEngine::~ConsoleEngine() {
    Cleanup();
    delete[] screenBuffer;
    delete[] prevScreenBuffer;
}

bool ConsoleEngine::Initialize() {
#ifdef _WIN32
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    hInput = GetStdHandle(STD_INPUT_HANDLE);

    if (hConsole == INVALID_HANDLE_VALUE || hInput == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD mode;
    GetConsoleMode(hInput, &mode);
    mode |= ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS;
    mode &= ~ENABLE_QUICK_EDIT_MODE;
    SetConsoleMode(hInput, mode);

    COORD bufferSize = screenSize;
    SetConsoleScreenBufferSize(hConsole, bufferSize);

    SMALL_RECT windowSize = {0, 0, static_cast<SHORT>(screenSize.X - 1), static_cast<SHORT>(screenSize.Y - 1)};
    SetConsoleWindowInfo(hConsole, TRUE, &windowSize);

    SetCursorVisible(false);

    DWORD consoleMode;
    GetConsoleMode(hConsole, &consoleMode);
    consoleMode |= ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;
    SetConsoleMode(hConsole, consoleMode);
#else
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        return false;
    }

    if (!g_terminalConfigured) {
        if (tcgetattr(STDIN_FILENO, &g_originalTermios) != 0) {
            return false;
        }

        termios raw = g_originalTermios;
        raw.c_lflag &= static_cast<tcflag_t>(~(ECHO | ICANON | IEXTEN));
        raw.c_iflag &= static_cast<tcflag_t>(~(IXON | ICRNL));
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;

        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
            return false;
        }

        g_originalInputFlags = fcntl(STDIN_FILENO, F_GETFL, 0);
        if (g_originalInputFlags == -1) {
            g_originalInputFlags = 0;
        }
        fcntl(STDIN_FILENO, F_SETFL, g_originalInputFlags | O_NONBLOCK);

        g_terminalConfigured = true;
    }

    std::cout << "\033[?1049h\033[2J\033[H\033[?25l\033[?1000h\033[?1006h";
    std::cout.flush();
#endif

    ClearScreen();
    return true;
}

void ConsoleEngine::Cleanup() {
#ifdef _WIN32
    if (hConsole != INVALID_HANDLE_VALUE) {
        SetCursorVisible(true);
    }

    if (hInput != INVALID_HANDLE_VALUE) {
        DWORD mode;
        GetConsoleMode(hInput, &mode);
        mode |= ENABLE_QUICK_EDIT_MODE;
        SetConsoleMode(hInput, mode);
    }
#else
    if (g_terminalConfigured) {
        std::cout << "\033[0m\033[?1006l\033[?1000l\033[?25h\033[?1049l";
        std::cout.flush();

        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_originalTermios);
        fcntl(STDIN_FILENO, F_SETFL, g_originalInputFlags);
        g_terminalConfigured = false;
    }
#endif
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

    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
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
        int x = cx + static_cast<int>(r * std::cos(rad));
        int y = cy + static_cast<int>(r * std::sin(rad));
        SetPixel(x, y, character, color);
    }
}

void ConsoleEngine::DrawRect(int x, int y, int width, int height, char character, Color color) {
    for (int i = 0; i < width; ++i) {
        SetPixel(x + i, y, character, color);
        SetPixel(x + i, y + height - 1, character, color);
    }

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
#ifdef _WIN32
    WriteConsoleOutput(hConsole, screenBuffer, screenSize, {0, 0}, &writeRegion);
#else
    std::string output;
    output.reserve(static_cast<size_t>(screenSize.X * screenSize.Y * 2));
    output += "\033[H";

    WORD currentAttributes = 0xFFFF;
    for (int y = 0; y < screenSize.Y; ++y) {
        for (int x = 0; x < screenSize.X; ++x) {
            int index = y * screenSize.X + x;
            WORD attributes = screenBuffer[index].Attributes;
            if (attributes != currentAttributes) {
                AppendAnsiColor(output, attributes);
                currentAttributes = attributes;
            }

            char ch = screenBuffer[index].Char.AsciiChar;
            output += ch == '\0' ? ' ' : ch;
        }
        if (y + 1 < screenSize.Y) {
            output += '\n';
        }
    }

    output += "\033[0m";
    std::cout << output;
    std::cout.flush();
#endif
}

void ConsoleEngine::PollInput() {
    UpdateInputStates();
}

bool ConsoleEngine::IsKeyPressed(int virtualKey) {
    if (virtualKey < 0 || virtualKey >= 256) return false;
    return keyStates[virtualKey] && !prevKeyStates[virtualKey];
}

bool ConsoleEngine::IsKeyDown(int virtualKey) {
    if (virtualKey < 0 || virtualKey >= 256) return false;
    return keyStates[virtualKey];
}

bool ConsoleEngine::IsSpacePressed() {
    return IsKeyPressed(VK_SPACE);
}

COORD ConsoleEngine::GetMousePosition() {
    return mousePos;
}

bool ConsoleEngine::IsMouseButtonPressed(int button) {
    if (button < 0 || button > 1) return false;
#ifdef _WIN32
    return mouseButtons[button] && !prevMouseButtons[button];
#else
    return g_mousePressedThisFrame[button] || (mouseButtons[button] && !prevMouseButtons[button]);
#endif
}

void ConsoleEngine::UpdateInputStates() {
    std::memcpy(prevKeyStates, keyStates, sizeof(keyStates));
    std::memcpy(prevMouseButtons, mouseButtons, sizeof(mouseButtons));

#ifdef _WIN32
    for (int i = 0; i < 256; ++i) {
        keyStates[i] = (GetAsyncKeyState(i) & 0x8000) != 0;
    }

    mouseButtons[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    mouseButtons[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

    INPUT_RECORD inputRecord;
    DWORD eventsRead;

    while (PeekConsoleInput(hInput, &inputRecord, 1, &eventsRead) && eventsRead > 0) {
        ReadConsoleInput(hInput, &inputRecord, 1, &eventsRead);

        if (inputRecord.EventType == KEY_EVENT) {
            KEY_EVENT_RECORD keyEvent = inputRecord.Event.KeyEvent;
            if (keyEvent.bKeyDown) {
                keyStates[keyEvent.wVirtualKeyCode] = true;

                if (keyEvent.wVirtualKeyCode == VK_SPACE || keyEvent.uChar.AsciiChar == ' ') {
                    keyStates[VK_SPACE] = true;
                    keyStates[32] = true;
                }
            }
        } else if (inputRecord.EventType == MOUSE_EVENT) {
            MOUSE_EVENT_RECORD mouseEvent = inputRecord.Event.MouseEvent;
            mousePos = mouseEvent.dwMousePosition;

            mouseButtons[0] = (mouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) != 0;
            mouseButtons[1] = (mouseEvent.dwButtonState & RIGHTMOST_BUTTON_PRESSED) != 0;
        }
    }
#else
    std::memset(keyStates, 0, sizeof(keyStates));
    g_mousePressedThisFrame[0] = false;
    g_mousePressedThisFrame[1] = false;

    char readBuffer[256];
    while (true) {
        ssize_t bytesRead = read(STDIN_FILENO, readBuffer, sizeof(readBuffer));
        if (bytesRead > 0) {
            g_inputBuffer.append(readBuffer, static_cast<size_t>(bytesRead));
            continue;
        }
        if (bytesRead == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            break;
        }
        break;
    }

    auto now = std::chrono::steady_clock::now();
    auto pressKey = [&](int key) {
        if (key < 0 || key >= 256) return;
        keyStates[key] = true;
        g_keyHoldUntil[static_cast<size_t>(key)] = now + kLinuxKeyHoldDuration;
    };

    while (!g_inputBuffer.empty()) {
        unsigned char ch = static_cast<unsigned char>(g_inputBuffer[0]);

        if (ch == '\033') {
            if (g_inputBuffer.size() >= 3 && g_inputBuffer[1] == '[') {
                char code = g_inputBuffer[2];
                if (code == 'A' || code == 'B' || code == 'C' || code == 'D') {
                    switch (code) {
                        case 'A': pressKey(VK_UP); break;
                        case 'B': pressKey(VK_DOWN); break;
                        case 'C': pressKey(VK_RIGHT); break;
                        case 'D': pressKey(VK_LEFT); break;
                    }
                    g_inputBuffer.erase(0, 3);
                    continue;
                }

                if (code == '<') {
                    size_t end = g_inputBuffer.find_first_of("Mm", 3);
                    if (end == std::string::npos) {
                        break;
                    }

                    int button = 0;
                    int x = 0;
                    int y = 0;
                    std::string payload = g_inputBuffer.substr(3, end - 3);
                    if (std::sscanf(payload.c_str(), "%d;%d;%d", &button, &x, &y) == 3) {
                        mousePos.X = static_cast<SHORT>(std::max(0, x - 1));
                        mousePos.Y = static_cast<SHORT>(std::max(0, y - 1));

                        int baseButton = button & 0x03;
                        bool pressed = g_inputBuffer[end] == 'M';
                        if (baseButton == 0) {
                            if (pressed) g_mousePressedThisFrame[0] = true;
                            mouseButtons[0] = pressed;
                        } else if (baseButton == 2) {
                            if (pressed) g_mousePressedThisFrame[1] = true;
                            mouseButtons[1] = pressed;
                        } else if (!pressed) {
                            mouseButtons[0] = false;
                            mouseButtons[1] = false;
                        }
                    }

                    g_inputBuffer.erase(0, end + 1);
                    continue;
                }
            }

            pressKey(VK_ESCAPE);
            g_inputBuffer.erase(0, 1);
            continue;
        }

        if (ch == '\r' || ch == '\n') {
            pressKey(VK_RETURN);
        } else if (ch == ' ') {
            pressKey(VK_SPACE);
            pressKey(32);
        } else if (std::isprint(ch)) {
            pressKey(ch);
            pressKey(std::toupper(ch));
        }

        g_inputBuffer.erase(0, 1);
    }

    for (int i = 0; i < 256; ++i) {
        if (g_keyHoldUntil[static_cast<size_t>(i)] > now) {
            keyStates[i] = true;
        }
    }
#endif
}

void ConsoleEngine::SetCursorVisible(bool visible) {
#ifdef _WIN32
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = visible;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
#else
    std::cout << (visible ? "\033[?25h" : "\033[?25l");
    std::cout.flush();
#endif
}

void ConsoleEngine::SetTitle(const std::string& title) {
#ifdef _WIN32
    SetConsoleTitleA(title.c_str());
#else
    std::cout << "\033]0;" << title << '\007';
    std::cout.flush();
#endif
}

void ConsoleEngine::PlayBeep(int frequency, int duration) {
#ifdef _WIN32
    Beep(frequency, duration);
#else
    (void)frequency;
    std::cout << '\a';
    std::cout.flush();
    if (duration > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(duration));
    }
#endif
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
        std::this_thread::sleep_for(sleepTime);
        currentTime = std::chrono::high_resolution_clock::now();
    }

    deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
    lastFrameTime = currentTime;
}

float ConsoleEngine::GetDeltaTime() {
    return deltaTime;
}
