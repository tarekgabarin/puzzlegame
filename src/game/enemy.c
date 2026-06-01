#include "game/enemy.h"
#include "game/platform.h"
#include "raymath.h"
#include "rlgl.h"

#define WALK_DURATION   0.2f    // must match player WALK_DURATION for sync
#define CUBE_SIZE       1.0f    // fills a tile so each roll lands flush

static const Color ENEMY_BODY = { 220, 20, 60, 255 };   // crimson

void InitEnemyResources(void)   {}
void UnloadEnemyResources(void) {}

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

    // Tongue moves INVERSE to the player's input.
    int idx = -dx;
    int idz = -dz;

    e->facing = FacingFromDelta(idx, idz);

    int tx = e->gridX + idx;
    int tz = e->gridZ + idz;
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
    (void)camera;

    float cubeRestY = PLATFORM_HEIGHT * 0.5f + CUBE_SIZE * 0.5f;

    if (e->state != ENEMY_WALKING) {
        // Idle: sit flat on the tile.
        Vector3 pos = GridToWorld(e->gridX, e->gridZ);
        pos.y = cubeRestY;
        DrawCube     (pos, CUBE_SIZE, CUBE_SIZE, CUBE_SIZE, ENEMY_BODY);
        DrawCubeWires(pos, CUBE_SIZE, CUBE_SIZE, CUBE_SIZE, BLACK);
        return;
    }

    // Rolling: pivot around the leading bottom edge.
    //
    // Pivot is the world-space line where the cube's leading bottom face meets
    // the ground, halfway between the start tile and the destination tile.
    // We rotate the cube around this line by `progress * 90°` so it tumbles
    // forward exactly one tile per step.
    Vector3 start = GridToWorld(e->prevGridX, e->prevGridZ);
    int dx = e->gridX - e->prevGridX;
    int dz = e->gridZ - e->prevGridZ;

    Vector3 pivot = {
        start.x + (float)dx * (PLATFORM_SIZE * 0.5f),
        PLATFORM_HEIGHT * 0.5f,                          // platform top = cube bottom
        start.z + (float)dz * (PLATFORM_SIZE * 0.5f),
    };

    // Cube's resting center relative to the pivot at progress=0.
    Vector3 offset = {
        start.x - pivot.x,
        cubeRestY - pivot.y,
        start.z - pivot.z,
    };

    // Rotation axis: perpendicular to movement, horizontal. Cross(up, dir).
    // Positive rotation tilts the top of the cube toward the movement direction.
    float axisX = (float)dz;
    float axisY = 0.0f;
    float axisZ = (float)-dx;

    float angle = e->moveProgress * 90.0f;   // degrees

    rlPushMatrix();
        rlTranslatef(pivot.x, pivot.y, pivot.z);
        rlRotatef(angle, axisX, axisY, axisZ);
        rlTranslatef(offset.x, offset.y, offset.z);

        Vector3 origin = { 0.0f, 0.0f, 0.0f };
        DrawCube     (origin, CUBE_SIZE, CUBE_SIZE, CUBE_SIZE, ENEMY_BODY);
        DrawCubeWires(origin, CUBE_SIZE, CUBE_SIZE, CUBE_SIZE, BLACK);
    rlPopMatrix();
}
