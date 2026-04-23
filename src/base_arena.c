#include "base_arena.h"
#include "platform.h"
#include "raymath.h"
#include <math.h>

#define CAMERA_FILL_RATIO     0.85f
#define FIT_SEARCH_ITERS      24
#define FIT_SEARCH_D_MIN      1.0f
#define FIT_SEARCH_D_MAX      100.0f
#define CENTER_SEARCH_ITERS   6
#define CENTER_TOLERANCE_PX   0.5f

Camera3D CreateCamera(void) {
    Camera3D camera = { 0 };
    camera.position   = (Vector3){ 3.0f, 4.0f, 6.0f };
    camera.target     = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up         = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy       = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    return camera;
}

// --- AABB from placed platforms -------------------------------------------

// Writes 8 AABB corners for the level into `corners`. AABB is derived from
// level->platforms[] so it's tight regardless of grid padding, shape, etc.
static void BuildLevelCorners(const Level *level, Vector3 *corners) {
    float half = PLATFORM_SIZE  * 0.5f;
    float platY = PLATFORM_HEIGHT * 0.5f;

    // Initialize min/max from first platform, then scan the rest.
    float minX = level->platforms[0].gridX * PLATFORM_SIZE - half;
    float maxX = level->platforms[0].gridX * PLATFORM_SIZE + half;
    float minZ = level->platforms[0].gridZ * PLATFORM_SIZE - half;
    float maxZ = level->platforms[0].gridZ * PLATFORM_SIZE + half;

    for (int i = 1; i < level->platformCount; i++) {
        float x = level->platforms[i].gridX * PLATFORM_SIZE;
        float z = level->platforms[i].gridZ * PLATFORM_SIZE;
        if (x - half < minX) minX = x - half;
        if (x + half > maxX) maxX = x + half;
        if (z - half < minZ) minZ = z - half;
        if (z + half > maxZ) maxZ = z + half;
    }

    corners[0] = (Vector3){ minX, -platY, minZ };
    corners[1] = (Vector3){ maxX, -platY, minZ };
    corners[2] = (Vector3){ minX, -platY, maxZ };
    corners[3] = (Vector3){ maxX, -platY, maxZ };
    corners[4] = (Vector3){ minX,  platY, minZ };
    corners[5] = (Vector3){ maxX,  platY, minZ };
    corners[6] = (Vector3){ minX,  platY, maxZ };
    corners[7] = (Vector3){ maxX,  platY, maxZ };
}

static Vector3 AabbCenter(const Vector3 *corners) {
    return (Vector3){
        (corners[0].x + corners[3].x) * 0.5f,
        0.0f,
        (corners[0].z + corners[3].z) * 0.5f,
    };
}

// --- Screen-space bbox helpers --------------------------------------------

typedef struct {
    float minX, maxX, minY, maxY;
} ScreenBBox;

static ScreenBBox ProjectCornersBBox(Camera3D cam, const Vector3 *corners) {
    Vector2 p0 = GetWorldToScreen(corners[0], cam);
    ScreenBBox b = { p0.x, p0.x, p0.y, p0.y };
    for (int i = 1; i < 8; i++) {
        Vector2 p = GetWorldToScreen(corners[i], cam);
        if (p.x < b.minX) b.minX = p.x;
        if (p.x > b.maxX) b.maxX = p.x;
        if (p.y < b.minY) b.minY = p.y;
        if (p.y > b.maxY) b.maxY = p.y;
    }
    return b;
}

// `fill` as `max(bboxW/screenW, bboxH/screenH)`. Monotonically decreasing in
// camera distance, which is what the binary search relies on.
static float FillRatio(ScreenBBox b) {
    float bboxW = b.maxX - b.minX;
    float bboxH = b.maxY - b.minY;
    float fx = bboxW / (float)GetScreenWidth();
    float fy = bboxH / (float)GetScreenHeight();
    return (fx > fy) ? fx : fy;
}

// --- Camera basis vectors -------------------------------------------------

static Vector3 CameraRight(Camera3D cam) {
    Vector3 fwd = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    return Vector3Normalize(Vector3CrossProduct(fwd, cam.up));
}

static Vector3 CameraUp(Camera3D cam) {
    Vector3 fwd   = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(fwd, cam.up));
    return Vector3CrossProduct(right, fwd);
}

// Shifts cam.target (and cam.position along with it, to preserve the viewing
// angle) so the screen-space bbox of `corners` becomes centered on the screen.
// Iterates a few times because the projection is nonlinear.
static void CenterBBoxOnScreen(Camera3D *cam, const Vector3 *corners) {
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    Vector2 screenCenter = { screenW * 0.5f, screenH * 0.5f };

    for (int iter = 0; iter < CENTER_SEARCH_ITERS; iter++) {
        ScreenBBox b = ProjectCornersBBox(*cam, corners);
        Vector2 bboxCenter = {
            (b.minX + b.maxX) * 0.5f,
            (b.minY + b.maxY) * 0.5f,
        };
        Vector2 delta = {
            bboxCenter.x - screenCenter.x,
            bboxCenter.y - screenCenter.y,
        };
        if (fabsf(delta.x) < CENTER_TOLERANCE_PX &&
            fabsf(delta.y) < CENTER_TOLERANCE_PX) break;

        // At the depth from camera to target, one pixel of screen space maps
        // to (2 * depth * tan(fovy/2)) / screenH world units.
        float depth = Vector3Length(Vector3Subtract(cam->target, cam->position));
        float worldPerPixel = (2.0f * depth * tanf(cam->fovy * DEG2RAD * 0.5f)) / screenH;

        Vector3 right = CameraRight(*cam);
        Vector3 up    = CameraUp(*cam);

        // Shift camera (target + position together) to move the projection
        // opposite-ish: if bbox is right of screen center, move camera right so
        // the projection shifts left toward center. Screen Y grows downward
        // while world up is +Y, so the Y term is negated.
        Vector3 shift = Vector3Add(
            Vector3Scale(right,  delta.x * worldPerPixel),
            Vector3Scale(up,    -delta.y * worldPerPixel)
        );
        cam->target   = Vector3Add(cam->target,   shift);
        cam->position = Vector3Add(cam->position, shift);
    }
}

// --- Main entry point -----------------------------------------------------

Camera3D ComputeFittedCamera(const Level *level) {
    Camera3D cam = CreateCamera();

    if (level->platformCount <= 0) return cam;   // empty level: bail

    Vector3 corners[8];
    BuildLevelCorners(level, corners);

    cam.target = AabbCenter(corners);

    // Preset direction matches the original FitCameraToLevel ratios
    // (1 : 1.4 : 1.8), preserving the established overhead-diagonal look.
    // Normalized so `d` in the search is an actual distance.
    Vector3 dir = Vector3Normalize((Vector3){ 1.0f, 1.4f, 1.8f });

    // Binary search camera distance for the target fill ratio.
    float dLo = FIT_SEARCH_D_MIN;
    float dHi = FIT_SEARCH_D_MAX;
    for (int i = 0; i < FIT_SEARCH_ITERS; i++) {
        float d = (dLo + dHi) * 0.5f;
        cam.position = Vector3Add(cam.target, Vector3Scale(dir, d));
        float fill = FillRatio(ProjectCornersBBox(cam, corners));
        if (fill > CAMERA_FILL_RATIO) dLo = d;   // too close, push further
        else                          dHi = d;   // too far, pull closer
    }
    float d = (dLo + dHi) * 0.5f;
    cam.position = Vector3Add(cam.target, Vector3Scale(dir, d));

    // Re-center: shift camera so the screen bbox (which is offset from the
    // AABB center due to perspective) lines up with the screen middle.
    CenterBBoxOnScreen(&cam, corners);

    return cam;
}
