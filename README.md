# Gunny Game - Terminal Edition

Một game artillery (pháo binh) được viết bằng C++ có thể chạy trên Windows Terminal với đồ họa màu sắc và hiệu ứng mượt mà.

## Tính năng

### 🎮 Gameplay
- **Multiplayer**: Hỗ trợ nhiều người chơi (mặc định 2 người)
- **Physics thực tế**: Đạn bay theo quỹ đạo parabol với trọng lực
- **Hệ thống gió**: Gió thay đổi ngẫu nhiên ảnh hưởng đến quỹ đạo đạn
- **Địa hình phá hủy được**: Đạn tạo ra hố nổ trên địa hình
- **Nhiều loại vũ khí**: Normal, Explosive, Triple Shot, Laser
- **Player movement**: Di chuyển trên địa hình với physics tự nhiên
- **Turn timer**: Thời gian giới hạn cho mỗi lượt (30 giây)
- **Enhanced shooting**: Lực bắn mạnh hơn, đạn bay xa hơn

### 🎨 Đồ họa
- **Màu sắc đa dạng**: 16 màu console với hiệu ứng visual
- **Animation mượt mà**: Particle effects, explosion effects, bullet trails
- **Terrain đẹp mắt**: Địa hình với nhiều màu sắc theo độ cao
- **UI trực quan**: Health bars, trajectory preview, wind indicator, turn timer
- **Enhanced bullets**: Đạn có trail effect và glow, dễ nhìn thấy hơn
- **Improved HUD**: Hiển thị thông tin cả 2 player rõ ràng

### 🎯 Điều khiển
- **Keyboard**: Điều khiển góc bắn, sức mạnh, chọn vũ khí, di chuyển
- **Mouse**: Click để ngắm bắn, right-click để bắn
- **Menu navigation**: Arrow keys và Enter
- **Player movement**: Arrow keys để di chuyển trên địa hình
- **Turn-based**: Mỗi player có 30 giây để thực hiện lượt

## Cài đặt và Chạy

### Yêu cầu hệ thống
- Windows 10/11
- MinGW-w64 hoặc Visual Studio với C++17 support
- Windows Terminal (khuyến nghị) hoặc Command Prompt

### Build từ source

1. **Clone hoặc download source code**
```bash
# Nếu có git
git clone <repository-url>
cd gunny

# Hoặc extract từ zip file
```

2. **Build với MinGW**
```bash
# Sử dụng Makefile
make

# Hoặc build thủ công
g++ -std=c++17 -Wall -Wextra -O2 main.cpp ConsoleEngine.cpp Terrain.cpp GameEngine.cpp -o gunny.exe
```

3. **Build với Visual Studio**
```bash
# Mở Developer Command Prompt
cl /EHsc /std:c++17 main.cpp ConsoleEngine.cpp Terrain.cpp GameEngine.cpp /Fe:gunny.exe
```

4. **Chạy game**
```bash
./gunny.exe
```

## Hướng dẫn chơi

### Menu chính
- **↑/↓**: Di chuyển trong menu
- **Enter**: Chọn
- **ESC**: Thoát

### Trong game
- **A/D**: Điều chỉnh góc bắn (0° - 180°)
- **W/S**: Điều chỉnh sức mạnh (0% - 100%)
- **Q/E**: Thay đổi loại vũ khí
- **Space**: Bắn
- **Mouse**: Click để ngắm, Right-click để bắn
- **P**: Pause game
- **ESC**: Pause menu

### Loại vũ khí
1. **Normal** (*): Đạn thường, damage 25, explosion radius 2
2. **Explosive** (O): Đạn nổ, damage 50, explosion radius 5  
3. **Triple Shot** (.): Bắn 3 viên cùng lúc, damage 15 mỗi viên
4. **Laser** (|): Đạn laser, damage 35, explosion radius 1

### Chiến thuật
- **Quan sát gió**: Mũi tên chỉ hướng và sức mạnh gió
- **Sử dụng trajectory preview**: Đường chấm cho thấy quỹ đạo dự kiến
- **Địa hình**: Sử dụng hố nổ để che chắn hoặc tạo đường đi
- **Timing**: Gió thay đổi mỗi 5 giây

## Cấu trúc code

### Files chính
- `main.cpp`: Entry point của game
- `GameEngine.h/cpp`: Game logic chính, state management
- `ConsoleEngine.h/cpp`: Graphics engine cho console
- `Terrain.h/cpp`: Hệ thống địa hình và map generation
- `GameStructures.h`: Định nghĩa các struct và constants

### Architecture
```
GameEngine
├── ConsoleEngine (Graphics & Input)
├── Terrain (Map & Collision)
├── Players (Game entities)
├── Bullets (Projectiles)
├── Particles (Effects)
└── Game States (Menu, Playing, Paused, etc.)
```

## Tùy chỉnh

### Thay đổi cài đặt game
Trong `GameStructures.h`:
```cpp
const int SCREEN_WIDTH = 120;    // Độ rộng màn hình
const int SCREEN_HEIGHT = 40;    // Độ cao màn hình
const float GRAVITY = 9.8f;      // Trọng lực
```

### Thêm loại vũ khí mới
1. Thêm vào enum `BulletType` trong `GameStructures.h`
2. Cập nhật constructor của `Bullet` struct
3. Thêm logic xử lý trong `GameEngine::HandleBulletCollision`

### Thêm terrain mới
1. Thêm vào enum `TerrainType` trong `Terrain.h`
2. Implement generation function trong `Terrain.cpp`
3. Cập nhật `TerrainGenerator::GenerateTerrain`

## Troubleshooting

### Game không hiển thị màu
- Sử dụng Windows Terminal thay vì Command Prompt cũ
- Đảm bảo terminal hỗ trợ ANSI colors

### Input không hoạt động
- Chạy với quyền administrator nếu cần
- Đảm bảo không có phần mềm nào khác capture input

### Performance issues
- Giảm `SCREEN_WIDTH` và `SCREEN_HEIGHT`
- Build với optimization flags (`-O2`)

## Credits

Game được phát triển bằng C++ sử dụng Windows Console API.
Inspired by classic artillery games như Worms và Gunbound.

## License

MIT License - Xem file LICENSE để biết thêm chi tiết.
