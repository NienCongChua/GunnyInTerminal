#include "GameEngine.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>

namespace {
    constexpr float kTurnSeconds = 30.0f;
    constexpr int kTargetFps = 60;
    constexpr float kMaxDeltaTime = 0.05f;

    float ClampFloat(float value, float minValue, float maxValue) {
        return std::max(minValue, std::min(maxValue, value));
    }

    int WrapIndex(int value, int count) {
        if (count <= 0) return 0;
        if (value < 0) return count - 1;
        if (value >= count) return 0;
        return value;
    }

    std::string FormatFloat(float value, int precision = 1) {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(precision) << value;
        return stream.str();
    }

    std::string WeaponName(BulletType type) {
        switch (type) {
            case BulletType::NORMAL: return "Normal";
            case BulletType::EXPLOSIVE: return "Explosive";
            case BulletType::TRIPLE_SHOT: return "Triple";
            case BulletType::LASER: return "Laser";
        }
        return "Unknown";
    }

    BulletType NextWeapon(BulletType type) {
        switch (type) {
            case BulletType::NORMAL: return BulletType::EXPLOSIVE;
            case BulletType::EXPLOSIVE: return BulletType::TRIPLE_SHOT;
            case BulletType::TRIPLE_SHOT: return BulletType::LASER;
            case BulletType::LASER: return BulletType::NORMAL;
        }
        return BulletType::NORMAL;
    }

    BulletType PreviousWeapon(BulletType type) {
        switch (type) {
            case BulletType::NORMAL: return BulletType::LASER;
            case BulletType::EXPLOSIVE: return BulletType::NORMAL;
            case BulletType::TRIPLE_SHOT: return BulletType::EXPLOSIVE;
            case BulletType::LASER: return BulletType::TRIPLE_SHOT;
        }
        return BulletType::NORMAL;
    }

    Color PlayerColor(int index) {
        static const Color colors[] = {
            Color::CYAN,
            Color::YELLOW,
            Color::GREEN,
            Color::MAGENTA
        };
        return colors[index % 4];
    }

    char PlayerSymbol(int index) {
        static const char symbols[] = {'@', '&', '$', 'A'};
        return symbols[index % 4];
    }
}

GameEngine::GameEngine()
    : currentState(GameState::MENU),
      currentPlayerIndex(0),
      gameRunning(false),
      windForce(0.0f),
      turnTimeLeft(kTurnSeconds),
      maxTurnTime(kTurnSeconds),
      selectedMenuItem(0),
      menuItems({"New Game", "Tutorial", "Settings", "Exit"}),
      leftMousePressed(false),
      rightMousePressed(false),
      mousePosition({0, 0}),
      numPlayers(2),
      gameSpeed(1.0f),
      soundEnabled(true) {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
}

GameEngine::~GameEngine() {
    Cleanup();
}

bool GameEngine::Initialize() {
    console = std::make_unique<ConsoleEngine>(SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!console->Initialize()) {
        return false;
    }

    console->SetTitle("Gunny Terminal Edition");
    terrain = std::make_unique<Terrain>(SCREEN_WIDTH, SCREEN_HEIGHT);
    terrain->GenerateHillTerrain();

    gameRunning = true;
    currentState = GameState::MENU;
    return true;
}

void GameEngine::Run() {
    while (gameRunning) {
        console->LimitFPS(kTargetFps);
        float deltaTime = std::min(console->GetDeltaTime(), kMaxDeltaTime);

        HandleInput();
        Update(deltaTime * gameSpeed);
        Render();
    }
}

void GameEngine::Cleanup() {
    if (console) {
        console->Cleanup();
    }
}

void GameEngine::Update(float deltaTime) {
    switch (currentState) {
        case GameState::PLAYING: {
            static float windTimer = 0.0f;
            windTimer += deltaTime;
            if (windTimer >= 5.0f) {
                UpdateWind();
                windTimer = 0.0f;
            }

            UpdateTurnTimer(deltaTime);
            UpdateBullets(deltaTime);
            UpdateParticles(deltaTime);
            UpdateExplosions(deltaTime);
            CheckPlayerCollisions();

            if (IsGameOver()) {
                EndGame();
            }
            break;
        }
        case GameState::PAUSED:
        case GameState::GAME_OVER:
        case GameState::MENU:
        case GameState::TUTORIAL:
        case GameState::SETTINGS:
            UpdateParticles(deltaTime);
            UpdateExplosions(deltaTime);
            break;
    }
}

void GameEngine::Render() {
    if (!console) return;

    console->ClearScreen();

    switch (currentState) {
        case GameState::MENU:
            RenderMenu();
            break;
        case GameState::TUTORIAL:
            RenderTutorial();
            break;
        case GameState::PLAYING:
            RenderGame();
            break;
        case GameState::PAUSED:
            RenderGame();
            RenderPauseMenu();
            break;
        case GameState::GAME_OVER:
            RenderGame();
            RenderGameOver();
            break;
        case GameState::SETTINGS:
            RenderSettings();
            break;
    }

    console->Present();
}

void GameEngine::HandleInput() {
    UpdateMouseInput();

    switch (currentState) {
        case GameState::MENU:
        case GameState::TUTORIAL:
        case GameState::SETTINGS:
        case GameState::GAME_OVER:
            HandleMenuInput();
            break;
        case GameState::PLAYING:
            HandleGameInput();
            break;
        case GameState::PAUSED:
            HandlePauseInput();
            break;
    }
}

void GameEngine::SetGameState(GameState newState) {
    currentState = newState;
}

void GameEngine::StartNewGame() {
    ResetGame();
    SetGameState(GameState::PLAYING);
    PlaySoundEffect(740, 80);
}

void GameEngine::PauseGame() {
    selectedMenuItem = 0;
    SetGameState(GameState::PAUSED);
}

void GameEngine::ResumeGame() {
    SetGameState(GameState::PLAYING);
}

void GameEngine::EndGame() {
    selectedMenuItem = 0;
    SetGameState(GameState::GAME_OVER);
}

void GameEngine::AddPlayer(const std::string& name, Point2D position, Color color) {
    players.emplace_back(position, name, color);
    players.back().symbol = PlayerSymbol(static_cast<int>(players.size()) - 1);
}

void GameEngine::NextPlayer() {
    if (players.empty()) return;
    if (IsGameOver()) {
        EndGame();
        return;
    }

    for (size_t i = 1; i <= players.size(); ++i) {
        int nextIndex = static_cast<int>((currentPlayerIndex + i) % players.size());
        if (players[nextIndex].isAlive) {
            currentPlayerIndex = nextIndex;
            break;
        }
    }

    turnTimeLeft = maxTurnTime;
    UpdateWind();
}

Player& GameEngine::GetCurrentPlayer() {
    if (players.empty()) {
        throw std::runtime_error("No players have been initialized");
    }
    return players[currentPlayerIndex];
}

bool GameEngine::IsGameOver() const {
    int alivePlayers = 0;
    for (const auto& player : players) {
        if (player.isAlive) {
            ++alivePlayers;
        }
    }
    return !players.empty() && alivePlayers <= 1;
}

void GameEngine::FireBullet(Point2D startPos, float angle, float power, BulletType type) {
    if (type == BulletType::TRIPLE_SHOT) {
        for (float offset : {-5.0f, 0.0f, 5.0f}) {
            float shotAngle = ClampFloat(angle + offset, 0.0f, 180.0f);
            bullets.emplace_back(startPos, CalculateTrajectory(shotAngle, power), BulletType::TRIPLE_SHOT);
        }
    } else {
        bullets.emplace_back(startPos, CalculateTrajectory(angle, power), type);
    }

    PlaySoundEffect(880, 45);
}

void GameEngine::UpdateBullets(float deltaTime) {
    bool hadBullets = !bullets.empty();

    for (auto& bullet : bullets) {
        if (!bullet.active) continue;

        bullet.prevPosition = bullet.position;
        bullet.trailTimer += deltaTime;

        ApplyWind(bullet.velocity);
        bullet.velocity.y += GRAVITY * 4.0f * deltaTime;
        bullet.position = bullet.position + bullet.velocity * deltaTime;

        if (bullet.position.x < 0 || bullet.position.x >= SCREEN_WIDTH ||
            bullet.position.y >= SCREEN_HEIGHT) {
            bullet.active = false;
            continue;
        }

        if (bullet.position.y >= 0 && CheckCollision(bullet.position, 1.0f)) {
            HandleBulletCollision(bullet);
        }
    }

    bullets.erase(
        std::remove_if(bullets.begin(), bullets.end(), [](const Bullet& bullet) {
            return !bullet.active;
        }),
        bullets.end()
    );

    if (hadBullets && bullets.empty() && currentState == GameState::PLAYING) {
        if (IsGameOver()) {
            EndGame();
        } else {
            NextPlayer();
        }
    }
}

void GameEngine::HandleBulletCollision(Bullet& bullet) {
    if (!bullet.active) return;

    bullet.active = false;
    CreateExplosion(bullet.position, bullet.explosionRadius);
    DamagePlayersInRadius(bullet.position, bullet.explosionRadius, bullet.damage);
    AddParticles(bullet.position, 24, bullet.color);
    PlaySoundEffect(140, 70);
}

void GameEngine::CreateExplosion(Point2D center, float radius) {
    explosions.emplace_back(center, radius);
    if (terrain) {
        terrain->CreateExplosion(center, radius);
    }
}

void GameEngine::AddParticles(Point2D center, int count, Color color) {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * PI);
    std::uniform_real_distribution<float> speedDist(8.0f, 32.0f);
    std::uniform_real_distribution<float> lifeDist(0.35f, 0.9f);

    for (int i = 0; i < count; ++i) {
        float angle = angleDist(rng);
        float speed = speedDist(rng);
        Vector2D velocity(std::cos(angle) * speed, std::sin(angle) * speed);
        char symbol = (i % 3 == 0) ? '*' : ((i % 3 == 1) ? '+' : '.');
        particles.emplace_back(center, velocity, color, symbol, lifeDist(rng));
    }
}

void GameEngine::UpdateParticles(float deltaTime) {
    for (auto& particle : particles) {
        if (!particle.active) continue;
        particle.velocity.y += GRAVITY * 2.0f * deltaTime;
        particle.update(deltaTime);
    }

    particles.erase(
        std::remove_if(particles.begin(), particles.end(), [](const Particle& particle) {
            return !particle.active;
        }),
        particles.end()
    );
}

void GameEngine::UpdateExplosions(float deltaTime) {
    for (auto& explosion : explosions) {
        explosion.update(deltaTime * 2.0f);
    }

    explosions.erase(
        std::remove_if(explosions.begin(), explosions.end(), [](const Explosion& explosion) {
            return !explosion.active;
        }),
        explosions.end()
    );
}

Vector2D GameEngine::CalculateTrajectory(float angle, float power) const {
    float radians = angle * DEG_TO_RAD;
    float speed = ClampFloat(power, 1.0f, 100.0f) * 0.95f;
    return Vector2D(std::cos(radians) * speed, -std::sin(radians) * speed);
}

bool GameEngine::CheckCollision(Point2D position, float radius) const {
    if (!terrain) return false;
    if (position.y >= SCREEN_HEIGHT) return true;
    if (position.x < 0 || position.x >= SCREEN_WIDTH) return true;
    if (position.y < 0) return false;

    int minX = static_cast<int>(std::floor(position.x - radius));
    int maxX = static_cast<int>(std::ceil(position.x + radius));
    int minY = static_cast<int>(std::floor(position.y - radius));
    int maxY = static_cast<int>(std::ceil(position.y + radius));

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            Point2D sample(static_cast<float>(x), static_cast<float>(y));
            if (position.distance(sample) <= radius && terrain->IsTerrainAt(x, y)) {
                return true;
            }
        }
    }

    for (const auto& player : players) {
        if (player.isAlive && position.distance(player.position) <= radius + 1.1f) {
            return true;
        }
    }

    return false;
}

void GameEngine::ApplyWind(Vector2D& velocity) const {
    velocity.x += windForce * 0.018f;
}

void GameEngine::RenderMenu() {
    int centerX = SCREEN_WIDTH / 2;
    int titleY = 5;

    console->DrawString(centerX - 23, titleY,     "  #####  ##   ##  ##   ##  ##   ##  ##   ##", Color::YELLOW);
    console->DrawString(centerX - 23, titleY + 1, " ##      ##   ##  ###  ##  ###  ##   ## ## ", Color::YELLOW);
    console->DrawString(centerX - 23, titleY + 2, " ## ###  ##   ##  #### ##  #### ##    ###  ", Color::YELLOW);
    console->DrawString(centerX - 23, titleY + 3, " ##  ##  ##   ##  ## ####  ## ####    ###  ", Color::YELLOW);
    console->DrawString(centerX - 23, titleY + 4, "  ####    #####   ##  ###  ##  ###    ###  ", Color::YELLOW);
    console->DrawString(centerX - 15, titleY + 6, "Terminal Artillery Battle", Color::CYAN);

    int menuY = titleY + 10;
    for (size_t i = 0; i < menuItems.size(); ++i) {
        bool selected = static_cast<int>(i) == selectedMenuItem;
        Color color = selected ? Color::BLACK : Color::WHITE;
        Color bg = selected ? Color::YELLOW : Color::BLACK;
        std::string text = selected ? "> " + menuItems[i] + " <" : "  " + menuItems[i] + "  ";
        int x = centerX - static_cast<int>(text.size()) / 2;

        if (selected) {
            console->FillRect(centerX - 12, menuY + static_cast<int>(i) * 2, 24, 1, ' ', Color::BLACK);
        }
        for (size_t c = 0; c < text.size(); ++c) {
            console->SetPixel(x + static_cast<int>(c), menuY + static_cast<int>(i) * 2, text[c], color, bg);
        }
    }

    console->DrawString(centerX - 21, SCREEN_HEIGHT - 4, "Arrow keys: navigate | Enter: select | Esc: exit", Color::DARK_GRAY);
}

void GameEngine::RenderTutorial() {
    int x = 12;
    int y = 4;

    console->DrawString(x, y, "How to Play", Color::YELLOW);
    console->DrawString(x, y + 2, "Goal: destroy the other players before they destroy you.", Color::WHITE);
    console->DrawString(x, y + 4, "A / D        Adjust cannon angle", Color::CYAN);
    console->DrawString(x, y + 5, "W / S        Adjust shot power", Color::CYAN);
    console->DrawString(x, y + 6, "Left / Right Move along the terrain", Color::CYAN);
    console->DrawString(x, y + 7, "Q / E        Change weapon", Color::CYAN);
    console->DrawString(x, y + 8, "Space        Fire", Color::CYAN);
    console->DrawString(x, y + 9, "Mouse left   Aim at cursor", Color::CYAN);
    console->DrawString(x, y + 10, "Mouse right  Fire", Color::CYAN);
    console->DrawString(x, y + 11, "P or Esc     Pause", Color::CYAN);

    console->DrawString(x, y + 14, "Tips:", Color::YELLOW);
    console->DrawString(x, y + 16, "- Wind changes between turns and can bend long shots.", Color::WHITE);
    console->DrawString(x, y + 17, "- Explosive shots carve bigger holes and hit harder.", Color::WHITE);
    console->DrawString(x, y + 18, "- Triple shot is forgiving when you are still finding the angle.", Color::WHITE);

    console->DrawString(x, SCREEN_HEIGHT - 4, "Enter: start game | Esc: back to menu", Color::DARK_GRAY);
}

void GameEngine::RenderGame() {
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        Color color = y < SCREEN_HEIGHT / 3 ? Color::DARK_BLUE : Color::BLUE;
        for (int x = 0; x < SCREEN_WIDTH; x += 9) {
            if (((x + y * 3) % 37) == 0) {
                console->SetPixel(x, y, '.', color);
            }
        }
    }

    if (terrain) {
        terrain->Render(*console);
    }

    if (currentState == GameState::PLAYING && bullets.empty() && !players.empty()) {
        DrawTrajectoryPreview();
        DrawCrosshair();
    }

    for (const auto& player : players) {
        if (!player.isAlive) continue;
        int x = static_cast<int>(std::round(player.position.x));
        int y = static_cast<int>(std::round(player.position.y));
        console->SetPixel(x, y, player.symbol, player.color);
        console->SetPixel(x, y - 1, '^', player.color);
    }

    for (const auto& bullet : bullets) {
        if (!bullet.active) continue;
        int bx = static_cast<int>(std::round(bullet.position.x));
        int by = static_cast<int>(std::round(bullet.position.y));
        int tx = static_cast<int>(std::round(bullet.prevPosition.x));
        int ty = static_cast<int>(std::round(bullet.prevPosition.y));
        console->SetPixel(tx, ty, '.', Color::DARK_GRAY);
        console->SetPixel(bx, by, bullet.symbol, bullet.color);
    }

    for (const auto& explosion : explosions) {
        if (explosion.active) {
            console->DrawCircle(explosion.center, explosion.radius, '*', Color::RED);
        }
    }

    for (const auto& particle : particles) {
        if (!particle.active) continue;
        console->SetPixel(
            static_cast<int>(std::round(particle.position.x)),
            static_cast<int>(std::round(particle.position.y)),
            particle.symbol,
            particle.color
        );
    }

    RenderHUD();
}

void GameEngine::RenderHUD() {
    console->FillRect(0, 0, SCREEN_WIDTH, 4, ' ', Color::WHITE);
    console->DrawRect(0, 0, SCREEN_WIDTH, 4, '#', Color::DARK_GRAY);

    RenderPlayerInfo();
    RenderTurnTimer();
    RenderWeaponSelector();
    DrawWindIndicator();
    DrawMiniMap();
}

void GameEngine::RenderPlayerHUD(const Player& player, int x, int y, bool isCurrentPlayer) {
    Color labelColor = isCurrentPlayer ? Color::YELLOW : Color::LIGHT_GRAY;
    std::string marker = isCurrentPlayer ? "> " : "  ";
    console->DrawString(x, y, marker + player.name, labelColor);
    console->DrawHealthBar(x, y + 1, 16, player.health, player.maxHealth);
    console->DrawString(x + 18, y + 1, FormatFloat(player.health, 0) + " HP", labelColor);
}

void GameEngine::RenderTurnTimer() {
    if (currentState != GameState::PLAYING && currentState != GameState::PAUSED) return;

    float percentage = maxTurnTime > 0.0f ? turnTimeLeft / maxTurnTime : 0.0f;
    Color timerColor = percentage > 0.5f ? Color::GREEN : (percentage > 0.25f ? Color::YELLOW : Color::RED);
    int x = SCREEN_WIDTH / 2 - 10;
    console->DrawString(x, 1, "Turn", Color::WHITE);
    console->DrawProgressBar(x + 6, 1, 14, percentage, timerColor, Color::DARK_GRAY);
    console->DrawString(x + 22, 1, FormatFloat(turnTimeLeft, 0) + "s", timerColor);
}

void GameEngine::RenderPlayerInfo() {
    if (players.empty()) return;

    RenderPlayerHUD(players[0], 2, 1, currentPlayerIndex == 0);
    if (players.size() > 1) {
        RenderPlayerHUD(players[1], SCREEN_WIDTH - 30, 1, currentPlayerIndex == 1);
    }
}

void GameEngine::RenderWeaponSelector() {
    if (players.empty() || currentState == GameState::MENU) return;

    const Player& player = players[currentPlayerIndex];
    std::string info = "Angle " + FormatFloat(player.angle, 0) +
                       "  Power " + FormatFloat(player.power, 0) +
                       "  Weapon " + WeaponName(player.currentWeapon);
    console->DrawString(SCREEN_WIDTH / 2 - static_cast<int>(info.size()) / 2, 3, info, Color::CYAN);
}

void GameEngine::RenderPauseMenu() {
    int w = 34;
    int h = 11;
    int x = SCREEN_WIDTH / 2 - w / 2;
    int y = SCREEN_HEIGHT / 2 - h / 2;
    const std::vector<std::string> items = {"Resume", "New Game", "Settings", "Main Menu"};

    console->FillRect(x, y, w, h, ' ', Color::WHITE);
    console->DrawRect(x, y, w, h, '#', Color::YELLOW);
    console->DrawString(x + 12, y + 1, "Paused", Color::YELLOW);

    for (size_t i = 0; i < items.size(); ++i) {
        Color color = static_cast<int>(i) == selectedMenuItem ? Color::YELLOW : Color::WHITE;
        std::string prefix = static_cast<int>(i) == selectedMenuItem ? "> " : "  ";
        console->DrawString(x + 10, y + 3 + static_cast<int>(i) * 2, prefix + items[i], color);
    }
}

void GameEngine::RenderGameOver() {
    int w = 44;
    int h = 10;
    int x = SCREEN_WIDTH / 2 - w / 2;
    int y = SCREEN_HEIGHT / 2 - h / 2;

    console->FillRect(x, y, w, h, ' ', Color::WHITE);
    console->DrawRect(x, y, w, h, '#', Color::YELLOW);
    console->DrawString(x + 16, y + 1, "Game Over", Color::YELLOW);

    std::string winner = "No winner";
    for (const auto& player : players) {
        if (player.isAlive) {
            winner = player.name + " wins!";
            break;
        }
    }

    console->DrawString(x + 4, y + 4, winner, Color::GREEN);
    console->DrawString(x + 4, y + 6, "Enter: play again", Color::CYAN);
    console->DrawString(x + 4, y + 7, "Esc: main menu", Color::CYAN);
}

void GameEngine::RenderSettings() {
    const std::vector<std::string> labels = {
        "Players: " + std::to_string(numPlayers),
        "Game speed: " + FormatFloat(gameSpeed, 1) + "x",
        std::string("Sound: ") + (soundEnabled ? "On" : "Off"),
        "Back"
    };

    int centerX = SCREEN_WIDTH / 2;
    int y = 8;
    console->DrawString(centerX - 6, y - 3, "Settings", Color::YELLOW);

    for (size_t i = 0; i < labels.size(); ++i) {
        bool selected = static_cast<int>(i) == selectedMenuItem;
        Color color = selected ? Color::YELLOW : Color::WHITE;
        std::string prefix = selected ? "> " : "  ";
        console->DrawString(centerX - 14, y + static_cast<int>(i) * 3, prefix + labels[i], color);
    }

    console->DrawString(centerX - 24, SCREEN_HEIGHT - 4, "Left/Right: change | Enter/Esc: back", Color::DARK_GRAY);
}

void GameEngine::HandleMenuInput() {
    if (currentState == GameState::MENU) {
        if (IsKeyJustPressed(VK_UP)) {
            selectedMenuItem = WrapIndex(selectedMenuItem - 1, static_cast<int>(menuItems.size()));
        }
        if (IsKeyJustPressed(VK_DOWN)) {
            selectedMenuItem = WrapIndex(selectedMenuItem + 1, static_cast<int>(menuItems.size()));
        }
        if (IsKeyJustPressed(VK_ESCAPE)) {
            gameRunning = false;
        }
        if (IsKeyJustPressed(VK_RETURN)) {
            switch (selectedMenuItem) {
                case 0:
                    StartNewGame();
                    break;
                case 1:
                    SetGameState(GameState::TUTORIAL);
                    break;
                case 2:
                    selectedMenuItem = 0;
                    SetGameState(GameState::SETTINGS);
                    break;
                case 3:
                    gameRunning = false;
                    break;
            }
        }
        return;
    }

    if (currentState == GameState::TUTORIAL) {
        if (IsKeyJustPressed(VK_ESCAPE)) {
            selectedMenuItem = 0;
            SetGameState(GameState::MENU);
        }
        if (IsKeyJustPressed(VK_RETURN) || IsKeyJustPressed(VK_SPACE)) {
            StartNewGame();
        }
        return;
    }

    if (currentState == GameState::SETTINGS) {
        const int itemCount = 4;
        if (IsKeyJustPressed(VK_UP)) {
            selectedMenuItem = WrapIndex(selectedMenuItem - 1, itemCount);
        }
        if (IsKeyJustPressed(VK_DOWN)) {
            selectedMenuItem = WrapIndex(selectedMenuItem + 1, itemCount);
        }
        if (IsKeyJustPressed(VK_LEFT)) {
            if (selectedMenuItem == 0) numPlayers = std::max(2, numPlayers - 1);
            if (selectedMenuItem == 1) gameSpeed = ClampFloat(gameSpeed - 0.1f, 0.5f, 2.0f);
            if (selectedMenuItem == 2) soundEnabled = !soundEnabled;
        }
        if (IsKeyJustPressed(VK_RIGHT)) {
            if (selectedMenuItem == 0) numPlayers = std::min(4, numPlayers + 1);
            if (selectedMenuItem == 1) gameSpeed = ClampFloat(gameSpeed + 0.1f, 0.5f, 2.0f);
            if (selectedMenuItem == 2) soundEnabled = !soundEnabled;
        }
        if (IsKeyJustPressed(VK_RETURN) || IsKeyJustPressed(VK_ESCAPE)) {
            selectedMenuItem = 0;
            SetGameState(GameState::MENU);
        }
        return;
    }

    if (currentState == GameState::GAME_OVER) {
        if (IsKeyJustPressed(VK_RETURN)) {
            StartNewGame();
        }
        if (IsKeyJustPressed(VK_ESCAPE)) {
            selectedMenuItem = 0;
            SetGameState(GameState::MENU);
        }
    }
}

void GameEngine::HandleGameInput() {
    if (IsKeyJustPressed(VK_ESCAPE) || IsKeyJustPressed('P')) {
        PauseGame();
        return;
    }

    if (players.empty() || !bullets.empty()) {
        return;
    }

    Player& player = GetCurrentPlayer();
    if (!player.isAlive) {
        NextPlayer();
        return;
    }

    if (console->IsKeyDown('A')) {
        player.angle = ClampFloat(player.angle + 0.8f, 0.0f, 180.0f);
    }
    if (console->IsKeyDown('D')) {
        player.angle = ClampFloat(player.angle - 0.8f, 0.0f, 180.0f);
    }
    if (console->IsKeyDown('W')) {
        player.power = ClampFloat(player.power + 0.8f, 5.0f, 100.0f);
    }
    if (console->IsKeyDown('S')) {
        player.power = ClampFloat(player.power - 0.8f, 5.0f, 100.0f);
    }

    float moveAmount = 0.35f;
    if (console->IsKeyDown(VK_LEFT)) {
        player.position.x = ClampFloat(player.position.x - moveAmount, 1.0f, SCREEN_WIDTH - 2.0f);
    }
    if (console->IsKeyDown(VK_RIGHT)) {
        player.position.x = ClampFloat(player.position.x + moveAmount, 1.0f, SCREEN_WIDTH - 2.0f);
    }

    Point2D top = terrain->GetTerrainTop(static_cast<int>(std::round(player.position.x)));
    player.position.y = ClampFloat(top.y - 2.0f, 4.0f, SCREEN_HEIGHT - 2.0f);

    if (IsKeyJustPressed('Q')) {
        player.currentWeapon = PreviousWeapon(player.currentWeapon);
    }
    if (IsKeyJustPressed('E')) {
        player.currentWeapon = NextWeapon(player.currentWeapon);
    }

    if (leftMousePressed) {
        float dx = static_cast<float>(mousePosition.X) - player.position.x;
        float dy = player.position.y - static_cast<float>(mousePosition.Y);
        if (std::fabs(dx) > 0.1f || std::fabs(dy) > 0.1f) {
            float angle = std::atan2(dy, dx) / DEG_TO_RAD;
            player.angle = ClampFloat(angle, 0.0f, 180.0f);
        }
    }

    if (IsKeyJustPressed(VK_SPACE) || rightMousePressed) {
        float radians = player.angle * DEG_TO_RAD;
        Point2D muzzle(
            player.position.x + std::cos(radians) * 3.0f,
            player.position.y - std::sin(radians) * 3.0f
        );
        FireBullet(muzzle, player.angle, player.power, player.currentWeapon);
    }
}

void GameEngine::HandlePauseInput() {
    const int itemCount = 4;

    if (IsKeyJustPressed(VK_ESCAPE) || IsKeyJustPressed('P')) {
        ResumeGame();
        return;
    }

    if (IsKeyJustPressed(VK_UP)) {
        selectedMenuItem = WrapIndex(selectedMenuItem - 1, itemCount);
    }
    if (IsKeyJustPressed(VK_DOWN)) {
        selectedMenuItem = WrapIndex(selectedMenuItem + 1, itemCount);
    }

    if (IsKeyJustPressed(VK_RETURN)) {
        switch (selectedMenuItem) {
            case 0:
                ResumeGame();
                break;
            case 1:
                StartNewGame();
                break;
            case 2:
                selectedMenuItem = 0;
                SetGameState(GameState::SETTINGS);
                break;
            case 3:
                selectedMenuItem = 0;
                SetGameState(GameState::MENU);
                break;
        }
    }
}

void GameEngine::InitializePlayers() {
    players.clear();

    std::vector<Point2D> spawnPositions = terrain->GetSpawnPositions(numPlayers);
    for (int i = 0; i < numPlayers; ++i) {
        std::string name = "Player " + std::to_string(i + 1);
        AddPlayer(name, spawnPositions[i], PlayerColor(i));
        players.back().angle = (i == 0) ? 45.0f : 135.0f;
        players.back().power = 55.0f;
    }
}

void GameEngine::ResetGame() {
    terrain = std::make_unique<Terrain>(SCREEN_WIDTH, SCREEN_HEIGHT);

    int terrainType = std::rand() % 3;
    if (terrainType == 0) {
        terrain->GenerateHillTerrain();
    } else if (terrainType == 1) {
        terrain->GenerateMountainTerrain();
    } else {
        terrain->GenerateRandomTerrain();
    }

    bullets.clear();
    particles.clear();
    explosions.clear();
    currentPlayerIndex = 0;
    turnTimeLeft = maxTurnTime;
    InitializePlayers();
    UpdateWind();
}

void GameEngine::CheckPlayerCollisions() {
    if (!terrain) return;

    for (auto& player : players) {
        if (!player.isAlive) continue;

        player.position.x = ClampFloat(player.position.x, 1.0f, SCREEN_WIDTH - 2.0f);
        Point2D top = terrain->GetTerrainTop(static_cast<int>(std::round(player.position.x)));
        float targetY = top.y - 2.0f;

        if (targetY < 3.0f) {
            targetY = 3.0f;
        }
        if (targetY > SCREEN_HEIGHT - 2.0f) {
            player.takeDamage(player.maxHealth);
            continue;
        }

        if (player.position.y < targetY) {
            player.position.y = std::min(player.position.y + 0.45f, targetY);
        } else {
            player.position.y = targetY;
        }
    }
}

void GameEngine::UpdateWind() {
    windForce = static_cast<float>((std::rand() % 201) - 100) / 10.0f;
}

void GameEngine::UpdateTurnTimer(float deltaTime) {
    if (!bullets.empty() || players.empty()) return;

    turnTimeLeft -= deltaTime;
    if (turnTimeLeft <= 0.0f) {
        NextPlayer();
    }
}

void GameEngine::PlaySoundEffect(int frequency, int duration) {
    if (soundEnabled && console) {
        console->PlayBeep(frequency, duration);
    }
}

void GameEngine::DrawTrajectoryPreview() {
    if (players.empty()) return;

    const Player& player = players[currentPlayerIndex];
    float radians = player.angle * DEG_TO_RAD;
    Point2D position(
        player.position.x + std::cos(radians) * 3.0f,
        player.position.y - std::sin(radians) * 3.0f
    );
    Vector2D velocity = CalculateTrajectory(player.angle, player.power);

    for (int i = 0; i < 70; ++i) {
        ApplyWind(velocity);
        velocity.y += GRAVITY * 4.0f * 0.06f;
        position = position + velocity * 0.06f;

        if (position.x < 0 || position.x >= SCREEN_WIDTH || position.y >= SCREEN_HEIGHT) {
            break;
        }

        if (position.y >= 0) {
            console->SetPixel(
                static_cast<int>(std::round(position.x)),
                static_cast<int>(std::round(position.y)),
                '.',
                Color::LIGHT_GRAY
            );

            if (i > 3 && terrain && terrain->IsTerrainAt(position)) {
                break;
            }
        }
    }
}

void GameEngine::DrawCrosshair() {
    if (players.empty()) return;

    const Player& player = players[currentPlayerIndex];
    float radians = player.angle * DEG_TO_RAD;
    Point2D start = player.position;
    Point2D end(
        player.position.x + std::cos(radians) * 6.0f,
        player.position.y - std::sin(radians) * 6.0f
    );

    console->DrawLine(start, end, '-', Color::YELLOW);
    console->SetPixel(static_cast<int>(std::round(end.x)), static_cast<int>(std::round(end.y)), '+', Color::YELLOW);
}

void GameEngine::DrawWindIndicator() {
    int x = SCREEN_WIDTH / 2 - 10;
    int y = 2;
    std::string direction = windForce > 0.3f ? ">>>>" : (windForce < -0.3f ? "<<<<" : "----");
    Color color = std::fabs(windForce) < 3.0f ? Color::GREEN : (std::fabs(windForce) < 7.0f ? Color::YELLOW : Color::RED);
    console->DrawString(x, y, "Wind " + direction + " " + FormatFloat(std::fabs(windForce), 1), color);
}

void GameEngine::DrawMiniMap() {
    int x = SCREEN_WIDTH - 22;
    int y = SCREEN_HEIGHT - 5;
    int w = 20;
    int h = 4;

    console->DrawRect(x, y, w, h, '.', Color::DARK_GRAY);
    for (const auto& player : players) {
        if (!player.isAlive) continue;
        int px = x + 1 + static_cast<int>((player.position.x / SCREEN_WIDTH) * (w - 2));
        int py = y + 1 + static_cast<int>((player.position.y / SCREEN_HEIGHT) * (h - 2));
        console->SetPixel(px, py, player.symbol, player.color);
    }
}

bool GameEngine::IsKeyJustPressed(int key) {
    static bool previousStates[256] = {};
    if (key < 0 || key >= 256) return false;

    bool isDown = (GetAsyncKeyState(key) & 0x8000) != 0;
    bool justPressed = isDown && !previousStates[key];
    previousStates[key] = isDown;
    return justPressed;
}

void GameEngine::UpdateMouseInput() {
    if (!console) return;

    mousePosition = console->GetMousePosition();

    static bool previousLeft = false;
    static bool previousRight = false;
    bool leftDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    bool rightDown = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

    leftMousePressed = leftDown && !previousLeft;
    rightMousePressed = rightDown && !previousRight;
    previousLeft = leftDown;
    previousRight = rightDown;
}

float GameEngine::CalculateDamage(Point2D explosionCenter, Point2D targetPos, float explosionRadius) const {
    float distance = explosionCenter.distance(targetPos);
    if (distance > explosionRadius) return 0.0f;

    float falloff = 1.0f - (distance / explosionRadius);
    return std::max(0.15f, falloff);
}

void GameEngine::DamagePlayersInRadius(Point2D center, float radius, float baseDamage) {
    for (auto& player : players) {
        if (!player.isAlive) continue;

        float multiplier = CalculateDamage(center, player.position, radius);
        if (multiplier <= 0.0f) continue;

        float damage = baseDamage * multiplier;
        player.takeDamage(damage);
        AddParticles(player.position, player.isAlive ? 8 : 20, player.color);

        if (!player.isAlive) {
            PlaySoundEffect(90, 120);
        }
    }
}
