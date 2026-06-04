#include "ConsoleEngine.h"

#include <string>

int main() {
    ConsoleEngine console(60, 12);
    if (!console.Initialize()) {
        return 1;
    }

    std::string lastEvent = "Waiting for input...";
    bool running = true;

    while (running) {
        console.LimitFPS(30);
        console.PollInput();

        if (console.IsKeyPressed(VK_ESCAPE)) {
            lastEvent = "ESC pressed - exiting";
            running = false;
        } else if (console.IsKeyPressed(VK_RETURN)) {
            lastEvent = "ENTER pressed";
        } else if (console.IsKeyPressed(VK_UP)) {
            lastEvent = "UP arrow pressed";
        } else if (console.IsKeyPressed(VK_DOWN)) {
            lastEvent = "DOWN arrow pressed";
        } else if (console.IsKeyPressed(VK_LEFT)) {
            lastEvent = "LEFT arrow pressed";
        } else if (console.IsKeyPressed(VK_RIGHT)) {
            lastEvent = "RIGHT arrow pressed";
        } else if (console.IsKeyPressed(VK_SPACE)) {
            lastEvent = "SPACE pressed";
        }

        console.ClearScreen();
        console.DrawString(2, 2, "Input Test", Color::YELLOW);
        console.DrawString(2, 4, "Press arrows, Enter, Space, or Esc.", Color::WHITE);
        console.DrawString(2, 6, lastEvent, running ? Color::CYAN : Color::GREEN);
        console.Present();
    }

    console.Cleanup();
    return 0;
}
