#include "enemy.h"
#include "platform.h"
#include "raymath.h"
#include <stddef.h>

#define FRAMES_PER_ROW        4
#define ROW_COUNT             13
#define WALK_DURATION         0.2f    // must match player WALK_DURATION for sync
#define ANIM_FRAME_DURATION   0.12f
#define ENEMY_SPRITE_SCALE    1.2f

#define ROW_IDLE_DOWN   0
#define ROW_IDLE_RIGHT  1
#define ROW_IDLE_UP     2
#define ROW_WALK_DOWN   3
#define ROW_WALK_RIGHT  4
#define ROW_WALK_UP     5

static Texture2D slimeTexture;
static float     frameW;
static float     frameH;

void InitEnemyResources(void) {
    slimeTexture = LoadTexture("assets/slime.png");
    frameW = (float)slimeTexture.width  / (float)FRAMES_PER_ROW;
    frameH = (float)slimeTexture.height / (float)ROW_COUNT;
}

void UnloadEnemyResources(void) {
    UnloadTexture(slimeTexture);
}

static Facing FacingFromDelta(int dx, int dz) {
    if (dz < 0) return FACING_UP;
    if (dz > 0) return FACING_DOWN;
    if (dx < 0) return FACING_LEFT;
    return FACING_RIGHT;
}

EnemyInstance CreateEnemyInstance(const Enemy *spawn) {
    EnemyInstance e = { 0 };
    e.gridX     = spawn->gridX;
    e.gridZ     = spawn->gridZ;
    e.prevGridX = spawn->gridX;
    e.prevGridZ = spawn->gridZ;
    e.spawnX    = spawn->gridX;
    e.spawnZ    = spawn->gridZ;
    e.facing    = FACING_DOWN;
    e.state     = ENEMY_IDLE;
    e.type      = spawn->type;
    return e;
}

void ResetEnemyInstance(EnemyInstance *e) {
    e->gridX        = e->spawnX;
    e->gridZ        = e->spawnZ;
    e->prevGridX    = e->spawnX;
    e->prevGridZ    = e->spawnZ;
    e->facing       = FACING_DOWN;
    e->state        = ENEMY_IDLE;
    e->moveProgress = 0.0f;
    e->animFrame    = 0;
    e->animTimer    = 0.0f;
}

void UpdateEnemyInstance(EnemyInstance *e, float dt) {
    e->animTimer += dt;
    if (e->animTimer >= ANIM_FRAME_DURATION) {
        e->animTimer -= ANIM_FRAME_DURATION;
        e->animFrame = (e->animFrame + 1) % FRAMES_PER_ROW;
    }
    if (e->state == ENEMY_WALKING) {
        e->moveProgress += dt / WALK_DURATION;
        if (e->moveProgress >= 1.0f) {
            e->moveProgress = 0.0f;
            e->prevGridX    = e->gridX;
            e->prevGridZ    = e->gridZ;
            e->state        = ENEMY_IDLE;
        }
    }
}

void StepEnemyAI(EnemyInstance *e, const Level *level,
                 const EnemyInstance *allEnemies, int enemyCount,
                 int dx, int dz) {
    if (e->type != ENEMY_TONGUE) return;
    if (e->state == ENEMY_WALKING) return;
    if (dx == 0 && dz == 0) return;

    e->facing = FacingFromDelta(dx, dz);

    int tx = e->gridX + dx;
    int tz = e->gridZ + dz;
    if (!IsWalkable(level, tx, tz)) return;

    // Don't pile onto another enemy's tile (by current position OR target).
    for (int i = 0; i < enemyCount; i++) {
        if (&allEnemies[i] == e) continue;
        if (allEnemies[i].gridX == tx && allEnemies[i].gridZ == tz) return;
    }

    e->prevGridX    = e->gridX;
    e->prevGridZ    = e->gridZ;
    e->gridX        = tx;
    e->gridZ        = tz;
    e->state        = ENEMY_WALKING;
    e->moveProgress = 0.0f;
}

static void PickRowAndFlip(EnemyState state, Facing facing, int *row, bool *flip) {
    *flip = false;
    bool walking = (state == ENEMY_WALKING);
    switch (facing) {
        case FACING_DOWN:
            *row = walking ? ROW_WALK_DOWN : ROW_IDLE_DOWN;
            break;
        case FACING_UP:
            *row = walking ? ROW_WALK_UP : ROW_IDLE_UP;
            break;
        case FACING_LEFT:
            *row  = walking ? ROW_WALK_RIGHT : ROW_IDLE_RIGHT;
            *flip = true;
            break;
        case FACING_RIGHT:
            *row = walking ? ROW_WALK_RIGHT : ROW_IDLE_RIGHT;
            break;
    }
}

void DrawEnemyInstance(const EnemyInstance *e, Camera3D camera) {
    int row; bool flip;
    PickRowAndFlip(e->state, e->facing, &row, &flip);

    Rectangle source = {
        e->animFrame * frameW,
        row * frameH,
        flip ? -frameW : frameW,
        frameH,
    };

    Vector3 prev = GridToWorld(e->prevGridX, e->prevGridZ);
    Vector3 curr = GridToWorld(e->gridX,     e->gridZ);
    Vector3 pos  = Vector3Lerp(prev, curr, e->moveProgress);
    pos.y = PLATFORM_HEIGHT * 0.5f;

    Vector2 size = {
        PLATFORM_SIZE * ENEMY_SPRITE_SCALE,
        PLATFORM_SIZE * ENEMY_SPRITE_SCALE * (frameH / frameW),
    };
    Vector3 up     = { 0.0f, 1.0f, 0.0f };
    Vector2 origin = { size.x * 0.5f, 0.0f };

    DrawBillboardPro(camera, slimeTexture, source, pos, up, size, origin, 0.0f, WHITE);
}
