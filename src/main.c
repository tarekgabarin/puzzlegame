#include "raylib.h"
#include "base_arena.h"
#include "test_level.h"

int main() {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(800, 600, "Engacho");

    Camera3D camera = CreateCamera();

    RunTestLevel(&camera);

    CloseWindow();
    return 0;
}
