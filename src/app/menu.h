#ifndef MENU_H
#define MENU_H

typedef enum {
    APP_MENU,
    APP_PLAY,
    APP_EDITOR,
    APP_QUIT,
} AppState;

// Runs the main-menu loop until the user picks an action or closes the window.
// On APP_PLAY, fills outLevelPath with the full path (e.g. "levels/foo.txt").
AppState RunMainMenu(char *outLevelPath, int outLevelPathSize);

#endif
