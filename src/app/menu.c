#include "app/menu.h"
#include "raylib.h"
#include "vendor/raygui.h"
#include <string.h>
#include <stdio.h>

#define BTN_W   240
#define BTN_H   48
#define GAP     16

typedef enum { VIEW_HOME, VIEW_PICK } MenuView;

static void DrawTitle(const char *text, int yTop) {
    int screenW = GetScreenWidth();
    int fontSize = 48;
    int textW = MeasureText(text, fontSize);
    DrawText(text, (screenW - textW) / 2, yTop, fontSize, DARKGRAY);
}

// Builds the displayable list of level filenames in levels/.
// Returns a newly-allocated array of char* (and writes the count). Caller frees
// each entry then the array via FreeLevelList.
static char **LoadLevelList(int *outCount) {
    *outCount = 0;
    FilePathList files = LoadDirectoryFiles("levels");
    char **list = (char **)MemAlloc(files.count * sizeof(char *));
    for (unsigned int i = 0; i < files.count; i++) {
        const char *p = files.paths[i];
        if (!IsFileExtension(p, ".txt")) continue;
        const char *base = GetFileName(p);   // "levels/foo.txt" -> "foo.txt"
        size_t len = strlen(base) + 1;
        char *copy = (char *)MemAlloc((unsigned int)len);
        memcpy(copy, base, len);
        list[(*outCount)++] = copy;
    }
    UnloadDirectoryFiles(files);
    return list;
}

static void FreeLevelList(char **list, int count) {
    for (int i = 0; i < count; i++) MemFree(list[i]);
    MemFree(list);
}

// raygui's GuiListView wants a single newline-joined string. Concatenate the
// filenames into one buffer, total size kept tight.
static void JoinForListView(char **list, int count, char *out, int outSize) {
    out[0] = '\0';
    int written = 0;
    for (int i = 0; i < count; i++) {
        int n = snprintf(out + written, outSize - written,
                         "%s%s", (i == 0 ? "" : ";"), list[i]);
        if (n < 0 || n >= outSize - written) break;
        written += n;
    }
}

AppState RunMainMenu(char *outLevelPath, int outLevelPathSize) {
    MenuView view = VIEW_HOME;

    char **levelList = NULL;
    int    levelCount = 0;
    char   joined[2048] = "";
    int    listScroll = 0;
    int    listActive = 0;

    while (!WindowShouldClose()) {
        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();

        BeginDrawing();
            ClearBackground(RAYWHITE);

            if (view == VIEW_HOME) {
                DrawTitle("Engacho", 80);

                int cx = (screenW - BTN_W) / 2;
                int cy = 200;

                if (GuiButton((Rectangle){ (float)cx, (float)cy, BTN_W, BTN_H }, "Play")) {
                    // Refresh level list when entering the picker.
                    if (levelList) FreeLevelList(levelList, levelCount);
                    levelList = LoadLevelList(&levelCount);
                    JoinForListView(levelList, levelCount, joined, sizeof(joined));
                    listScroll = 0;
                    listActive = (levelCount > 0) ? 0 : -1;
                    view = VIEW_PICK;
                }
                cy += BTN_H + GAP;
                if (GuiButton((Rectangle){ (float)cx, (float)cy, BTN_W, BTN_H }, "Level Editor")) {
                    if (levelList) FreeLevelList(levelList, levelCount);
                    EndDrawing();
                    return APP_EDITOR;
                }
                cy += BTN_H + GAP;
                if (GuiButton((Rectangle){ (float)cx, (float)cy, BTN_W, BTN_H }, "Quit")) {
                    if (levelList) FreeLevelList(levelList, levelCount);
                    EndDrawing();
                    return APP_QUIT;
                }
            } else {  // VIEW_PICK
                DrawTitle("Pick a level", 60);

                int listW = 400;
                int listH = 320;
                int listX = (screenW - listW) / 2;
                int listY = 140;

                if (levelCount == 0) {
                    const char *msg = "No levels found in levels/";
                    int mw = MeasureText(msg, 20);
                    DrawText(msg, (screenW - mw) / 2, listY + listH / 2 - 10, 20, GRAY);
                } else {
                    GuiListView((Rectangle){ (float)listX, (float)listY, listW, listH },
                                joined, &listScroll, &listActive);
                }

                int btnY = listY + listH + GAP;
                int twoBtnW = (BTN_W * 2 + GAP);
                int btnX = (screenW - twoBtnW) / 2;

                bool canPlay = (levelCount > 0 && listActive >= 0 && listActive < levelCount);
                GuiSetState(canPlay ? STATE_NORMAL : STATE_DISABLED);
                if (GuiButton((Rectangle){ (float)btnX, (float)btnY, BTN_W, BTN_H }, "Play Selected")
                    && canPlay) {
                    snprintf(outLevelPath, outLevelPathSize, "levels/%s", levelList[listActive]);
                    FreeLevelList(levelList, levelCount);
                    levelList = NULL;
                    EndDrawing();
                    return APP_PLAY;
                }
                GuiSetState(STATE_NORMAL);

                if (GuiButton((Rectangle){ (float)(btnX + BTN_W + GAP), (float)btnY, BTN_W, BTN_H },
                              "Back")) {
                    view = VIEW_HOME;
                }
            }
        EndDrawing();
    }

    if (levelList) FreeLevelList(levelList, levelCount);
    return APP_QUIT;
}
