#include "annotation/intention.h"
#include "annotation/overview.h"
#include "database/db_catalog.h"

#include <stddef.h>

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: DbCatalog (database/db_catalog)
 * LEVEL: L1 — File Metadata (Rule 28: declarative descriptors, swappable
 * with zero code changes: edit rows, never touch logic)
 * ============================================================================
 * The database-kind directory: 10 static descriptor rows covering the
 * store families the engine may integrate — name, vendor, default wire
 * port, and a note. One table, linear enum lookup, zero allocation,
 * immutable (thread-safe reads without locks). Descriptors only: the
 * actual storage engines and their `Database` interface live in
 * db-haven/darkbase (Rule 17); these shapes are api-haven's connector
 * contract layer and never substitute for them.
 *
 * STRUCT FIELDS (Mirroring database/db_catalog.h — exactly this file's class):
 * ----------------------------------------------------------------------------
 *   uint32_t reserved;   // singleton marker; no mutable state — all data
 *                        // lives in the static const DbCatalogSlot rows
 *
 * SLOT RECORD (DbCatalogSlot — Rule 3 co-location, zero behavior of its own:
 * all query behavior hangs off this catalog class):
 * ----------------------------------------------------------------------------
 *   DbKind kind;            // enum key; primary index
 *   const char *displayName; // human label
 *   uint16_t defaultPort;   // canonical wire port; 0 = embedded/unknown
 *   const char *vendor;     // primary vendor
 *   const char *note;       // driver/wire caveat; NULL when none
 *
 * PRIVATE HELPERS (data only, full field list per row is the SLOT RECORD
 * above; each row carries the same 5 fields):
 * ----------------------------------------------------------------------------
 *   kDbKinds[] — src/database/data/db_kinds.inc (10 rows: SQLITE, POSTGRES,
 *                MYSQL, MARIADB, REDIS, DUCKDB, SQLSERVER, MONGODB,
 *                ELASTICSEARCH, NEO4J)
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Constructor:
 *   - DbCatalog_shared()  : returns the singleton catalog handle
 *
 * Core Functions:
 *   - DbCatalog_count(self)             : total rows (10)
 *   - DbCatalog_at(self, i)             : row at index i
 *   - DbCatalog_getByKind(self, kind)   : row by enum kind
 *
 * Getters (Rule 24; null-safe. Setters omitted — immutable rows, waiver):
 *   - DbCatalog_getKind / getDisplayName / getDefaultPort / getVendor /
 *     getNote(self, slot)
 * ============================================================================
 */
;;INTENTION("api-haven defines connector contracts (descriptor registries + fn-pointer clients) with zero database engine includes; actual storage and the Database interface live in db-haven/darkbase under Rule 17 — see preferences.md Rule 17 api-haven clause")
;;INTENTION("directory name database/ mirrors the user-directed connector layer; class names DbCatalog/DbConnector never collide with db-haven's Database interface")
;;INTENTION("immutable slot records — setters omitted; data is static const; access via DbCatalog_* table functions — Rule 33 Tier-2 waiver")

#include "database/data/db_kinds.inc"

static const uint32_t kDbKindCount =
    (uint32_t)(sizeof(kDbKinds) / sizeof(kDbKinds[0]));

static DbCatalog sDbCatalogShared; // zero-init singleton

// CONSTRUCTORS

DbCatalog *DbCatalog_shared(void) {
    return &sDbCatalogShared;
}

// CORE FUNCTIONS

uint32_t DbCatalog_count(const DbCatalog *self) {
    if (!self)
        return 0;
    return kDbKindCount;
}

const DbCatalogSlot *DbCatalog_at(const DbCatalog *self, uint32_t i) {
    if (!self || i >= kDbKindCount)
        return NULL;
    return &kDbKinds[i];
}

const DbCatalogSlot *DbCatalog_getByKind(const DbCatalog *self, DbKind kind) {
    if (!self)
        return NULL;
    for (uint32_t i = 0; i < kDbKindCount; i++) {
        if (kDbKinds[i].kind == kind)
            return &kDbKinds[i];
    }
    return NULL;
}

// GETTERS

DbKind DbCatalog_getKind(const DbCatalog *self, const DbCatalogSlot *slot) {
    if (!self || !slot)
        return DB_KIND_SQLITE; // safe default
    return (*slot).kind;
}

const char *DbCatalog_getDisplayName(const DbCatalog *self, const DbCatalogSlot *slot) {
    (void)self;
    return slot ? (*slot).displayName : NULL;
}

uint16_t DbCatalog_getDefaultPort(const DbCatalog *self, const DbCatalogSlot *slot) {
    if (!self || !slot)
        return 0;
    return (*slot).defaultPort;
}

const char *DbCatalog_getVendor(const DbCatalog *self, const DbCatalogSlot *slot) {
    (void)self;
    return slot ? (*slot).vendor : NULL;
}

const char *DbCatalog_getNote(const DbCatalog *self, const DbCatalogSlot *slot) {
    (void)self;
    return slot ? (*slot).note : NULL;
}