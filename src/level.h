#ifndef LEVEL_H
#define LEVEL_H

#include <stdbool.h>
#include "platform.h"
#include "entity_map.h"

typedef enum {
    ENEMY_TONGUE,       // '1'
    ENEMY_WINGED_BUTT,  // '2'
    ENEMY_SNOTTY,       // '3'
    ENEMY_ARMPITS,      // '4'
} EnemyType;

typedef struct {
    int       gridX;
    int       gridZ;
    EnemyType type;
    char      name[ENTITY_NAME_LEN];   // "enemy_1", "enemy_2", ... — matches EntityMap key
} Enemy;

typedef struct {
    Platform *platforms;
    int       platformCount;
    int       gridWidth;
    int       gridHeight;

    int       playerStartX;
    int       playerStartZ;

    Enemy    *enemies;
    int       enemyCount;

    EntityMap entities;   // "player", "enemy_1", "enemy_2", ...
} Level;

Level LoadLevel(const char *filename);
void  UnloadLevel(Level *level);
void  DrawLevel(Level *level);

// Occupancy / walkability helpers (thin wrappers over EntityMap).
bool         IsWalkable     (const Level *level, int x, int z);
bool         HasPlayerAt    (const Level *level, int x, int z);
bool         HasEnemyAt     (const Level *level, int x, int z);
const char  *GetEnemyNameAt (const Level *level, int x, int z);
const Enemy *GetEnemyAt     (const Level *level, int x, int z);
void         SetPlayerAt    (Level *level, int x, int z, bool present);

#endif
