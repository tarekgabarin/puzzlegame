#include "camera_intro.h"
#include <math.h>

#define INTRO_DURATION               2.2f
#define GO_HOLD                      0.5f
#define GO_POP_DURATION              0.3f
#define GO_FADE_DURATION             0.2f

#define SWIRL_START_ANGLE_OFFSET_RAD 3.14159265358979323846f   // 180 degrees
#define SWIRL_START_RADIUS_MULT      1.4f
#define SWIRL_START_ELEV_MULT        1.5f

#define SPLASH_FONT_SIZE             60.0f
#define SPLASH_FONT_SPACING          3.0f

// --- Cylindrical-coordinate helpers ---------------------------------------

// Horizontal angle (around Y) and radius of the camera relative to its target.
// Angle convention: 0 radians = +Z direction, increases toward +X.
static void DecomposeCamera(Camera3D cam, float *angle, float *radius, float *elev) {
    float dx = cam.position.x - cam.target.x;
    float dz = cam.position.z - cam.target.z;
    *radius = sqrtf(dx * dx + dz * dz);
    *angle  = atan2f(dx, dz);
    *elev   = cam.position.y - cam.target.y;
}

static Vector3 PositionFromCylindrical(Vector3 target, float angle, float radius, float elev) {
    return (Vector3){
        target.x + sinf(angle) * radius,
        target.y + elev,
        target.z + cosf(angle) * radius,
    };
}

// --- Lifecycle ------------------------------------------------------------

CameraIntro CreateCameraIntro(Camera3D fittedTarget) {
    CameraIntro intro = { 0 };
    intro.targetCam   = fittedTarget;
    intro.levelCenter = fittedTarget.target;
    intro.phase       = CAM_INTRO_PHASE_INTRO;
    intro.elapsed     = 0.0f;

    DecomposeCamera(fittedTarget,
                    &intro.targetAngle,
                    &intro.targetRadius,
                    &intro.targetElev);

    intro.startAngle  = intro.targetAngle + SWIRL_START_ANGLE_OFFSET_RAD;
    intro.startRadius = intro.targetRadius * SWIRL_START_RADIUS_MULT;
    intro.startElev   = intro.targetElev   * SWIRL_START_ELEV_MULT;

    return intro;
}

// Cubic ease-in-out on t in [0,1].
static float Smoothstep(float t) {
    return t * t * (3.0f - 2.0f * t);
}

void UpdateCameraIntro(CameraIntro *intro, Camera3D *cam, float dt) {
    intro->elapsed += dt;

    if (intro->phase == CAM_INTRO_PHASE_INTRO) {
        float t = intro->elapsed / INTRO_DURATION;
        if (t > 1.0f) t = 1.0f;
        float te = Smoothstep(t);

        float a = intro->startAngle  + (intro->targetAngle  - intro->startAngle ) * te;
        float r = intro->startRadius + (intro->targetRadius - intro->startRadius) * te;
        float e = intro->startElev   + (intro->targetElev   - intro->startElev  ) * te;

        cam->target     = intro->levelCenter;
        cam->position   = PositionFromCylindrical(intro->levelCenter, a, r, e);
        cam->up         = intro->targetCam.up;
        cam->fovy       = intro->targetCam.fovy;
        cam->projection = intro->targetCam.projection;

        if (intro->elapsed >= INTRO_DURATION) {
            intro->phase   = CAM_INTRO_PHASE_GO;
            intro->elapsed = 0.0f;
        }
        return;
    }

    if (intro->phase == CAM_INTRO_PHASE_GO) {
        *cam = intro->targetCam;
        if (intro->elapsed >= GO_HOLD) {
            intro->phase   = CAM_INTRO_PHASE_PLAY;
            intro->elapsed = 0.0f;
        }
        return;
    }

    // PLAY: leave cam untouched.
}

bool CameraIntroAcceptsInput(const CameraIntro *intro) {
    return intro->phase != CAM_INTRO_PHASE_INTRO;
}

// --- Splash rendering -----------------------------------------------------

void DrawCameraIntroSplash(const CameraIntro *intro) {
    if (intro->phase == CAM_INTRO_PHASE_PLAY) return;

    Font font = GetFontDefault();
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    if (intro->phase == CAM_INTRO_PHASE_INTRO) {
        const char *text = "READY...";
        Vector2 size = MeasureTextEx(font, text, SPLASH_FONT_SIZE, SPLASH_FONT_SPACING);
        Vector2 pos  = {
            (screenW - size.x) * 0.5f,
            screenH * 0.33f - size.y * 0.5f,
        };
        unsigned char alpha = (unsigned char)(180.0f + 75.0f * sinf(intro->elapsed * 6.0f));
        Color shadow = { 0,   0, 0, alpha };
        Color main   = { 255, 140, 0, alpha };   // ORANGE
        Vector2 shadowPos = { pos.x + 3.0f, pos.y + 3.0f };
        DrawTextEx(font, text, shadowPos, SPLASH_FONT_SIZE, SPLASH_FONT_SPACING, shadow);
        DrawTextEx(font, text, pos,       SPLASH_FONT_SIZE, SPLASH_FONT_SPACING, main);
        return;
    }

    // CAM_INTRO_PHASE_GO
    const char *text = "GO!";

    float popT = intro->elapsed / GO_POP_DURATION;
    if (popT > 1.0f) popT = 1.0f;
    float scale = 1.0f + 0.4f * sinf(PI * popT);

    float alphaF = 1.0f;
    float fadeStart = GO_HOLD - GO_FADE_DURATION;
    if (intro->elapsed > fadeStart) {
        alphaF = (GO_HOLD - intro->elapsed) / GO_FADE_DURATION;
        if (alphaF < 0.0f) alphaF = 0.0f;
    }

    float fontSize = SPLASH_FONT_SIZE * scale;
    float spacing  = SPLASH_FONT_SPACING * scale;
    Vector2 size = MeasureTextEx(font, text, fontSize, spacing);
    Vector2 pos  = {
        (screenW - size.x) * 0.5f,
        screenH * 0.33f - size.y * 0.5f,
    };
    unsigned char alpha = (unsigned char)(255.0f * alphaF);
    Color shadow = {   0,   0,   0, alpha };
    Color main   = {  50, 220,  50, alpha };   // bright green
    Vector2 shadowPos = { pos.x + 3.0f * scale, pos.y + 3.0f * scale };
    DrawTextEx(font, text, shadowPos, fontSize, spacing, shadow);
    DrawTextEx(font, text, pos,       fontSize, spacing, main);
}
