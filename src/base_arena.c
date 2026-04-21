#include "base_arena.h"
#include "platform.h"

Camera3D CreateCamera(void) {
    Camera3D camera = {0};
    camera.position   = (Vector3){ 3.0f, 4.0f, 6.0f };
    camera.target     = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up         = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy       = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    return camera;
}

void FitCameraToLevel(Camera3D *camera, int gridWidth, int gridHeight) {
    // World-space size and center of the level. The -1 accounts for grid
    // coordinates being tile indices (grid 0 through gridWidth-1 spans
    // gridWidth tiles), so the center sits halfway between the end tiles.
    float worldWidth = gridWidth  * PLATFORM_SIZE;
    float worldDepth = gridHeight * PLATFORM_SIZE;
    float centerX    = (gridWidth  - 1) * PLATFORM_SIZE * 0.5f;
    float centerZ    = (gridHeight - 1) * PLATFORM_SIZE * 0.5f;

    camera->target = (Vector3){ centerX, 0.0f, centerZ };

    // Scale the camera's offset by whichever axis is larger so the whole
    // level always fits. The 1.0 / 1.4 / 1.8 ratios give a diagonal overhead
    // view that looks the same regardless of level size.
    float maxDim = (worldWidth > worldDepth) ? worldWidth : worldDepth;
    camera->position = (Vector3){
        centerX + maxDim * 1.0f,
        maxDim * 1.4f,
        centerZ + maxDim * 1.8f,
    };
}
