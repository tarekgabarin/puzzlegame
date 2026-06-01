#include "raylib.h"
#include <stddef.h>
#include "camera/base_arena.h"
#include "camera/camera_intro.h"
#include "game/enemy.h"
#include "game/level.h"
#include "game/player.h"
#include "app/level_runner.h"

#define DEATH_DURATION   0.6f   // seconds the dying animation plays before reset

// Outcome of the level, driving the freeze + overlay once the player wins or
// runs out of moves.
typedef enum { RUN_PLAYING, RUN_WON, RUN_LOST } RunState;

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

    int      moveCount = 0;
    RunState runState  = RUN_PLAYING;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        UpdateCameraIntro(&intro, camera, dt);

        if (CameraIntroAcceptsInput(&intro) && runState == RUN_PLAYING) {
            // Advance existing slides / anims FIRST so a newly-accepted move
            // starts at moveProgress=0 and both player and enemies first
            // advance together on the NEXT frame (keeps them phase-locked).
            UpdatePlayer(&player, &level, dt);
            for (int i = 0; i < level.enemyCount; i++) {
                UpdateEnemyInstance(&enemies[i], dt);
            }

            if (player.justMoved) {
                moveCount++;
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
                moveCount = 0;
            }

            // Win/lose are evaluated only once the player is back at rest, so the
            // final move's slide finishes (player visibly arrives on the exit)
            // before the scene freezes. A dying player is never IDLE, so an
            // enemy collision takes priority over a win on the same tile.
            if (player.state == PLAYER_IDLE) {
                if (IsExitTile(&level, player.gridX, player.gridZ)) {
                    runState = RUN_WON;
                } else if (moveCount >= level.moveLimit) {
                    runState = RUN_LOST;
                }
            }
        } else if (runState != RUN_PLAYING) {
            // Frozen on a win/lose overlay — wait for the player to dismiss it.
            if (IsKeyPressed(KEY_ENTER)) {
                if (runState == RUN_WON) break;   // back to the menu
                ResetLevel(&player, enemies, level.enemyCount, &level);
                moveCount = 0;
                runState  = RUN_PLAYING;
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

            // HUD: move counter, always visible.
            DrawText(TextFormat("Moves: %d/%d", moveCount, level.moveLimit),
                     20, 20, 28, DARKGRAY);

            // Win/lose overlay.
            if (runState != RUN_PLAYING) {
                int sw = GetScreenWidth();
                int sh = GetScreenHeight();

                const char *title = (runState == RUN_WON) ? "You Win!" : "Out of Moves!";
                const char *hint  = (runState == RUN_WON) ? "Press Enter"
                                                          : "Press Enter to retry";
                Color titleColor  = (runState == RUN_WON) ? DARKGREEN : MAROON;

                DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.5f));

                int titleSize = 64;
                int titleW    = MeasureText(title, titleSize);
                DrawText(title, (sw - titleW) / 2, sh / 2 - titleSize, titleSize, titleColor);

                int hintSize = 24;
                int hintW    = MeasureText(hint, hintSize);
                DrawText(hint, (sw - hintW) / 2, sh / 2 + 16, hintSize, RAYWHITE);
            }
        EndDrawing();
    }

    if (enemies != NULL) MemFree(enemies);
    UnloadLevel(&level);
}
