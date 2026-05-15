#pragma once
#include "GameStructures.h"
#include <vector>
#include <random>

class Terrain {
private:
    std::vector<std::vector<bool>> terrainMap;
    std::vector<std::vector<Color>> terrainColors;
    int width, height;
    std::mt19937 rng;
    
public:
    Terrain(int w, int h);
    
    // Terrain generation
    void GenerateRandomTerrain();
    void GenerateHillTerrain();
    void GenerateMountainTerrain();
    void GenerateFlatTerrain();
    
    // Terrain modification
    void CreateExplosion(Point2D center, float radius);
    void RemoveTerrainAt(int x, int y);
    void AddTerrainAt(int x, int y, Color color = Color::DARK_GREEN);
    
    // Collision detection
    bool IsTerrainAt(int x, int y) const;
    bool IsTerrainAt(Point2D point) const;
    Point2D GetTerrainTop(int x) const;
    
    // Rendering
    void Render(class ConsoleEngine& engine) const;
    
    // Utility
    bool IsValidPosition(int x, int y) const;
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }
    
    // Player positioning
    Point2D FindSafeSpawnPosition(int preferredX = -1) const;
    std::vector<Point2D> GetSpawnPositions(int numPlayers) const;
    
private:
    void SmoothTerrain();
    Color GetTerrainColorByHeight(int y) const;
    void ApplyGravityToTerrain();
};

// Terrain patterns for different map types
enum class TerrainType {
    RANDOM,
    HILLS,
    MOUNTAINS,
    FLAT,
    CANYON,
    ISLANDS
};

class TerrainGenerator {
public:
    static void GenerateTerrain(Terrain& terrain, TerrainType type);
    
private:
    static void GeneratePerlinNoise(std::vector<float>& heights, int width, float scale, int octaves);
    static float PerlinNoise(float x, float scale);
    static float Interpolate(float a, float b, float t);
    static float Fade(float t);
};
