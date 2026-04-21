#include "platform.h"

void DrawPlatform(Platform *platform) {
    Color color;

    switch (platform->type) {
        case PLATFORM_NORMAL: color = GREEN; break;
        case PLATFORM_EXIT:   color = BLUE;  break;
        default:              color = GRAY;  break;
    }

    DrawCube(platform->position, PLATFORM_SIZE, PLATFORM_HEIGHT, PLATFORM_SIZE, color);
    DrawCubeWires(platform->position, PLATFORM_SIZE, PLATFORM_HEIGHT, PLATFORM_SIZE, BLACK);
}
