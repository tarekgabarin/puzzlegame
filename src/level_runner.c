#include "raylib.h"
#include "base_arena.h"
#include "level.h"
#include "level_runner.h"

void RunLevel(Camera3D *camera, const char *levelFile) {
    Level level = LoadLevel(levelFile);
    FitCameraToLevel(camera, level.gridWidth, level.gridHeight);

    while (!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(RAYWHITE);

            BeginMode3D(*camera);
                DrawLevel(&level);
            EndMode3D();

        EndDrawing();
    }

    UnloadLevel(&level);
}
