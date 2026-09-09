#ifndef DB_CATALOG_H
#define DB_CATALOG_H

#include <stdint.h>

// database/db_catalog.h — the database-kind directory (L1 descriptors).
//
// One behavior class owns the catalog of store kinds the ecosystem may
// integrate: name, vendor, default endpoint port, and a note. Rows are
// static const slot records (see db_catalog.c); this header declares the
// schema and the table API. Pure descriptors — no drivers, no includes
// beyond vexspoke; actual storage engines live in db-haven/darkbase
// (Rule 17: their `Database` interface is untouched by these shapes).

// Store wire families (driver contract groups, not engine implementations).
typedef enum DbKind {
    DB_KIND_SQLITE,
    DB_KIND_POSTGRES,
    DB_KIND_MYSQL,
    DB_KIND_MARIADB,
    DB_KIND_REDIS,
    DB_KIND_DUCKDB,
    DB_KIND_SQLSERVER,
    DB_KIND_MONGODB,
    DB_KIND_ELASTICSEARCH,
    DB_KIND_NEO4J,
} DbKind;

// SLOT RECORD — one row of the catalog (Rule 3 co-location).
// Immutable: rows are static const data in db_catalog.c; all behavior
// hangs off the DbCatalog table class (Rule 33 Tier-2 waiver, see
// ;;INTENTION in db_catalog.c).
typedef struct DbCatalogSlot {
    DbKind kind;            // enum key; the row's primary index
    const char *displayName; // human label, e.g. "PostgreSQL"
    uint16_t defaultPort;   // canonical wire port (0 = embedded/unknown)
    const char *vendor;     // primary vendor, e.g. "PostgreSQL Global Dev Group"
    const char *note;       // driver/wire caveat; NULL when none
} DbCatalogSlot;

// The catalog class — a singleton handle; state lives in static const
// rows generated under src/database/data/ (see db_catalog.c overview).
typedef struct DbCatalog {
    uint32_t reserved; // signature/marker; no mutable state
} DbCatalog;

// --- Constructor ---
// Returns the shared catalog handle (static, zero-init, never NULL).
DbCatalog *DbCatalog_shared(void);

// --- Core functions ---
// Total rows in the catalog. Null-safe: 0 on NULL self.
uint32_t DbCatalog_count(const DbCatalog *self);
// Row at index i. NULL when out of range.
const DbCatalogSlot *DbCatalog_at(const DbCatalog *self, uint32_t i);
// Row by enum kind. NULL when absent.
const DbCatalogSlot *DbCatalog_getByKind(const DbCatalog *self, DbKind kind);

// --- Getters (Rule 24; null-safe; setters omitted — immutable rows) ---
DbKind DbCatalog_getKind(const DbCatalog *self, const DbCatalogSlot *slot);
const char *DbCatalog_getDisplayName(const DbCatalog *self, const DbCatalogSlot *slot);
uint16_t DbCatalog_getDefaultPort(const DbCatalog *self, const DbCatalogSlot *slot);
const char *DbCatalog_getVendor(const DbCatalog *self, const DbCatalogSlot *slot);
const char *DbCatalog_getNote(const DbCatalog *self, const DbCatalogSlot *slot);

#endif