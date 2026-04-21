#include "raylib.h"
#include "level.h"
#include "level_runner.h"

void RunLevel(Camera3D *camera, const char *levelFile) {
    Level level = LoadLevel(levelFile);

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
