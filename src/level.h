#ifndef LEVEL_H
#define LEVEL_H

#include <stdbool.h>
#include "platform.h"

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

    // Flat gridWidth*gridHeight array: static tile geometry for O(1) walkability.
    PlatformType *tileTypes;
} Level;

Level LoadLevel(const char *filename);
void  UnloadLevel(Level *level);
void  DrawLevel(Level *level);

bool         IsWalkable (const Level *level, int x, int z);
bool         HasEnemyAt (const Level *level, int x, int z);
const Enemy *GetEnemyAt (const Level *level, int x, int z);

#endif
