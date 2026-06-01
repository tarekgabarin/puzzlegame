#include "raylib.h"
#include "camera/base_arena.h"
#include "app/editor.h"
#include "game/enemy.h"
#include "app/menu.h"
#include "game/platform.h"
#include "game/player.h"
#include "app/level_runner.h"

int main() {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(800, 600, "Engacho");

    InitPlatformResources();
    InitPlayerResources();
    InitEnemyResources();

    Camera3D camera = CreateCamera();

    AppState state = APP_MENU;
    char chosenLevel[256] = "";

    while (!WindowShouldClose() && state != APP_QUIT) {
        switch (state) {
            case APP_MENU:
                state = RunMainMenu(chosenLevel, sizeof(chosenLevel));
                break;
            case APP_PLAY:
                RunLevel(&camera, chosenLevel);
                camera = CreateCamera();   // reset for next session
                state = APP_MENU;
                break;
            case APP_EDITOR:
                RunEditor();
                state = APP_MENU;
                break;
            default:
                break;
        }
    }

    UnloadEnemyResources();
    UnloadPlayerResources();
    UnloadPlatformResources();
    CloseWindow();
    return 0;
}
