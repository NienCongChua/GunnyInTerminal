#pragma once
#include "GameStructures.h"
#include "ConsoleEngine.h"
#include "Terrain.h"
#include <vector>
#include <memory>

class GameEngine {
private:
    std::unique_ptr<ConsoleEngine> console;
    std::unique_ptr<Terrain> terrain;
    
    // Game state
    GameState currentState;
    std::vector<Player> players;
    std::vector<Bullet> bullets;
    std::vector<Particle> particles;
    std::vector<Explosion> explosions;
    
    int currentPlayerIndex;
    bool gameRunning;
    float windForce;

    // Turn timer
    float turnTimeLeft;
    float maxTurnTime;
    
    // Menu state
    int selectedMenuItem;
    std::vector<std::string> menuItems;
    
    // Input handling
    bool leftMousePressed;
    bool rightMousePressed;
    COORD mousePosition;
    
    // Game settings
    int numPlayers;
    float gameSpeed;
    bool soundEnabled;
    
public:
    GameEngine();
    ~GameEngine();
    
    // Main game functions
    bool Initialize();
    void Run();
    void Cleanup();
    
    // Game loop
    void Update(float deltaTime);
    void Render();
    void HandleInput();
    
    // Game state management
    void SetGameState(GameState newState);
    void StartNewGame();
    void PauseGame();
    void ResumeGame();
    void EndGame();
    
    // Player management
    void AddPlayer(const std::string& name, Point2D position, Color color);
    void NextPlayer();
    Player& GetCurrentPlayer();
    bool IsGameOver() const;
    
    // Weapon and shooting
    void FireBullet(Point2D startPos, float angle, float power, BulletType type);
    void UpdateBullets(float deltaTime);
    void HandleBulletCollision(Bullet& bullet);
    
    // Effects
    void CreateExplosion(Point2D center, float radius);
    void AddParticles(Point2D center, int count, Color color);
    void UpdateParticles(float deltaTime);
    void UpdateExplosions(float deltaTime);
    
    // Physics
    Vector2D CalculateTrajectory(float angle, float power) const;
    bool CheckCollision(Point2D position, float radius = 1.0f) const;
    void ApplyWind(Vector2D& velocity) const;
    
    // UI rendering
    void RenderMenu();
    void RenderTutorial();
    void RenderGame();
    void RenderHUD();
    void RenderPlayerHUD(const Player& player, int x, int y, bool isCurrentPlayer);
    void RenderTurnTimer();
    void RenderPlayerInfo();
    void RenderWeaponSelector();
    void RenderPauseMenu();
    void RenderGameOver();
    void RenderSettings();
    
    // Menu handling
    void HandleMenuInput();
    void HandleGameInput();
    void HandlePauseInput();
    
private:
    // Helper functions
    void InitializePlayers();
    void ResetGame();
    void CheckPlayerCollisions();
    void UpdateWind();
    void UpdateTurnTimer(float deltaTime);
    void PlaySoundEffect(int frequency, int duration);
    
    // Rendering helpers
    void DrawTrajectoryPreview();
    void DrawCrosshair();
    void DrawWindIndicator();
    void DrawMiniMap();
    
    // Input helpers
    bool IsKeyJustPressed(int key);
    void UpdateMouseInput();
    
    // Game logic helpers
    float CalculateDamage(Point2D explosionCenter, Point2D targetPos, float explosionRadius) const;
    void DamagePlayersInRadius(Point2D center, float radius, float baseDamage);
};
