#ifndef CAMERA_INTRO_H
#define CAMERA_INTRO_H

#include "raylib.h"
#include <stdbool.h>

typedef enum {
    CAM_INTRO_PHASE_INTRO,   // swirl + READY splash; input ignored
    CAM_INTRO_PHASE_GO,      // camera fixed; GO splash plays; input enabled
    CAM_INTRO_PHASE_PLAY,    // no splash; normal gameplay
} CameraIntroPhase;

typedef struct {
    Vector3          levelCenter;

    // Camera pose decomposed into cylindrical (angle, radius, elevation)
    // relative to levelCenter. Lerping these avoids the linear-lerp-through-
    // level-center artifact that you'd get from lerping positions directly.
    float            startAngle,  startRadius,  startElev;
    float            targetAngle, targetRadius, targetElev;

    Camera3D         targetCam;   // fitted pose; used in GO / PLAY

    float            elapsed;     // seconds in current phase (INTRO or GO)
    CameraIntroPhase phase;
} CameraIntro;

CameraIntro CreateCameraIntro(Camera3D fittedTarget);
void        UpdateCameraIntro(CameraIntro *intro, Camera3D *cam, float dt);
bool        CameraIntroAcceptsInput(const CameraIntro *intro);
void        DrawCameraIntroSplash(const CameraIntro *intro);

#endif
