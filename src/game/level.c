#include "game/level.h"
#include "raylib.h"
#include <stdbool.h>
#include <stddef.h>

// Default move budget when a level file has no '# steps: N' directive.
#define DEFAULT_MOVE_LIMIT 28

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

// Parse a '# steps: N' directive. Accepts optional leading whitespace, the '#',
// optional whitespace, the keyword "steps", an optional ':', whitespace, then a
// non-negative integer. Returns true (and writes *out) only on a full match.
static bool ParseStepsDirective(const char *line, int length, int *out) {
    int i = 0;
    while (i < length && (line[i] == ' ' || line[i] == '\t')) i++;
    if (i >= length || line[i] != '#') return false;
    i++;
    while (i < length && (line[i] == ' ' || line[i] == '\t')) i++;

    const char *kw = "steps";
    for (int k = 0; kw[k] != '\0'; k++) {
        if (i >= length || line[i] != kw[k]) return false;
        i++;
    }

    while (i < length && (line[i] == ' ' || line[i] == '\t')) i++;
    if (i < length && line[i] == ':') i++;
    while (i < length && (line[i] == ' ' || line[i] == '\t')) i++;

    if (i >= length || line[i] < '0' || line[i] > '9') return false;
    int value = 0;
    while (i < length && line[i] >= '0' && line[i] <= '9') {
        value = value * 10 + (line[i] - '0');
        i++;
    }
    *out = value;
    return true;
}

// --- Level helpers --------------------------------------------------------

bool IsWalkable(const Level *level, int x, int z) {
    if (x < 0 || x >= level->gridWidth || z < 0 || z >= level->gridHeight) return false;
    return level->tileTypes[z * level->gridWidth + x] != PLATFORM_NONE;
}

bool IsExitTile(const Level *level, int x, int z) {
    if (x < 0 || x >= level->gridWidth || z < 0 || z >= level->gridHeight) return false;
    return level->tileTypes[z * level->gridWidth + x] == PLATFORM_EXIT;
}

const Enemy *GetEnemyAt(const Level *level, int x, int z) {
    for (int i = 0; i < level->enemyCount; i++) {
        const Enemy *e = &level->enemies[i];
        if (e->gridX == x && e->gridZ == z) return e;
    }
    return NULL;
}

bool HasEnemyAt(const Level *level, int x, int z) {
    return GetEnemyAt(level, x, z) != NULL;
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
    bool foundSteps   = false;
    int  moveLimit    = DEFAULT_MOVE_LIMIT;

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
            } else {
                // Comment line — look for the '# steps: N' directive.
                int steps;
                if (ParseStepsDirective(line, lineLen, &steps)) {
                    moveLimit  = steps;
                    foundSteps = true;
                }
            }

            lineStart = i + 1;
        }
    }

    level.platformCount = platformCount;
    level.enemyCount    = enemyCount;
    level.gridWidth     = gridWidth;
    level.gridHeight    = gridHeight;
    level.moveLimit     = moveLimit;

    if (!foundSteps) {
        TraceLog(LOG_INFO, "LoadLevel: '%s' has no '# steps:' directive; defaulting to %d moves",
                 filename, DEFAULT_MOVE_LIMIT);
    }

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

    int cellCount = gridWidth * gridHeight;
    level.tileTypes = (PlatformType *)MemAlloc(cellCount * sizeof(PlatformType));
    for (int i = 0; i < cellCount; i++) level.tileTypes[i] = PLATFORM_NONE;

    // Pass 2: fill platforms[], enemies[], tileTypes[], and remember the player spawn.
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
                        level.tileTypes[row * gridWidth + col] = pt;
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
                    }

                    col++;
                }
                row++;
            }

            lineStart = i + 1;
        }
    }

    UnloadFileText(text);
    return level;
}

void UnloadLevel(Level *level) {
    if (level->platforms != NULL) MemFree(level->platforms);
    if (level->enemies   != NULL) MemFree(level->enemies);
    if (level->tileTypes != NULL) MemFree(level->tileTypes);
    *level = (Level){ 0 };
}

void DrawLevel(Level *level) {
    for (int i = 0; i < level->platformCount; i++) {
        DrawPlatform(&level->platforms[i]);
    }
}
