#include "raylib.h"
#include "platform.h"
#include "test_level.h"

void RunTestLevel(Camera3D *camera) {
    Platform normal = { .gridX = 0, .gridZ = 0, .type = PLATFORM_NORMAL };
    Platform exit   = { .gridX = 1, .gridZ = 0, .type = PLATFORM_EXIT   };

    while (!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(RAYWHITE);

            BeginMode3D(*camera);
                DrawPlatform(&normal);
                DrawPlatform(&exit);
            EndMode3D();

        EndDrawing();
    }
}
