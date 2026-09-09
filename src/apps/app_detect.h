#ifndef APP_DETECT_H
#define APP_DETECT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// apps/app_detect.h — local application detection (R2 connector probe).
//
// Answers "is this developer tool actually installed?" for the engine:
// PATH scans for CLIs, /Applications bundle checks, and a small L1
// registry of the known-tool slots (opencode, hermes, nous, ...).
// Pure POSIX libc (getenv/stat/access) — no exec, no subprocess, no
// allocation; caller-owned scratch everywhere.

// SLOT RECORD (owned by AppDetect, Rule 3 co-location): one known tool.
typedef struct AppSlot {
    const char *name;        // canonical executable/bundle name (lowercase)
    const char *displayName; // human label
    const char *note;        // what it is / where it lands; NULL when none
} AppSlot;

// The detection class — singleton handle; state lives in static rows.
typedef struct AppDetect {
    uint32_t reserved; // signature; no mutable state
} AppDetect;

// --- Constructor ---
// Returns the shared detection handle (static, zero-init, never NULL).
AppDetect *AppDetect_shared(void);

// --- Core functions (POSIX probes, no exec) ---
// Full first PATH match for a bare command name, e.g. "opencode".
// Resolves like `which` (getenv PATH, colon-separated, X_OK). Returns
// false when not found or scratch too small; true + NUL-terminated path.
bool AppDetect_which(const AppDetect *self, const char *name,
                     char *outBuf, size_t outCap);
// true when `name` resolves on PATH (AppDetect_which, any location).
bool AppDetect_isOnPath(const AppDetect *self, const char *name);
// true when an app bundle exists: /Applications/<Name>.app
// (display-name case-insensitive prefix match).
bool AppDetect_isAppBundle(const AppDetect *self, const char *name);
// Composite installed-check for a registry slot: PATH match, or a bundle
// in /Applications for the slot's display name. False on NULL args.
bool AppDetect_isInstalled(const AppDetect *self, const AppSlot *slot);

// --- Registry (L1 rows from apps/apps.inc) ---
uint32_t AppDetect_count(const AppDetect *self);
const AppSlot *AppDetect_at(const AppDetect *self, uint32_t i);
const AppSlot *AppDetect_get(const AppDetect *self, const char *name);

// --- Getters (Rule 24; null-safe; setters omitted — immutable rows) ---
const char *AppDetect_getName(const AppDetect *self, const AppSlot *slot);
const char *AppDetect_getDisplayName(const AppDetect *self, const AppSlot *slot);
const char *AppDetect_getNote(const AppDetect *self, const AppSlot *slot);

#endif