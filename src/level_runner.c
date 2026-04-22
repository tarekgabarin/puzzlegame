#include "raylib.h"
#include "base_arena.h"
#include "level.h"
#include "player.h"
#include "level_runner.h"

void RunLevel(Camera3D *camera, const char *levelFile) {
    Level  level  = LoadLevel(levelFile);
    FitCameraToLevel(camera, level.gridWidth, level.gridHeight);

    Player player = CreatePlayer(level.playerStartX, level.playerStartZ);
    // LoadLevel already marked the player's spawn tile as present.

    while (!WindowShouldClose()) {
        UpdatePlayer(&player, &level, GetFrameTime());

        BeginDrawing();
            ClearBackground(RAYWHITE);

            BeginMode3D(*camera);
                DrawLevel(&level);
                DrawPlayer(&player, *camera);
            EndMode3D();

        EndDrawing();
    }

    UnloadLevel(&level);
}
