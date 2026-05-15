#include "Terrain.h"
#include "ConsoleEngine.h"
#include <algorithm>
#include <cmath>

Terrain::Terrain(int w, int h) : width(w), height(h), rng(std::random_device{}()) {
    terrainMap.resize(height, std::vector<bool>(width, false));
    terrainColors.resize(height, std::vector<Color>(width, Color::BLACK));
}

void Terrain::GenerateRandomTerrain() {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    // Generate base terrain using noise
    for (int x = 0; x < width; ++x) {
        float noiseValue = 0;
        float amplitude = 1.0f;
        float frequency = 0.01f;
        
        // Multi-octave noise
        for (int octave = 0; octave < 4; ++octave) {
            noiseValue += amplitude * sin(x * frequency);
            amplitude *= 0.5f;
            frequency *= 2.0f;
        }
        
        int terrainHeight = height - static_cast<int>((noiseValue + 1.0f) * 0.25f * height + height * 0.3f);
        terrainHeight = std::clamp(terrainHeight, height / 4, height - 2);
        
        for (int y = terrainHeight; y < height; ++y) {
            terrainMap[y][x] = true;
            terrainColors[y][x] = GetTerrainColorByHeight(y);
        }
    }
    
    SmoothTerrain();
}

void Terrain::GenerateHillTerrain() {
    for (int x = 0; x < width; ++x) {
        float hillValue = sin(x * 0.05f) * 0.3f + sin(x * 0.02f) * 0.2f;
        int terrainHeight = height - static_cast<int>((hillValue + 0.5f) * height * 0.4f + height * 0.2f);
        terrainHeight = std::clamp(terrainHeight, height / 6, height - 2);
        
        for (int y = terrainHeight; y < height; ++y) {
            terrainMap[y][x] = true;
            terrainColors[y][x] = GetTerrainColorByHeight(y);
        }
    }
    
    SmoothTerrain();
}

void Terrain::GenerateMountainTerrain() {
    for (int x = 0; x < width; ++x) {
        float mountainValue = abs(sin(x * 0.03f)) * 0.6f + sin(x * 0.01f) * 0.3f;
        int terrainHeight = height - static_cast<int>(mountainValue * height * 0.7f + height * 0.1f);
        terrainHeight = std::clamp(terrainHeight, 2, height - 2);
        
        for (int y = terrainHeight; y < height; ++y) {
            terrainMap[y][x] = true;
            terrainColors[y][x] = GetTerrainColorByHeight(y);
        }
    }
    
    SmoothTerrain();
}

void Terrain::GenerateFlatTerrain() {
    int baseHeight = height - height / 3;
    
    for (int x = 0; x < width; ++x) {
        // Add small random variations
        std::uniform_int_distribution<int> variation(-2, 2);
        int terrainHeight = baseHeight + variation(rng);
        terrainHeight = std::clamp(terrainHeight, height / 2, height - 2);
        
        for (int y = terrainHeight; y < height; ++y) {
            terrainMap[y][x] = true;
            terrainColors[y][x] = GetTerrainColorByHeight(y);
        }
    }
}

void Terrain::CreateExplosion(Point2D center, float radius) {
    int cx = static_cast<int>(center.x);
    int cy = static_cast<int>(center.y);
    int r = static_cast<int>(radius);
    
    for (int y = cy - r; y <= cy + r; ++y) {
        for (int x = cx - r; x <= cx + r; ++x) {
            if (IsValidPosition(x, y)) {
                float distance = sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy));
                if (distance <= radius) {
                    terrainMap[y][x] = false;
                    terrainColors[y][x] = Color::BLACK;
                }
            }
        }
    }
    
    ApplyGravityToTerrain();
}

void Terrain::RemoveTerrainAt(int x, int y) {
    if (IsValidPosition(x, y)) {
        terrainMap[y][x] = false;
        terrainColors[y][x] = Color::BLACK;
    }
}

void Terrain::AddTerrainAt(int x, int y, Color color) {
    if (IsValidPosition(x, y)) {
        terrainMap[y][x] = true;
        terrainColors[y][x] = color;
    }
}

bool Terrain::IsTerrainAt(int x, int y) const {
    if (!IsValidPosition(x, y)) return false;
    return terrainMap[y][x];
}

bool Terrain::IsTerrainAt(Point2D point) const {
    return IsTerrainAt(static_cast<int>(point.x), static_cast<int>(point.y));
}

Point2D Terrain::GetTerrainTop(int x) const {
    if (x < 0 || x >= width) return Point2D(x, height);
    
    for (int y = 0; y < height; ++y) {
        if (terrainMap[y][x]) {
            return Point2D(x, y);
        }
    }
    return Point2D(x, height);
}

void Terrain::Render(ConsoleEngine& engine) const {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (terrainMap[y][x]) {
                char symbol = '#';

                // Add texture variation based on position
                if ((x + y) % 3 == 0) symbol = '%';
                else if ((x + y) % 5 == 0) symbol = '=';

                engine.SetPixel(x, y, symbol, terrainColors[y][x]);
            }
        }
    }
}

bool Terrain::IsValidPosition(int x, int y) const {
    return x >= 0 && x < width && y >= 0 && y < height;
}

Point2D Terrain::FindSafeSpawnPosition(int preferredX) const {
    int x = (preferredX >= 0 && preferredX < width) ? preferredX : width / 2;
    
    // Find the top of terrain at this x position
    Point2D terrainTop = GetTerrainTop(x);
    
    // Place player 2 units above terrain
    return Point2D(x, std::max(0.0f, terrainTop.y - 2.0f));
}

std::vector<Point2D> Terrain::GetSpawnPositions(int numPlayers) const {
    std::vector<Point2D> positions;
    
    if (numPlayers <= 0) return positions;
    
    int spacing = width / (numPlayers + 1);
    
    for (int i = 0; i < numPlayers; ++i) {
        int x = spacing * (i + 1);
        positions.push_back(FindSafeSpawnPosition(x));
    }
    
    return positions;
}

void Terrain::SmoothTerrain() {
    // Simple smoothing pass
    auto newTerrainMap = terrainMap;
    
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            int neighbors = 0;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (terrainMap[y + dy][x + dx]) neighbors++;
                }
            }
            
            // Apply smoothing rules
            if (neighbors >= 5) {
                newTerrainMap[y][x] = true;
            } else if (neighbors <= 3) {
                newTerrainMap[y][x] = false;
            }
        }
    }
    
    terrainMap = newTerrainMap;
    
    // Update colors for new terrain
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (terrainMap[y][x] && terrainColors[y][x] == Color::BLACK) {
                terrainColors[y][x] = GetTerrainColorByHeight(y);
            }
        }
    }
}

Color Terrain::GetTerrainColorByHeight(int y) const {
    float heightRatio = static_cast<float>(y) / height;
    
    if (heightRatio < 0.3f) return Color::WHITE;        // Snow peaks
    else if (heightRatio < 0.5f) return Color::DARK_GRAY;   // Rock
    else if (heightRatio < 0.7f) return Color::DARK_GREEN;  // Grass
    else if (heightRatio < 0.9f) return Color::DARK_YELLOW; // Sand
    else return Color::DARK_RED;                             // Underground
}

void Terrain::ApplyGravityToTerrain() {
    // Make terrain fall down due to gravity
    for (int x = 0; x < width; ++x) {
        for (int y = height - 2; y >= 0; --y) {
            if (terrainMap[y][x] && !terrainMap[y + 1][x]) {
                // Find how far this piece should fall
                int fallDistance = 0;
                for (int checkY = y + 1; checkY < height && !terrainMap[checkY][x]; ++checkY) {
                    fallDistance++;
                }
                
                if (fallDistance > 0) {
                    // Move terrain down
                    int newY = std::min(height - 1, y + fallDistance);
                    terrainMap[newY][x] = true;
                    terrainColors[newY][x] = terrainColors[y][x];
                    terrainMap[y][x] = false;
                    terrainColors[y][x] = Color::BLACK;
                }
            }
        }
    }
}

// TerrainGenerator implementations
void TerrainGenerator::GenerateTerrain(Terrain& terrain, TerrainType type) {
    switch (type) {
        case TerrainType::RANDOM:
            terrain.GenerateRandomTerrain();
            break;
        case TerrainType::HILLS:
            terrain.GenerateHillTerrain();
            break;
        case TerrainType::MOUNTAINS:
            terrain.GenerateMountainTerrain();
            break;
        case TerrainType::FLAT:
            terrain.GenerateFlatTerrain();
            break;
        default:
            terrain.GenerateHillTerrain();
            break;
    }
}
