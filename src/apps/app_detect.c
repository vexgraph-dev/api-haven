#include "annotation/intention.h"
#include "annotation/overview.h"
#include "apps/app_detect.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: AppDetect (apps/app_detect)
 * LEVEL: L1 — File Metadata (declarative descriptors + trivial POSIX probes;
 * swappable/extendable with zero code changes)
 * ============================================================================
 * Local application detection for the engine: "is opencode installed?"
 * via PATH resolution and /Applications bundle scans, over a small L1
 * registry of known developer tools. No exec, no subprocess, no
 * allocation — pure getenv/access probes into caller-owned scratch.
 *
 * STRUCT FIELDS (Mirroring apps/app_detect.h — exactly this file's class):
 * ----------------------------------------------------------------------------
 *   uint32_t reserved;   // singleton marker; no mutable state — all data
 *                        // lives in the static const AppSlot rows
 *
 * SLOT RECORD (AppSlot — Rule 3 co-location, zero behavior of its own):
 * ----------------------------------------------------------------------------
 *   const char *name;         // canonical executable/bundle name
 *   const char *displayName;  // human label
 *   const char *note;         // what it is / where it lands; NULL when none
 *
 * PRIVATE HELPERS (data only, full field list per row is the SLOT RECORD
 * above; each row carries the same 3 fields):
 * ----------------------------------------------------------------------------
 *   kKnownApps[] — src/apps/apps.inc (6 rows: opencode, codex, claude,
 *                  cursor, hermes, nous)
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Constructor:
 *   - AppDetect_shared() : returns the singleton detection handle
 *
 * Core Functions (POSIX probes, no exec):
 *   - AppDetect_which(self, name, outBuf, cap)      : first PATH match
 *   - AppDetect_isOnPath(self, name)                : PATH presence
 *   - AppDetect_isAppBundle(self, name)             : /Applications bundle
 *   - AppDetect_isInstalled(self, slot)             : PATH or bundle
 *   - AppDetect_count / at / get(self, ...)         : registry access
 *
 * Getters (Rule 24; null-safe. Setters omitted — immutable rows, waiver):
 *   - AppDetect_getName / getDisplayName / getNote(self, slot)
 * ============================================================================
 */
;;INTENTION("local app detection lives in api-haven (connector hub) rather than vexspoke R1 system/ — user-directed placement; a one-file move later is trivial")
;;INTENTION("probes are PATH-scan + bundle-scan only; version probing needs fork/exec and is deferred to a later cycle (vexspoke process mgmt)")
;;INTENTION("immutable registry rows — setters omitted; access via AppDetect_* table functions — Rule 33 Tier-2 waiver")

#include "apps/apps.inc"

static const uint32_t kKnownAppCount =
    (uint32_t)(sizeof(kKnownApps) / sizeof(kKnownApps[0]));

static AppDetect sAppDetectShared; // zero-init singleton

// CONSTRUCTORS

AppDetect *AppDetect_shared(void) {
    return &sAppDetectShared;
}

// CORE FUNCTIONS

bool AppDetect_which(const AppDetect *self, const char *name,
                     char *outBuf, size_t outCap) {
    if (!self || !name || (*name) == '\0' || !outBuf || outCap == 0)
        return false;
    const char *path = getenv("PATH");
    if (!path)
        return false;
    const size_t nameLen = strlen(name);
    const char *dir = path;
    for (;;) {
        const char *sep = strchr(dir, ':');
        const size_t dirLen = sep ? (size_t)(sep - dir) : strlen(dir);
        if (dirLen + 1 + nameLen + 1 <= outCap) {
            memcpy(outBuf, dir, dirLen);
            outBuf[dirLen] = '/';
            memcpy(outBuf + dirLen + 1, name, nameLen);
            outBuf[dirLen + 1 + nameLen] = '\0';
            if (access(outBuf, X_OK) == 0)
                return true;
        }
        if (!sep)
            break;
        dir = sep + 1;
    }
    if (outCap > 0)
        outBuf[0] = '\0';
    return false;
}

bool AppDetect_isOnPath(const AppDetect *self, const char *name) {
    char scratch[4096];
    return AppDetect_which(self, name, scratch, sizeof(scratch));
}

bool AppDetect_isAppBundle(const AppDetect *self, const char *name) {
    if (!self || !name || (*name) == '\0')
        return false;
    // Exact bundle as given, then display-style case of the first letter.
    char bundle[512];
    const size_t n = strlen(name);
    if (n + 6 > sizeof(bundle))
        return false; // "/Applications/" + name + ".app"
    memcpy(bundle, "/Applications/", 14);
    memcpy(bundle + 14, name, n);
    memcpy(bundle + 14 + n, ".app", 5); // includes NUL
    if (access(bundle, F_OK) == 0)
        return true;
    if (n + 6 > sizeof(bundle))
        return false;
    bundle[14] = (char)toupper((unsigned char)bundle[14]);
    return access(bundle, F_OK) == 0;
}

bool AppDetect_isInstalled(const AppDetect *self, const AppSlot *slot) {
    if (!self || !slot)
        return false;
    if (AppDetect_isOnPath(self, (*slot).name))
        return true;
    return AppDetect_isAppBundle(self, (*slot).name);
}

uint32_t AppDetect_count(const AppDetect *self) {
    if (!self)
        return 0;
    return kKnownAppCount;
}

const AppSlot *AppDetect_at(const AppDetect *self, uint32_t i) {
    if (!self || i >= kKnownAppCount)
        return NULL;
    return &kKnownApps[i];
}

const AppSlot *AppDetect_get(const AppDetect *self, const char *name) {
    if (!self || !name)
        return NULL;
    for (uint32_t i = 0; i < kKnownAppCount; i++) {
        if (strcmp(kKnownApps[i].name, name) == 0)
            return &kKnownApps[i];
    }
    return NULL;
}

// GETTERS

const char *AppDetect_getName(const AppDetect *self, const AppSlot *slot) {
    (void)self;
    return slot ? (*slot).name : NULL;
}

const char *AppDetect_getDisplayName(const AppDetect *self, const AppSlot *slot) {
    (void)self;
    return slot ? (*slot).displayName : NULL;
}

const char *AppDetect_getNote(const AppDetect *self, const AppSlot *slot) {
    (void)self;
    return slot ? (*slot).note : NULL;
}