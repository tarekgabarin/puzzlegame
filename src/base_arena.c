#include "base_arena.h"

Camera3D CreateCamera(void) {
    Camera3D camera = {0};
    camera.position   = (Vector3){ 3.0f, 4.0f, 6.0f };
    camera.target     = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up         = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy       = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    return camera;
}
