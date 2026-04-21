#include "raylib.h"
#include "platform.h"
#include "test_level.h"

void RunTestLevel(Camera3D *camera) {
    Platform normal = { .position = { -1.0f, 0.0f, 0.0f }, .type = PLATFORM_NORMAL };
    Platform exit   = { .position = {  1.0f, 0.0f, 0.0f }, .type = PLATFORM_EXIT   };

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
