#ifndef PLATFORM_H
#define PLATFORM_H

#include "raylib.h"

#define PLATFORM_SIZE      1.0f
#define PLATFORM_HEIGHT    0.25f
#define BORDER_THICKNESS   0.01f

typedef enum {
    PLATFORM_NORMAL,
    // PLATFORM_TRAP,
    PLATFORM_EXIT,
} PlatformType;

typedef struct {
    int gridX;
    int gridZ;
    PlatformType type;
} Platform;

// Loads the shared platform mesh+model into GPU memory. Call once after
// InitWindow and before drawing any platforms.
void InitPlatformResources(void);

// Frees the shared platform model. Call once before CloseWindow.
void UnloadPlatformResources(void);

// Converts integer grid coordinates to a Vector3 world position.
Vector3 GridToWorld(int gridX, int gridZ);

// Draws a platform using the shared model, tinted by its type.
void DrawPlatform(Platform *platform);

#endif
