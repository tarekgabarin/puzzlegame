#ifndef BASE_ARENA_H
#define BASE_ARENA_H

#include "raylib.h"
#include "level.h"

Camera3D CreateCamera(void);

// Returns the camera pose that frames the level tightly — the screen-space
// projection of the level's platforms fills CAMERA_FILL_RATIO of the tighter
// screen axis, and is centered on screen. AABB is derived from the actual
// placed platforms so it adapts to any level shape (any grid dims, empty
// borders, etc.). Pose is not applied; caller decides when / how to use it.
Camera3D ComputeFittedCamera(const Level *level);

#endif
