#ifndef LEVEL_H
#define LEVEL_H

#include "platform.h"

typedef struct {
    Platform *platforms;
    int platformCount;
    int gridWidth;
    int gridHeight;
} Level;

// Loads a level from a text file. Caller must call UnloadLevel when done.
Level LoadLevel(const char *filename);

// Frees the platform array and zeros the struct.
void UnloadLevel(Level *level);

// Draws every platform in the level using the shared platform model.
void DrawLevel(Level *level);

#endif
