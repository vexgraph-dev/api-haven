#include "annotation/overview.h"
#include "database/db_catalog.h"

#include <stdio.h>
#include <string.h>

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: DbCatalogTest (src/database/tests/db_catalog_test.c)
 * LEVEL: L2 — Behavior verification (headless; no drivers, no network)
 * ============================================================================
 * Executable proof of the database catalog: row count, enum lookup,
 * default ports, descriptor getters, and every null-safety guard.
 *
 * Exit code 0 = all checks green; 1 = at least one check failed.
 * ============================================================================
 */

static int sFailures = 0;

#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);     \
            sFailures++;                                               \
        }                                                              \
    } while (0)

int main(int argc, const char **argv) {
    (void)argc;
    (void)argv;

    DbCatalog *catalog = DbCatalog_shared();
    CHECK(catalog != NULL);
    const uint32_t total = DbCatalog_count(catalog);
    printf("DbCatalog_count = %u\n", total);
    CHECK(total == 10);

    // --- enum lookups ----------------------------------------------------------
    const DbCatalogSlot *pg = DbCatalog_getByKind(catalog, DB_KIND_POSTGRES);
    CHECK(pg != NULL);
    CHECK(pg && strcmp(DbCatalog_getDisplayName(catalog, pg), "PostgreSQL") == 0);
    CHECK(pg && DbCatalog_getDefaultPort(catalog, pg) == 5432);
    CHECK(pg && DbCatalog_getVendor(catalog, pg) != NULL);

    const DbCatalogSlot *sqlite = DbCatalog_getByKind(catalog, DB_KIND_SQLITE);
    CHECK(sqlite && DbCatalog_getDefaultPort(catalog, sqlite) == 0);

    const DbCatalogSlot *redis = DbCatalog_getByKind(catalog, DB_KIND_REDIS);
    CHECK(redis && DbCatalog_getDefaultPort(catalog, redis) == 6379);

    const DbCatalogSlot *mongo = DbCatalog_getByKind(catalog, DB_KIND_MONGODB);
    CHECK(mongo && DbCatalog_getDefaultPort(catalog, mongo) == 27017);

    CHECK(DbCatalog_getByKind(catalog, (DbKind)999) == NULL);

    // --- positional + consistency ----------------------------------------------
    CHECK(DbCatalog_at(catalog, 0) != NULL);
    CHECK(DbCatalog_at(catalog, total) == NULL);
    CHECK(DbCatalog_at(catalog, total + 3) == NULL);

    // --- null-safety (Rule 24) ------------------------------------------------
    CHECK(DbCatalog_count(NULL) == 0);
    CHECK(DbCatalog_getByKind(NULL, DB_KIND_POSTGRES) == NULL);
    CHECK(DbCatalog_at(NULL, 0) == NULL);
    CHECK(DbCatalog_getKind(NULL, NULL) == DB_KIND_SQLITE);
    CHECK(DbCatalog_getDisplayName(NULL, NULL) == NULL);
    CHECK(DbCatalog_getDefaultPort(NULL, NULL) == 0);
    CHECK(DbCatalog_getVendor(NULL, NULL) == NULL);
    CHECK(DbCatalog_getNote(NULL, NULL) == NULL);

    if (sFailures == 0) {
        printf("db_catalog_test: ALL CHECKS PASSED (%u kinds)\n", total);
        return 0;
    }
    printf("db_catalog_test: %d FAILURES\n", sFailures);
    return 1;
}