#ifndef PLAYER_H
#define PLAYER_H

#include "raylib.h"
#include "level.h"

typedef enum { FACING_DOWN, FACING_LEFT, FACING_UP, FACING_RIGHT } Facing;
typedef enum { PLAYER_IDLE, PLAYER_WALKING, PLAYER_DYING } PlayerState;

typedef struct {
    int         gridX, gridZ;
    int         prevGridX, prevGridZ;
    Facing      facing;
    PlayerState state;
    float       moveProgress;   // 0..1 while walking
    int         animFrame;      // 0..5 — column of the sheet
    float       animTimer;
} Player;

void   InitPlayerResources(void);
void   UnloadPlayerResources(void);
Player CreatePlayer(int gridX, int gridZ);
void   UpdatePlayer(Player *player, Level *level, float dt);
void   DrawPlayer  (const Player *player, Camera3D camera);

#endif
