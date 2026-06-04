#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

TARGET="gunny"
SOURCES=(
  main.cpp
  ConsoleEngine.cpp
  Terrain.cpp
  GameEngine.cpp
)

echo "========================================"
echo "   Building Gunny Game for Linux"
echo "========================================"

if ! command -v g++ >/dev/null 2>&1; then
  echo "Build failed: g++ is not installed or not in PATH."
  echo "Install it with your distro package manager, for example:"
  echo "  sudo apt install g++"
  exit 1
fi

echo "Cleaning previous build..."
rm -f "$TARGET" ./*.o

echo "Compiling..."
if g++ -std=c++17 -Wall -Wextra -O2 "${SOURCES[@]}" -o "$TARGET"; then
  echo
  echo "========================================"
  echo "   Build Successful!"
  echo "========================================"
  echo
  if [[ ! -t 0 ]]; then
    echo "Build finished. Run ./$TARGET from an interactive terminal to play."
    exit 0
  fi

  echo "Starting Gunny Game..."
  echo
  echo "Game Controls:"
  echo "- Menu: Arrow keys to navigate, Enter to select"
  echo "- Game: A/D for angle, W/S for power, Space to fire"
  echo "- Mouse: Click to aim, Right-click to fire"
  echo "- Q/E to change weapons, P to pause"
  echo
  read -r -p "Press Enter to start..."
  "./$TARGET"
  echo
  echo "Game ended."
else
  echo
  echo "========================================"
  echo "   Build Failed!"
  echo "========================================"
  echo
  echo "Please check the error messages above."
  exit 1
fi
