# Makefile for Gunny Game

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDFLAGS =

TARGET = gunny.exe
SOURCES = main.cpp ConsoleEngine.cpp Terrain.cpp GameEngine.cpp
OBJECTS = $(SOURCES:.cpp=.o)
HEADERS = GameStructures.h ConsoleEngine.h Terrain.h GameEngine.h

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete! Run with: ./$(TARGET)"

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	del /Q *.o $(TARGET) 2>nul || true

rebuild: clean all

install:
	@echo "No external dependencies required for this project"

run: $(TARGET)
	./$(TARGET)

debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET)

release: CXXFLAGS += -DNDEBUG
release: $(TARGET)

help:
	@echo "Available targets:"
	@echo "  all      - Build the game (default)"
	@echo "  clean    - Remove build files"
	@echo "  rebuild  - Clean and build"
	@echo "  run      - Build and run the game"
	@echo "  debug    - Build with debug symbols"
	@echo "  release  - Build optimized release version"
	@echo "  help     - Show this help message"

.PHONY: all clean rebuild install run debug release help
