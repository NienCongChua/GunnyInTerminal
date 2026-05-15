#include "GameEngine.h"
#include <iostream>
#include <memory>

int main() {
    try {
        // Create and initialize game engine
        auto game = std::make_unique<GameEngine>();
        
        if (!game->Initialize()) {
            std::cerr << "Failed to initialize game engine!" << std::endl;
            return -1;
        }
        
        // Run the game
        game->Run();
        
        // Cleanup is handled by destructor
        
    } catch (const std::exception& e) {
        std::cerr << "Game crashed with error: " << e.what() << std::endl;
        return -1;
    } catch (...) {
        std::cerr << "Game crashed with unknown error!" << std::endl;
        return -1;
    }
    
    return 0;
}
