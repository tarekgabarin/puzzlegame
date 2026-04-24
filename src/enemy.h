#ifndef ENEMY_H
#define ENEMY_H

#include "raylib.h"
#include <stdbool.h>
#include "level.h"
#include "player.h"   // for Facing

typedef enum { ENEMY_IDLE, ENEMY_WALKING } EnemyState;

typedef struct {
    int         gridX, gridZ;
    int         prevGridX, prevGridZ;
    int         spawnX, spawnZ;
    Facing      facing;
    EnemyState  state;
    float       moveProgress;
    int         animFrame;
    float       animTimer;
    EnemyType   type;
} EnemyInstance;

void          InitEnemyResources(void);
void          UnloadEnemyResources(void);

EnemyInstance CreateEnemyInstance(const Enemy *spawn);
void          ResetEnemyInstance(EnemyInstance *e);

// Advances the slide and animation timers. Does not pick new moves.
void          UpdateEnemyInstance(EnemyInstance *e, float dt);

// Called once per accepted player move. (dx, dz) is the player's step delta.
// Tongue copies it; other types are inert this pass.
void          StepEnemyAI(EnemyInstance *e, const Level *level,
                          const EnemyInstance *allEnemies, int enemyCount,
                          int dx, int dz);

void          DrawEnemyInstance(const EnemyInstance *e, Camera3D camera);

#endif
