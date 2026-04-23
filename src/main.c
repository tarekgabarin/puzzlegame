#include "raylib.h"
#include "base_arena.h"
#include "platform.h"
#include "player.h"
#include "level_runner.h"

int main() {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(800, 600, "Engacho");

    InitPlatformResources();
    InitPlayerResources();

    Camera3D camera = CreateCamera();

    // TODO: stop hardcoding this once we track which level the player is on
    // (progress save, main-menu level select, etc.)
    RunLevel(&camera, "levels/level_weird.txt");

    UnloadPlayerResources();
    UnloadPlatformResources();
    CloseWindow();
    return 0;
}
