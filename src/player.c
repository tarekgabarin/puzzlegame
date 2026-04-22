#include "player.h"
#include "platform.h"
#include "raymath.h"
#include <stddef.h>

#define FRAMES_PER_ROW        6
#define ROW_COUNT             10
#define WALK_DURATION         0.2f    // seconds to slide one tile
#define ANIM_FRAME_DURATION   0.12f   // seconds per animation frame
#define PLAYER_SPRITE_SCALE   1.6f    // billboard width in tiles (1.0 = exactly one tile)

// Row indices in the sprite sheet.
#define ROW_IDLE_DOWN   0
#define ROW_IDLE_LEFT   1
#define ROW_IDLE_UP     2
#define ROW_WALK_DOWN   3
#define ROW_WALK_RIGHT  4
#define ROW_WALK_UP     5
#define ROW_DYING       9

static Texture2D playerTexture;
static float     frameW;    // pixel width of one frame
static float     frameH;    // pixel height of one frame

void InitPlayerResources(void) {
    playerTexture = LoadTexture("assets/player.png");
    frameW = (float)playerTexture.width  / (float)FRAMES_PER_ROW;
    frameH = (float)playerTexture.height / (float)ROW_COUNT;
}

void UnloadPlayerResources(void) {
    UnloadTexture(playerTexture);
}

Player CreatePlayer(int gridX, int gridZ) {
    Player p = { 0 };
    p.gridX     = gridX;
    p.gridZ     = gridZ;
    p.prevGridX = gridX;
    p.prevGridZ = gridZ;
    p.facing    = FACING_DOWN;
    p.state     = PLAYER_IDLE;
    return p;
}

// Pick the target tile and new facing from arrow-key input. Returns true if
// any directional key was pressed (i.e. facing changed).
static bool ReadInput(Facing *outFacing, int *dx, int *dz) {
    *dx = 0;
    *dz = 0;
    if (IsKeyPressed(KEY_UP))    { *outFacing = FACING_UP;    *dz = -1; return true; }
    if (IsKeyPressed(KEY_DOWN))  { *outFacing = FACING_DOWN;  *dz =  1; return true; }
    if (IsKeyPressed(KEY_LEFT))  { *outFacing = FACING_LEFT;  *dx = -1; return true; }
    if (IsKeyPressed(KEY_RIGHT)) { *outFacing = FACING_RIGHT; *dx =  1; return true; }
    return false;
}

void UpdatePlayer(Player *p, Level *level, float dt) {
    // Animation tick runs regardless of state.
    p->animTimer += dt;
    if (p->animTimer >= ANIM_FRAME_DURATION) {
        p->animTimer -= ANIM_FRAME_DURATION;
        p->animFrame = (p->animFrame + 1) % FRAMES_PER_ROW;
    }

    if (p->state == PLAYER_WALKING) {
        p->moveProgress += dt / WALK_DURATION;
        if (p->moveProgress >= 1.0f) {
            p->moveProgress = 0.0f;
            p->prevGridX    = p->gridX;
            p->prevGridZ    = p->gridZ;
            p->state        = PLAYER_IDLE;
        }
        return;   // ignore input mid-slide
    }

    if (p->state == PLAYER_DYING) return;

    Facing newFacing;
    int dx, dz;
    if (!ReadInput(&newFacing, &dx, &dz)) return;

    p->facing = newFacing;

    int tx = p->gridX + dx;
    int tz = p->gridZ + dz;
    if (!IsWalkable(level, tx, tz)) return;
    if (HasEnemyAt(level, tx, tz)) {
        // For now: treat as blocked. Future pass: allow stepping onto the enemy
        // and trigger the dying state.
        return;
    }

    p->prevGridX    = p->gridX;
    p->prevGridZ    = p->gridZ;
    p->gridX        = tx;
    p->gridZ        = tz;
    p->state        = PLAYER_WALKING;
    p->moveProgress = 0.0f;

    const Enemy *e = GetEnemyAt(level, p->gridX, p->gridZ);
    if (e != NULL) {
        TraceLog(LOG_INFO, "Player collided with enemy (type %d)", e->type);
    }
}

// Map (state, facing) to the sheet row + horizontal-flip flag.
static void PickRowAndFlip(PlayerState state, Facing facing, int *row, bool *flip) {
    *flip = false;
    if (state == PLAYER_DYING) { *row = ROW_DYING; return; }

    bool walking = (state == PLAYER_WALKING);
    switch (facing) {
        case FACING_DOWN:  *row = walking ? ROW_WALK_DOWN  : ROW_IDLE_DOWN; break;
        case FACING_UP:    *row = walking ? ROW_WALK_UP    : ROW_IDLE_UP;   break;
        case FACING_LEFT:
            if (walking) { *row = ROW_WALK_RIGHT; *flip = true; }
            else         { *row = ROW_IDLE_LEFT; }
            break;
        case FACING_RIGHT:
            if (walking) { *row = ROW_WALK_RIGHT; }
            else         { *row = ROW_IDLE_LEFT;  *flip = true; }
            break;
    }
}

void DrawPlayer(const Player *p, Camera3D camera) {
    int row; bool flip;
    PickRowAndFlip(p->state, p->facing, &row, &flip);

    Rectangle source = {
        p->animFrame * frameW,
        row * frameH,
        flip ? -frameW : frameW,
        frameH,
    };

    Vector3 prev = GridToWorld(p->prevGridX, p->prevGridZ);
    Vector3 curr = GridToWorld(p->gridX,     p->gridZ);
    Vector3 pos  = Vector3Lerp(prev, curr, p->moveProgress);
    pos.y = PLATFORM_HEIGHT * 0.5f;   // platform top surface

    Vector2 size = {
        PLATFORM_SIZE * PLAYER_SPRITE_SCALE,
        PLATFORM_SIZE * PLAYER_SPRITE_SCALE * (frameH / frameW),
    };
    Vector3 up     = { 0.0f, 1.0f, 0.0f };
    Vector2 origin = { size.x * 0.5f, 0.0f };   // bottom-center — feet rest on `pos`

    DrawBillboardPro(camera, playerTexture, source, pos, up, size, origin, 0.0f, WHITE);
}
