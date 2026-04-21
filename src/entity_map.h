#ifndef ENTITY_MAP_H
#define ENTITY_MAP_H

#include <stdbool.h>
#include "platform.h"

#define ENTITY_NAME_LEN   32
#define ENTITY_MAP_SLOTS  64   // power of two — must stay a power of two for `& (SLOTS-1)` masking

typedef struct {
    bool         present;        // is THIS entity on this tile?
    PlatformType platformType;   // what kind of tile sits here (or PLATFORM_NONE)
} TileCell;

typedef struct {
    bool      used;
    char      key[ENTITY_NAME_LEN];
    TileCell *cells;             // heap-allocated, gridWidth * gridHeight
} EntityMapSlot;

typedef struct {
    EntityMapSlot slots[ENTITY_MAP_SLOTS];
    int           gridWidth;
    int           gridHeight;
} EntityMap;

void      EntityMapInit(EntityMap *m, int gridWidth, int gridHeight);
void      EntityMapFree(EntityMap *m);

// Create (or reuse) an entry for `name`. Cells start present=false, platformType=PLATFORM_NONE.
TileCell *EntityMapPut(EntityMap *m, const char *name);

// Returns the cell array for `name`, or NULL.
TileCell *EntityMapGet(const EntityMap *m, const char *name);

void EntityMapSetPlatform(EntityMap *m, const char *name, int x, int z, PlatformType t);
void EntityMapSetPresent (EntityMap *m, const char *name, int x, int z, bool present);
bool EntityMapIsPresent  (const EntityMap *m, const char *name, int x, int z);

// True if any entry whose key starts with `prefix` has present=true at (x, z).
bool EntityMapAnyPresentWithPrefixAt(const EntityMap *m, const char *prefix, int x, int z);

#endif
