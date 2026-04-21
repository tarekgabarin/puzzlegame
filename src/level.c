#include "level.h"
#include "raylib.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

// --- Character classification ---------------------------------------------

// Every tile-placing character (N/E/P/1-4) implies a platform underneath.
// Returns the platform type for those; '.' and unrecognised chars return
// PLATFORM_NONE (callers check IsGridChar to distinguish).
static PlatformType CharToPlatformType(char c) {
    switch (c) {
        case 'N': case 'P': case '1': case '2': case '3': case '4':
            return PLATFORM_NORMAL;
        case 'E':
            return PLATFORM_EXIT;
        default:
            return PLATFORM_NONE;
    }
}

static bool CharIsEnemy(char c, EnemyType *outType) {
    switch (c) {
        case '1': *outType = ENEMY_TONGUE;      return true;
        case '2': *outType = ENEMY_WINGED_BUTT; return true;
        case '3': *outType = ENEMY_SNOTTY;      return true;
        case '4': *outType = ENEMY_ARMPITS;     return true;
        default: return false;
    }
}

// A grid char is one that occupies a cell (including an empty '.').
static bool IsGridChar(char c) {
    return c == 'N' || c == 'E' || c == 'P' ||
           c == '1' || c == '2' || c == '3' || c == '4' ||
           c == '.';
}

// Skip whole-line comments ('#' as first non-whitespace) and blank lines.
static bool IsSkippableLine(const char *line, int length) {
    if (length == 0) return true;
    for (int i = 0; i < length; i++) {
        char c = line[i];
        if (c == ' ' || c == '\t' || c == '\r') continue;
        if (c == '#') return true;
        return false;
    }
    return true;
}

// --- Level helpers (occupancy wrappers) -----------------------------------

bool IsWalkable(const Level *level, int x, int z) {
    if (x < 0 || x >= level->gridWidth || z < 0 || z >= level->gridHeight) return false;
    TileCell *cells = EntityMapGet(&level->entities, "player");
    if (cells == NULL) return false;
    return cells[z * level->gridWidth + x].platformType != PLATFORM_NONE;
}

bool HasPlayerAt(const Level *level, int x, int z) {
    return EntityMapIsPresent(&level->entities, "player", x, z);
}

bool HasEnemyAt(const Level *level, int x, int z) {
    return EntityMapAnyPresentWithPrefixAt(&level->entities, "enemy_", x, z);
}

const Enemy *GetEnemyAt(const Level *level, int x, int z) {
    if (x < 0 || x >= level->gridWidth || z < 0 || z >= level->gridHeight) return NULL;
    for (int i = 0; i < level->enemyCount; i++) {
        const Enemy *e = &level->enemies[i];
        if (EntityMapIsPresent(&level->entities, e->name, x, z)) return e;
    }
    return NULL;
}

const char *GetEnemyNameAt(const Level *level, int x, int z) {
    const Enemy *e = GetEnemyAt(level, x, z);
    return e ? e->name : NULL;
}

void SetPlayerAt(Level *level, int x, int z, bool present) {
    EntityMapSetPresent(&level->entities, "player", x, z, present);
}

// --- Loading ---------------------------------------------------------------

Level LoadLevel(const char *filename) {
    Level level = { 0 };

    char *text = LoadFileText(filename);
    if (text == NULL) {
        TraceLog(LOG_WARNING, "LoadLevel: could not open '%s'", filename);
        return level;
    }

    // Pass 1: dimensions, platform count, enemy count, presence of 'P'.
    int platformCount = 0;
    int enemyCount    = 0;
    int gridHeight    = 0;
    int gridWidth     = 0;
    bool foundPlayer  = false;

    int textLen   = (int)TextLength(text);
    int lineStart = 0;
    for (int i = 0; i <= textLen; i++) {
        if (i == textLen || text[i] == '\n') {
            int lineLen = i - lineStart;
            const char *line = &text[lineStart];

            if (!IsSkippableLine(line, lineLen)) {
                int cols = 0;
                for (int c = 0; c < lineLen; c++) {
                    char ch = line[c];
                    if (ch == '\r') continue;
                    if (!IsGridChar(ch)) continue;
                    cols++;
                    if (CharToPlatformType(ch) != PLATFORM_NONE) platformCount++;
                    EnemyType dummy;
                    if (CharIsEnemy(ch, &dummy)) enemyCount++;
                    if (ch == 'P') foundPlayer = true;
                }
                if (cols > gridWidth) gridWidth = cols;
                gridHeight++;
            }

            lineStart = i + 1;
        }
    }

    level.platformCount = platformCount;
    level.enemyCount    = enemyCount;
    level.gridWidth     = gridWidth;
    level.gridHeight    = gridHeight;

    if (gridWidth == 0 || gridHeight == 0) {
        UnloadFileText(text);
        return level;
    }

    if (!foundPlayer) {
        TraceLog(LOG_WARNING, "LoadLevel: no 'P' in '%s'; defaulting playerStart to (0,0)", filename);
    }

    level.platforms = (platformCount > 0)
        ? (Platform *)MemAlloc(platformCount * sizeof(Platform))
        : NULL;
    level.enemies = (enemyCount > 0)
        ? (Enemy *)MemAlloc(enemyCount * sizeof(Enemy))
        : NULL;
    EntityMapInit(&level.entities, gridWidth, gridHeight);

    // Reserve one hash-map entry per named entity; cells start zeroed.
    EntityMapPut(&level.entities, "player");
    for (int i = 1; i <= enemyCount; i++) {
        char name[ENTITY_NAME_LEN];
        snprintf(name, sizeof(name), "enemy_%d", i);
        EntityMapPut(&level.entities, name);
    }

    // Pass 2: fill platforms[], enemies[], and remember the player spawn.
    int platformWriteIdx = 0;
    int enemyWriteIdx    = 0;
    int row              = 0;

    lineStart = 0;
    for (int i = 0; i <= textLen; i++) {
        if (i == textLen || text[i] == '\n') {
            int lineLen = i - lineStart;
            const char *line = &text[lineStart];

            if (!IsSkippableLine(line, lineLen)) {
                int col = 0;
                for (int c = 0; c < lineLen; c++) {
                    char ch = line[c];
                    if (ch == '\r') continue;
                    if (!IsGridChar(ch)) continue;

                    PlatformType pt = CharToPlatformType(ch);
                    if (pt != PLATFORM_NONE) {
                        Platform *p = &level.platforms[platformWriteIdx++];
                        p->id    = row * gridWidth + col;
                        p->gridX = col;
                        p->gridZ = row;
                        p->type  = pt;
                    }

                    if (ch == 'P') {
                        level.playerStartX = col;
                        level.playerStartZ = row;
                    }

                    EnemyType et;
                    if (CharIsEnemy(ch, &et)) {
                        Enemy *e = &level.enemies[enemyWriteIdx++];
                        e->gridX = col;
                        e->gridZ = row;
                        e->type  = et;
                        snprintf(e->name, sizeof(e->name), "enemy_%d", enemyWriteIdx);
                    }

                    col++;
                }
                row++;
            }

            lineStart = i + 1;
        }
    }

    // Mirror platform types into every entity's occupancy grid so each cell
    // carries (present, platformType). Platform types are identical across
    // entities — only `present` differs.
    for (int idx = 0; idx < level.platformCount; idx++) {
        Platform *p = &level.platforms[idx];
        EntityMapSetPlatform(&level.entities, "player", p->gridX, p->gridZ, p->type);
        for (int i = 1; i <= enemyCount; i++) {
            char name[ENTITY_NAME_LEN];
            snprintf(name, sizeof(name), "enemy_%d", i);
            EntityMapSetPlatform(&level.entities, name, p->gridX, p->gridZ, p->type);
        }
    }

    // Mark spawn tiles as occupied.
    SetPlayerAt(&level, level.playerStartX, level.playerStartZ, true);
    for (int i = 0; i < level.enemyCount; i++) {
        const Enemy *e = &level.enemies[i];
        EntityMapSetPresent(&level.entities, e->name, e->gridX, e->gridZ, true);
    }

    UnloadFileText(text);
    return level;
}

void UnloadLevel(Level *level) {
    if (level->platforms != NULL) MemFree(level->platforms);
    if (level->enemies   != NULL) MemFree(level->enemies);
    EntityMapFree(&level->entities);
    *level = (Level){ 0 };
}

void DrawLevel(Level *level) {
    for (int i = 0; i < level->platformCount; i++) {
        DrawPlatform(&level->platforms[i]);
    }
}
