#include "level.h"
#include "raylib.h"
#include <stdbool.h>
#include <stddef.h>

// Maps a single character from a level file to a PlatformType.
// Returns true and writes to *out when the character represents a platform.
// Returns false for empty cells ('.') and any other unrecognised characters,
// which the caller should skip silently.
static bool CharToPlatformType(char c, PlatformType *out) {
    switch (c) {
        case 'N': *out = PLATFORM_NORMAL; return true;
        case 'E': *out = PLATFORM_EXIT;   return true;
        default:  return false;
    }
}

// Returns true if the line should be skipped entirely (comment or blank).
// A comment line begins with '#' as the first non-whitespace character.
static bool IsSkippableLine(const char *line, int length) {
    if (length == 0) return true;
    for (int i = 0; i < length; i++) {
        char c = line[i];
        if (c == ' ' || c == '\t' || c == '\r') continue;
        if (c == '#') return true;
        return false;
    }
    return true;   // whitespace-only
}

Level LoadLevel(const char *filename) {
    Level level = { 0 };

    char *text = LoadFileText(filename);
    if (text == NULL) {
        TraceLog(LOG_WARNING, "LoadLevel: could not open '%s'", filename);
        return level;
    }

    // Walk the text twice. First pass just counts: we want exactly
    // `platformCount` slots so we can allocate once with no resizing.
    int platformCount = 0;
    int gridHeight    = 0;
    int gridWidth     = 0;

    int lineStart = 0;
    int textLen   = (int)TextLength(text);
    for (int i = 0; i <= textLen; i++) {
        if (i == textLen || text[i] == '\n') {
            int lineLen = i - lineStart;
            const char *line = &text[lineStart];

            if (!IsSkippableLine(line, lineLen)) {
                int cols = 0;
                for (int c = 0; c < lineLen; c++) {
                    if (line[c] == '\r') continue;
                    PlatformType dummy;
                    if (CharToPlatformType(line[c], &dummy)) platformCount++;
                    cols++;
                }
                if (cols > gridWidth) gridWidth = cols;
                gridHeight++;
            }

            lineStart = i + 1;
        }
    }

    level.platformCount = platformCount;
    level.gridWidth     = gridWidth;
    level.gridHeight    = gridHeight;

    if (platformCount == 0) {
        UnloadFileText(text);
        return level;
    }

    level.platforms = (Platform *)MemAlloc(platformCount * sizeof(Platform));

    // Second pass: fill the platform array.
    int writeIndex = 0;
    int row        = 0;

    lineStart = 0;
    for (int i = 0; i <= textLen; i++) {
        if (i == textLen || text[i] == '\n') {
            int lineLen = i - lineStart;
            const char *line = &text[lineStart];

            if (!IsSkippableLine(line, lineLen)) {
                int col = 0;
                for (int c = 0; c < lineLen; c++) {
                    if (line[c] == '\r') continue;
                    PlatformType type;
                    if (CharToPlatformType(line[c], &type)) {
                        level.platforms[writeIndex].gridX = col;
                        level.platforms[writeIndex].gridZ = row;
                        level.platforms[writeIndex].type  = type;
                        writeIndex++;
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
    if (level->platforms != NULL) {
        MemFree(level->platforms);
    }
    *level = (Level){ 0 };
}

void DrawLevel(Level *level) {
    for (int i = 0; i < level->platformCount; i++) {
        DrawPlatform(&level->platforms[i]);
    }
}
