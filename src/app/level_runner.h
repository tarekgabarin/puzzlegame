#ifndef LEVEL_RUNNER_H
#define LEVEL_RUNNER_H

#include "raylib.h"

// Loads the level from the given file path, runs the per-frame loop until
// the window is closed, then unloads the level.
void RunLevel(Camera3D *camera, const char *levelFile);

#endif
