#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include <cmath>

// Định nghĩa màu sắc cho console
enum class Color {
    BLACK = 0,
    DARK_BLUE = 1,
    DARK_GREEN = 2,
    DARK_CYAN = 3,
    DARK_RED = 4,
    DARK_MAGENTA = 5,
    DARK_YELLOW = 6,
    LIGHT_GRAY = 7,
    DARK_GRAY = 8,
    BLUE = 9,
    GREEN = 10,
    CYAN = 11,
    RED = 12,
    MAGENTA = 13,
    YELLOW = 14,
    WHITE = 15
};

// Cấu trúc điểm 2D
struct Point2D {
    float x, y;
    Point2D(float x = 0, float y = 0) : x(x), y(y) {}
    Point2D operator+(const Point2D& other) const { return Point2D(x + other.x, y + other.y); }
    Point2D operator-(const Point2D& other) const { return Point2D(x - other.x, y - other.y); }
    Point2D operator*(float scalar) const { return Point2D(x * scalar, y * scalar); }
    // Add operator+ for Vector2D
    Point2D operator+(const struct Vector2D& vec) const;
    float distance(const Point2D& other) const {
        return sqrt((x - other.x) * (x - other.x) + (y - other.y) * (y - other.y));
    }
};

// Cấu trúc vector 2D cho vận tốc
struct Vector2D {
    float x, y;
    Vector2D(float x = 0, float y = 0) : x(x), y(y) {}
    Vector2D operator+(const Vector2D& other) const { return Vector2D(x + other.x, y + other.y); }
    Vector2D operator*(float scalar) const { return Vector2D(x * scalar, y * scalar); }
    float magnitude() const { return sqrt(x * x + y * y); }
    Vector2D normalize() const {
        float mag = magnitude();
        return mag > 0 ? Vector2D(x / mag, y / mag) : Vector2D(0, 0);
    }
};

// Loại đạn
enum class BulletType {
    NORMAL,
    EXPLOSIVE,
    TRIPLE_SHOT,
    LASER
};

// Cấu trúc đạn
struct Bullet {
    Point2D position;
    Point2D prevPosition;  // For trail effect
    Vector2D velocity;
    BulletType type;
    float damage;
    float explosionRadius;
    bool active;
    char symbol;
    Color color;
    float trailTimer;      // For animation

    Bullet(Point2D pos, Vector2D vel, BulletType t = BulletType::NORMAL)
        : position(pos), prevPosition(pos), velocity(vel), type(t), active(true), trailTimer(0.0f) {
        switch (type) {
            case BulletType::NORMAL:
                damage = 40.0f;        // Tăng từ 25 lên 40
                explosionRadius = 4.0f; // Tăng từ 2 lên 4
                symbol = '*';
                color = Color::YELLOW;
                break;
            case BulletType::EXPLOSIVE:
                damage = 75.0f;        // Tăng từ 50 lên 75
                explosionRadius = 8.0f; // Tăng từ 5 lên 8
                symbol = 'O';
                color = Color::RED;
                break;
            case BulletType::TRIPLE_SHOT:
                damage = 25.0f;        // Tăng từ 15 lên 25
                explosionRadius = 3.0f; // Tăng từ 1.5 lên 3
                symbol = '.';
                color = Color::CYAN;
                break;
            case BulletType::LASER:
                damage = 55.0f;        // Tăng từ 35 lên 55
                explosionRadius = 2.5f; // Tăng từ 1 lên 2.5
                symbol = '|';
                color = Color::MAGENTA;
                break;
        }
    }
};

// Cấu trúc người chơi
struct Player {
    Point2D position;
    float health;
    float maxHealth;
    float angle;
    float power;
    BulletType currentWeapon;
    bool isAlive;
    Color color;
    char symbol;
    std::string name;
    
    Player(Point2D pos, std::string playerName, Color playerColor) 
        : position(pos), health(100.0f), maxHealth(100.0f), angle(45.0f), 
          power(50.0f), currentWeapon(BulletType::NORMAL), isAlive(true),
          color(playerColor), symbol('@'), name(playerName) {}
    
    void takeDamage(float damage) {
        health -= damage;
        if (health <= 0) {
            health = 0;
            isAlive = false;
        }
    }
    
    void heal(float amount) {
        health = std::min(maxHealth, health + amount);
    }
};

// Cấu trúc hạt cho hiệu ứng
struct Particle {
    Point2D position;
    Vector2D velocity;
    Color color;
    char symbol;
    float life;
    float maxLife;
    bool active;
    
    Particle(Point2D pos, Vector2D vel, Color c, char s, float lifetime)
        : position(pos), velocity(vel), color(c), symbol(s), 
          life(lifetime), maxLife(lifetime), active(true) {}
    
    void update(float deltaTime) {
        if (!active) return;
        position = position + velocity * deltaTime;
        life -= deltaTime;
        if (life <= 0) active = false;
    }
};

// Game States
enum class GameState {
    MENU,
    TUTORIAL,
    PLAYING,
    PAUSED,
    GAME_OVER,
    SETTINGS
};

// Cấu trúc explosion
struct Explosion {
    Point2D center;
    float radius;
    float maxRadius;
    float life;
    float maxLife;
    bool active;
    
    Explosion(Point2D pos, float maxR) 
        : center(pos), radius(0), maxRadius(maxR), 
          life(1.0f), maxLife(1.0f), active(true) {}
    
    void update(float deltaTime) {
        if (!active) return;
        life -= deltaTime;
        radius = maxRadius * (1.0f - life / maxLife);
        if (life <= 0) active = false;
    }
};

// Constants
const int SCREEN_WIDTH = 120;
const int SCREEN_HEIGHT = 40;
const float GRAVITY = 9.8f;
const float PI = 3.14159265f;
const float DEG_TO_RAD = PI / 180.0f;

// Implementation of Point2D + Vector2D operator
inline Point2D Point2D::operator+(const Vector2D& vec) const {
    return Point2D(x + vec.x, y + vec.y);
}
