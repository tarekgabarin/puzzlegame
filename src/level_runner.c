#include "raylib.h"
#include "base_arena.h"
#include "camera_intro.h"
#include "level.h"
#include "player.h"
#include "level_runner.h"

void RunLevel(Camera3D *camera, const char *levelFile) {
    Level       level  = LoadLevel(levelFile);
    Camera3D    fitted = ComputeFittedCamera(&level);
    CameraIntro intro  = CreateCameraIntro(fitted);

    Player player = CreatePlayer(level.playerStartX, level.playerStartZ);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        UpdateCameraIntro(&intro, camera, dt);
        if (CameraIntroAcceptsInput(&intro)) {
            UpdatePlayer(&player, &level, dt);
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);

            BeginMode3D(*camera);
                DrawLevel(&level);
                DrawPlayer(&player, *camera);
            EndMode3D();

            DrawCameraIntroSplash(&intro);
        EndDrawing();
    }

    UnloadLevel(&level);
}
