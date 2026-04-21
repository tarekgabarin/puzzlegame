#ifndef BASE_ARENA_H
#define BASE_ARENA_H

#include "raylib.h"

Camera3D CreateCamera(void);

// Repositions the camera so a level of the given grid dimensions fits
// comfortably in view, targeted at the level's center.
void FitCameraToLevel(Camera3D *camera, int gridWidth, int gridHeight);

#endif
