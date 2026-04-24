#include "enemy.h"
#include "platform.h"
#include "raymath.h"

#define WALK_DURATION       0.2f    // must match player WALK_DURATION for sync
#define BODY_SIZE           0.7f    // cube edge length (in tile units)
#define NOSE_SIZE           0.3f    // protrusion edge length, as fraction of BODY_SIZE

static const Color ENEMY_BODY = { 220, 20, 60, 255 };   // crimson

void InitEnemyResources(void)   { /* no texture; polygonal enemy */ }
void UnloadEnemyResources(void) { /* no texture; polygonal enemy */ }

static Facing FacingFromDelta(int dx, int dz) {
    if (dz < 0) return FACING_UP;
    if (dz > 0) return FACING_DOWN;
    if (dx < 0) return FACING_LEFT;
    return FACING_RIGHT;
}

// Unit vector in world space for the direction the enemy is facing.
static Vector3 FaceDir(Facing f) {
    switch (f) {
        case FACING_UP:    return (Vector3){  0.0f, 0.0f, -1.0f };
        case FACING_DOWN:  return (Vector3){  0.0f, 0.0f,  1.0f };
        case FACING_LEFT:  return (Vector3){ -1.0f, 0.0f,  0.0f };
        case FACING_RIGHT: return (Vector3){  1.0f, 0.0f,  0.0f };
    }
    return (Vector3){ 0.0f, 0.0f, 1.0f };
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
}

void UpdateEnemyInstance(EnemyInstance *e, float dt) {
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

void DrawEnemyInstance(const EnemyInstance *e, Camera3D camera) {
    (void)camera;   // polygonal enemy — no billboard needs the camera

    Vector3 prev = GridToWorld(e->prevGridX, e->prevGridZ);
    Vector3 curr = GridToWorld(e->gridX,     e->gridZ);
    Vector3 pos  = Vector3Lerp(prev, curr, e->moveProgress);

    // Body sits on platform top surface.
    Vector3 bodyCenter = {
        pos.x,
        PLATFORM_HEIGHT * 0.5f + BODY_SIZE * 0.5f,
        pos.z,
    };
    DrawCube     (bodyCenter, BODY_SIZE, BODY_SIZE, BODY_SIZE, ENEMY_BODY);
    DrawCubeWires(bodyCenter, BODY_SIZE, BODY_SIZE, BODY_SIZE, BLACK);

    // Small protruding nose indicating facing direction. Centered vertically
    // on the body, flush with the front face.
    float nose = BODY_SIZE * NOSE_SIZE;
    Vector3 dir = FaceDir(e->facing);
    Vector3 noseCenter = {
        bodyCenter.x + dir.x * (BODY_SIZE * 0.5f + nose * 0.5f),
        bodyCenter.y,
        bodyCenter.z + dir.z * (BODY_SIZE * 0.5f + nose * 0.5f),
    };
    DrawCube     (noseCenter, nose, nose, nose, ENEMY_BODY);
    DrawCubeWires(noseCenter, nose, nose, nose, BLACK);
}
