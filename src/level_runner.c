#include "raylib.h"
#include <stddef.h>
#include "base_arena.h"
#include "camera_intro.h"
#include "enemy.h"
#include "level.h"
#include "player.h"
#include "level_runner.h"

#define DEATH_DURATION   0.6f   // seconds the dying animation plays before reset

static void ResetLevel(Player *player, EnemyInstance *enemies, int enemyCount,
                       const Level *level) {
    *player = CreatePlayer(level->playerStartX, level->playerStartZ);
    for (int i = 0; i < enemyCount; i++) {
        ResetEnemyInstance(&enemies[i]);
    }
}

void RunLevel(Camera3D *camera, const char *levelFile) {
    Level       level  = LoadLevel(levelFile);
    Camera3D    fitted = ComputeFittedCamera(&level);
    CameraIntro intro  = CreateCameraIntro(fitted);

    Player player = CreatePlayer(level.playerStartX, level.playerStartZ);

    EnemyInstance *enemies = NULL;
    if (level.enemyCount > 0) {
        enemies = (EnemyInstance *)MemAlloc(level.enemyCount * sizeof(EnemyInstance));
        for (int i = 0; i < level.enemyCount; i++) {
            enemies[i] = CreateEnemyInstance(&level.enemies[i]);
        }
    }

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        UpdateCameraIntro(&intro, camera, dt);

        if (CameraIntroAcceptsInput(&intro)) {
            // Advance existing slides / anims FIRST so a newly-accepted move
            // starts at moveProgress=0 and both player and enemies first
            // advance together on the NEXT frame (keeps them phase-locked).
            UpdatePlayer(&player, &level, dt);
            for (int i = 0; i < level.enemyCount; i++) {
                UpdateEnemyInstance(&enemies[i], dt);
            }

            if (player.justMoved) {
                for (int i = 0; i < level.enemyCount; i++) {
                    StepEnemyAI(&enemies[i], &level,
                                enemies, level.enemyCount,
                                player.lastMoveDx, player.lastMoveDz);
                }
                // Post-step collision — either the player slid onto an enemy's
                // tile, or an enemy slid onto the player's tile.
                for (int i = 0; i < level.enemyCount; i++) {
                    if (enemies[i].gridX == player.gridX &&
                        enemies[i].gridZ == player.gridZ) {
                        player.state      = PLAYER_DYING;
                        player.deathTimer = 0.0f;
                        break;
                    }
                }
                player.justMoved = false;
            }

            if (player.state == PLAYER_DYING && player.deathTimer >= DEATH_DURATION) {
                ResetLevel(&player, enemies, level.enemyCount, &level);
            }
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);

            BeginMode3D(*camera);
                DrawLevel(&level);
                for (int i = 0; i < level.enemyCount; i++) {
                    DrawEnemyInstance(&enemies[i], *camera);
                }
                DrawPlayer(&player, *camera);
            EndMode3D();

            DrawCameraIntroSplash(&intro);
        EndDrawing();
    }

    if (enemies != NULL) MemFree(enemies);
    UnloadLevel(&level);
}
