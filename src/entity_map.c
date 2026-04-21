#include "entity_map.h"
#include "raylib.h"
#include <string.h>

#define SLOT_MASK (ENTITY_MAP_SLOTS - 1)

static unsigned int HashKey(const char *key) {
    int len = (int)strlen(key);
    return ComputeCRC32((unsigned char *)key, len);
}

// Core probe: walk from the hash-derived start slot, stopping at the matching
// key or the first empty slot. Returns the slot index; `*found` tells the
// caller which one it is. On a full map with no match this would loop forever
// — but we size the table so the load factor stays well under 50%, so there's
// always an empty slot within reach.
static int FindSlot(const EntityMap *m, const char *key, bool *found) {
    unsigned int h = HashKey(key);
    int start = (int)(h & SLOT_MASK);
    for (int i = 0; i < ENTITY_MAP_SLOTS; i++) {
        int idx = (start + i) & SLOT_MASK;
        if (!m->slots[idx].used) {
            *found = false;
            return idx;
        }
        if (strncmp(m->slots[idx].key, key, ENTITY_NAME_LEN) == 0) {
            *found = true;
            return idx;
        }
    }
    *found = false;
    return start;
}

void EntityMapInit(EntityMap *m, int gridWidth, int gridHeight) {
    for (int i = 0; i < ENTITY_MAP_SLOTS; i++) {
        m->slots[i].used = false;
        m->slots[i].key[0] = '\0';
        m->slots[i].cells = NULL;
    }
    m->gridWidth  = gridWidth;
    m->gridHeight = gridHeight;
}

void EntityMapFree(EntityMap *m) {
    for (int i = 0; i < ENTITY_MAP_SLOTS; i++) {
        if (m->slots[i].used && m->slots[i].cells != NULL) {
            MemFree(m->slots[i].cells);
        }
        m->slots[i].used = false;
        m->slots[i].cells = NULL;
    }
    m->gridWidth = 0;
    m->gridHeight = 0;
}

TileCell *EntityMapPut(EntityMap *m, const char *name) {
    bool found;
    int idx = FindSlot(m, name, &found);

    if (!found) {
        m->slots[idx].used = true;
        strncpy(m->slots[idx].key, name, ENTITY_NAME_LEN - 1);
        m->slots[idx].key[ENTITY_NAME_LEN - 1] = '\0';

        int cellCount = m->gridWidth * m->gridHeight;
        m->slots[idx].cells = (TileCell *)MemAlloc(cellCount * sizeof(TileCell));
        for (int i = 0; i < cellCount; i++) {
            m->slots[idx].cells[i].present      = false;
            m->slots[idx].cells[i].platformType = PLATFORM_NONE;
        }
    }

    return m->slots[idx].cells;
}

TileCell *EntityMapGet(const EntityMap *m, const char *name) {
    bool found;
    int idx = FindSlot(m, name, &found);
    return found ? m->slots[idx].cells : NULL;
}

static bool InBounds(const EntityMap *m, int x, int z) {
    return x >= 0 && x < m->gridWidth && z >= 0 && z < m->gridHeight;
}

void EntityMapSetPlatform(EntityMap *m, const char *name, int x, int z, PlatformType t) {
    if (!InBounds(m, x, z)) return;
    TileCell *cells = EntityMapGet(m, name);
    if (cells == NULL) return;
    cells[z * m->gridWidth + x].platformType = t;
}

void EntityMapSetPresent(EntityMap *m, const char *name, int x, int z, bool present) {
    if (!InBounds(m, x, z)) return;
    TileCell *cells = EntityMapGet(m, name);
    if (cells == NULL) return;
    cells[z * m->gridWidth + x].present = present;
}

bool EntityMapIsPresent(const EntityMap *m, const char *name, int x, int z) {
    if (!InBounds(m, x, z)) return false;
    TileCell *cells = EntityMapGet(m, name);
    if (cells == NULL) return false;
    return cells[z * m->gridWidth + x].present;
}

bool EntityMapAnyPresentWithPrefixAt(const EntityMap *m, const char *prefix, int x, int z) {
    if (!InBounds(m, x, z)) return false;
    int prefixLen = (int)strlen(prefix);
    int tileIdx = z * m->gridWidth + x;

    for (int i = 0; i < ENTITY_MAP_SLOTS; i++) {
        if (!m->slots[i].used) continue;
        if (strncmp(m->slots[i].key, prefix, prefixLen) != 0) continue;
        if (m->slots[i].cells[tileIdx].present) return true;
    }
    return false;
}
