#ifndef PLATFORM_H
#define PLATFORM_H

#include "raylib.h"

#define PLATFORM_SIZE   1.0f
#define PLATFORM_HEIGHT 0.25f

typedef enum {
    PLATFORM_NORMAL,
    // PLATFORM_TRAP,
    PLATFORM_EXIT,
} PlatformType;

typedef struct {
    Vector3 position;
    PlatformType type;
} Platform;

void DrawPlatform(Platform *platform);

#endif
