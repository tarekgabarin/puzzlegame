#include "app/editor.h"
#include "raylib.h"
#include "vendor/raygui.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define EDITOR_MAX_W   32
#define EDITOR_MAX_H   32
#define DEFAULT_W      8
#define DEFAULT_H      8

#define PANEL_W        300       // right-side controls panel
#define PALETTE_H      150       // palette area at the top (fits 2 rows of buttons + Selected line)
#define STATUS_H       32        // status bar at the bottom

#define DEFAULT_MOVE_LIMIT 28    // default step budget for a new level

typedef struct {
    char        ch;
    const char *name;
    Color       color;
} TileInfo;

static const TileInfo TILE_INFO[] = {
    { 'N', "Platform",     GREEN     },
    { 'E', "Level Exit",   BLUE      },
    { 'P', "Player Spawn", YELLOW    },
    { '1', "Enemy Spawn",  RED       },
    { '.', "Empty",        LIGHTGRAY },
};
#define TILE_COUNT  ((int)(sizeof(TILE_INFO) / sizeof(TILE_INFO[0])))

static const TileInfo *FindTile(char ch) {
    for (int i = 0; i < TILE_COUNT; i++) {
        if (TILE_INFO[i].ch == ch) return &TILE_INFO[i];
    }
    return &TILE_INFO[TILE_COUNT - 1];   // fallback: Empty
}

typedef struct {
    char grid[EDITOR_MAX_H][EDITOR_MAX_W];
    int  width;
    int  height;
    int  moveLimit;
    char paint;
    char filename[64];
    char status[128];
} Editor;

static void ClearGrid(Editor *e) {
    for (int z = 0; z < EDITOR_MAX_H; z++)
        for (int x = 0; x < EDITOR_MAX_W; x++)
            e->grid[z][x] = '.';
}

static void EditorReset(Editor *e) {
    ClearGrid(e);
    e->width     = DEFAULT_W;
    e->height    = DEFAULT_H;
    e->moveLimit = DEFAULT_MOVE_LIMIT;
    e->paint     = 'N';
    e->filename[0] = '\0';
    snprintf(e->status, sizeof(e->status), "New level");
}

// Paint a tile, enforcing single-instance rules for P and E.
static void PaintCell(Editor *e, int x, int z, char ch) {
    if (x < 0 || x >= e->width || z < 0 || z >= e->height) return;

    if (ch == 'P' || ch == 'E') {
        for (int zi = 0; zi < e->height; zi++) {
            for (int xi = 0; xi < e->width; xi++) {
                if (e->grid[zi][xi] == ch) e->grid[zi][xi] = 'N';
            }
        }
    }
    e->grid[z][x] = ch;
}

// Strip everything that isn't [A-Za-z0-9_] from the filename in place.
static void SanitizeFilename(char *s) {
    char *w = s;
    for (char *r = s; *r; r++) {
        if (isalnum((unsigned char)*r) || *r == '_') *w++ = *r;
    }
    *w = '\0';
}

static bool GridHas(const Editor *e, char ch) {
    for (int z = 0; z < e->height; z++)
        for (int x = 0; x < e->width; x++)
            if (e->grid[z][x] == ch) return true;
    return false;
}

static void EditorSave(Editor *e) {
    char clean[64];
    snprintf(clean, sizeof(clean), "%s", e->filename);
    SanitizeFilename(clean);
    if (clean[0] == '\0') {
        snprintf(e->status, sizeof(e->status), "Filename required");
        return;
    }
    if (!GridHas(e, 'P')) {
        snprintf(e->status, sizeof(e->status),
                 "No player spawn — place a Player tile first");
        return;
    }

    char fullpath[160];
    snprintf(fullpath, sizeof(fullpath), "levels/%s.txt", clean);

    FILE *f = fopen(fullpath, "w");
    if (!f) {
        snprintf(e->status, sizeof(e->status), "Couldn't open %s for writing", fullpath);
        return;
    }
    fprintf(f, "# Created with editor — Engacho\n");
    fprintf(f, "# steps: %d\n", e->moveLimit);
    for (int z = 0; z < e->height; z++) {
        for (int x = 0; x < e->width; x++) fputc(e->grid[z][x], f);
        fputc('\n', f);
    }
    fclose(f);

    const char *suffix = GridHas(e, 'E') ? "" : " (warning: no Exit)";
    snprintf(e->status, sizeof(e->status), "Saved to %s%s", fullpath, suffix);
}

// Parse a '# steps: N' directive (mirrors the loader in level.c). Returns true
// and writes *out only on a full match.
static bool ParseStepsLine(const char *line, int length, int *out) {
    int i = 0;
    while (i < length && (line[i] == ' ' || line[i] == '\t')) i++;
    if (i >= length || line[i] != '#') return false;
    i++;
    while (i < length && (line[i] == ' ' || line[i] == '\t')) i++;

    const char *kw = "steps";
    for (int k = 0; kw[k] != '\0'; k++) {
        if (i >= length || line[i] != kw[k]) return false;
        i++;
    }

    while (i < length && (line[i] == ' ' || line[i] == '\t')) i++;
    if (i < length && line[i] == ':') i++;
    while (i < length && (line[i] == ' ' || line[i] == '\t')) i++;

    if (i >= length || line[i] < '0' || line[i] > '9') return false;
    int value = 0;
    while (i < length && line[i] >= '0' && line[i] <= '9') {
        value = value * 10 + (line[i] - '0');
        i++;
    }
    *out = value;
    return true;
}

static void EditorLoad(Editor *e) {
    char clean[64];
    snprintf(clean, sizeof(clean), "%s", e->filename);
    SanitizeFilename(clean);
    if (clean[0] == '\0') {
        snprintf(e->status, sizeof(e->status), "Filename required");
        return;
    }
    char fullpath[160];
    snprintf(fullpath, sizeof(fullpath), "levels/%s.txt", clean);

    char *text = LoadFileText(fullpath);
    if (!text) {
        snprintf(e->status, sizeof(e->status), "Couldn't open %s", fullpath);
        return;
    }

    ClearGrid(e);
    e->moveLimit = DEFAULT_MOVE_LIMIT;
    int row = 0;
    int textLen = (int)TextLength(text);
    int lineStart = 0;
    int maxCols = 0;
    for (int i = 0; i <= textLen; i++) {
        if (i == textLen || text[i] == '\n') {
            int lineLen = i - lineStart;
            const char *line = &text[lineStart];

            // Skip whole-line comments and blank lines.
            bool skip = (lineLen == 0);
            for (int c = 0; !skip && c < lineLen; c++) {
                char ch = line[c];
                if (ch == ' ' || ch == '\t' || ch == '\r') continue;
                if (ch == '#') skip = true;
                break;
            }

            // Comment lines may carry the '# steps: N' directive.
            if (skip) {
                int steps;
                if (ParseStepsLine(line, lineLen, &steps)) e->moveLimit = steps;
            }

            if (!skip && row < EDITOR_MAX_H) {
                int col = 0;
                for (int c = 0; c < lineLen && col < EDITOR_MAX_W; c++) {
                    char ch = line[c];
                    if (ch == '\r') continue;
                    bool known = false;
                    for (int t = 0; t < TILE_COUNT; t++) {
                        if (TILE_INFO[t].ch == ch) { known = true; break; }
                    }
                    e->grid[row][col++] = known ? ch : '.';
                }
                if (col > maxCols) maxCols = col;
                row++;
            }

            lineStart = i + 1;
        }
    }
    UnloadFileText(text);

    if (row == 0 || maxCols == 0) {
        snprintf(e->status, sizeof(e->status), "%s is empty", fullpath);
        return;
    }

    e->width  = maxCols;
    e->height = row;
    snprintf(e->status, sizeof(e->status), "Loaded %s (%dx%d)", fullpath, maxCols, row);
}

static void DrawCellAt(Rectangle r, char ch) {
    const TileInfo *t = FindTile(ch);
    DrawRectangleRec(r, t->color);
    DrawRectangleLinesEx(r, 1.0f, DARKGRAY);
    if (ch != '.') {
        char label[2] = { ch, '\0' };
        int fontSize = (int)(r.height * 0.55f);
        if (fontSize < 10) fontSize = 10;
        int tw = MeasureText(label, fontSize);
        DrawText(label,
                 (int)(r.x + (r.width  - tw) * 0.5f),
                 (int)(r.y + (r.height - fontSize) * 0.5f),
                 fontSize, BLACK);
    }
}

void RunEditor(void) {
    Editor ed;
    EditorReset(&ed);

    bool filenameEdit = false;
    int  widthSpin    = ed.width;
    int  heightSpin   = ed.height;
    int  stepsSpin    = ed.moveLimit;
    bool widthEdit    = false;
    bool heightEdit   = false;
    bool stepsEdit    = false;

    while (!WindowShouldClose()) {
        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();

        // --- Grid area geometry --------------------------------------------
        int gridAreaX = 10;
        int gridAreaY = PALETTE_H + 10;
        int gridAreaW = screenW - PANEL_W - gridAreaX - 10;
        int gridAreaH = screenH - PALETTE_H - STATUS_H - 20;

        // Largest cell that fits the area while staying square.
        float cellSize = (float)gridAreaW / ed.width;
        if ((float)gridAreaH / ed.height < cellSize) cellSize = (float)gridAreaH / ed.height;
        if (cellSize < 8.0f) cellSize = 8.0f;
        float gridW = cellSize * ed.width;
        float gridH = cellSize * ed.height;
        float gridX = gridAreaX + (gridAreaW - gridW) * 0.5f;
        float gridY = gridAreaY + (gridAreaH - gridH) * 0.5f;

        // --- Mouse → grid coords -------------------------------------------
        Vector2 mp = GetMousePosition();
        int hoverX = -1, hoverZ = -1;
        if (mp.x >= gridX && mp.x < gridX + gridW &&
            mp.y >= gridY && mp.y < gridY + gridH) {
            hoverX = (int)((mp.x - gridX) / cellSize);
            hoverZ = (int)((mp.y - gridY) / cellSize);
            if (hoverX >= ed.width)  hoverX = ed.width  - 1;
            if (hoverZ >= ed.height) hoverZ = ed.height - 1;
        }

        // Paint only when the click is on the grid and no text field is being
        // edited (avoid swallowing the click meant for a focused widget).
        if (hoverX >= 0 &&
            !filenameEdit && !widthEdit && !heightEdit && !stepsEdit &&
            IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            PaintCell(&ed, hoverX, hoverZ, ed.paint);
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);

            // --- Palette (top) --------------------------------------------
            {
                int btnW = 110;
                int btnH = 36;
                int gap  = 8;
                int x = 10;
                int y = 10;
                DrawText("Palette", x, y, 18, DARKGRAY);
                y += 22;

                for (int i = 0; i < TILE_COUNT; i++) {
                    Rectangle r = { (float)x, (float)y, (float)btnW, (float)btnH };

                    // Highlight border for the selected paint.
                    if (TILE_INFO[i].ch == ed.paint) {
                        DrawRectangle(x - 2, y - 2, btnW + 4, btnH + 4, ORANGE);
                    }
                    DrawRectangleRec(r, TILE_INFO[i].color);
                    DrawRectangleLinesEx(r, 1.0f, BLACK);
                    int fs = 16;
                    int tw = MeasureText(TILE_INFO[i].name, fs);
                    DrawText(TILE_INFO[i].name,
                             x + (btnW - tw) / 2,
                             y + (btnH - fs) / 2,
                             fs, BLACK);

                    // Click selects.
                    if (CheckCollisionPointRec(mp, r) &&
                        IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        ed.paint = TILE_INFO[i].ch;
                    }

                    x += btnW + gap;
                    if (x + btnW > screenW - PANEL_W - 10) {
                        x = 10;
                        y += btnH + gap;
                    }
                }
            }

            // Selected line just below palette.
            {
                const TileInfo *sel = FindTile(ed.paint);
                char buf[64];
                snprintf(buf, sizeof(buf), "Selected: %s", sel->name);
                DrawText(buf, 10, PALETTE_H - 22, 18, MAROON);
            }

            // --- Grid -----------------------------------------------------
            for (int z = 0; z < ed.height; z++) {
                for (int x = 0; x < ed.width; x++) {
                    Rectangle r = {
                        gridX + x * cellSize,
                        gridY + z * cellSize,
                        cellSize, cellSize,
                    };
                    DrawCellAt(r, ed.grid[z][x]);
                }
            }
            // Hovered cell highlight.
            if (hoverX >= 0) {
                Rectangle r = {
                    gridX + hoverX * cellSize,
                    gridY + hoverZ * cellSize,
                    cellSize, cellSize,
                };
                DrawRectangleLinesEx(r, 3.0f, ORANGE);
            }

            // --- Right panel ----------------------------------------------
            int px = screenW - PANEL_W;
            int py = PALETTE_H + 10;

            // Hover line.
            {
                char buf[96];
                if (hoverX >= 0) {
                    const TileInfo *t = FindTile(ed.grid[hoverZ][hoverX]);
                    snprintf(buf, sizeof(buf), "Hover: (%d, %d) = %s",
                             hoverX, hoverZ, t->name);
                } else {
                    snprintf(buf, sizeof(buf), "Hover: -");
                }
                DrawText(buf, px, py, 18, DARKGRAY);
            }
            py += 32;

            // Filename
            DrawText("Filename:", px, py, 16, DARKGRAY);
            py += 20;
            Rectangle fnRect = { (float)px, (float)py, PANEL_W - 20, 32 };
            if (GuiTextBox(fnRect, ed.filename, sizeof(ed.filename), filenameEdit)) {
                filenameEdit = !filenameEdit;
            }
            py += 44;

            // Width
            DrawText("Width:", px, py, 16, DARKGRAY);
            Rectangle wRect = { (float)(px + 80), (float)py - 4, 140, 28 };
            if (GuiSpinner(wRect, NULL, &widthSpin, 3, EDITOR_MAX_W, widthEdit)) {
                widthEdit = !widthEdit;
            }
            if (widthSpin != ed.width) {
                // Resize: cells outside new bounds stay in the array but become
                // invisible / unused; new cells default to '.'.
                if (widthSpin > ed.width) {
                    for (int z = 0; z < ed.height; z++)
                        for (int x = ed.width; x < widthSpin; x++)
                            ed.grid[z][x] = '.';
                }
                ed.width = widthSpin;
            }
            py += 36;

            // Height
            DrawText("Height:", px, py, 16, DARKGRAY);
            Rectangle hRect = { (float)(px + 80), (float)py - 4, 140, 28 };
            if (GuiSpinner(hRect, NULL, &heightSpin, 3, EDITOR_MAX_H, heightEdit)) {
                heightEdit = !heightEdit;
            }
            if (heightSpin != ed.height) {
                if (heightSpin > ed.height) {
                    for (int z = ed.height; z < heightSpin; z++)
                        for (int x = 0; x < ed.width; x++)
                            ed.grid[z][x] = '.';
                }
                ed.height = heightSpin;
            }
            py += 36;

            // Step Limit — the level's move budget, written as '# steps: N'.
            DrawText("Steps:", px, py, 16, DARKGRAY);
            Rectangle sRect = { (float)(px + 80), (float)py - 4, 140, 28 };
            if (GuiSpinner(sRect, NULL, &stepsSpin, 1, 999, stepsEdit)) {
                stepsEdit = !stepsEdit;
            }
            if (stepsSpin != ed.moveLimit) {
                ed.moveLimit = stepsSpin;
            }
            py += 44;

            // Save / Load / New
            float btnW = (PANEL_W - 20 - 2 * 8) / 3.0f;
            if (GuiButton((Rectangle){ (float)px,                       (float)py, btnW, 36 }, "Save")) {
                EditorSave(&ed);
            }
            if (GuiButton((Rectangle){ (float)px + btnW + 8,            (float)py, btnW, 36 }, "Load")) {
                EditorLoad(&ed);
                widthSpin  = ed.width;
                heightSpin = ed.height;
                stepsSpin  = ed.moveLimit;
            }
            if (GuiButton((Rectangle){ (float)px + 2 * (btnW + 8),      (float)py, btnW, 36 }, "New")) {
                EditorReset(&ed);
                widthSpin  = ed.width;
                heightSpin = ed.height;
                stepsSpin  = ed.moveLimit;
            }
            py += 48;

            if (GuiButton((Rectangle){ (float)px, (float)py, PANEL_W - 20, 40 }, "Back to Menu")) {
                EndDrawing();
                return;
            }

            // --- Status bar (bottom) --------------------------------------
            DrawRectangle(0, screenH - STATUS_H, screenW, STATUS_H, LIGHTGRAY);
            DrawText(ed.status, 10, screenH - STATUS_H + 8, 16, DARKGRAY);
        EndDrawing();
    }
}
