#include "raylib.h"
#include "base_arena.h"
#include "platform.h"
#include "test_level.h"

int main() {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(800, 600, "Engacho");

    InitPlatformResources();

    Camera3D camera = CreateCamera();
    RunTestLevel(&camera);

    UnloadPlatformResources();
    CloseWindow();
    return 0;
}
