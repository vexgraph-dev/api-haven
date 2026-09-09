#include "annotation/overview.h"
#include "database/db_provider.h"

#include <stdio.h>
#include <string.h>

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: DbProviderTest (src/database/tests/db_provider_test.c)
 * LEVEL: L2 — Behavior verification (headless; descriptors only, no
 * drivers, no network)
 * ============================================================================
 * Executable proof of the database data-source directory: row count,
 * slug/engine lookups, wire families and ports for representative rows,
 * positional access, and every null-safety guard.
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

    DbProvider *db = DbProvider_shared();
    CHECK(db != NULL);
    const uint32_t total = DbProvider_count(db);
    printf("DbProvider_count = %u\n", total);
    CHECK(total == 25);

    // --- slug lookups ----------------------------------------------------------
    const DbProviderSlot *pg = DbProvider_get(db, "postgresql");
    CHECK(pg != NULL);
    CHECK(pg && strcmp(DbProvider_getDisplayName(db, pg), "PostgreSQL") == 0);
    CHECK(pg && strcmp(DbProvider_getEngine(db, pg), "postgres") == 0);
    CHECK(pg && DbProvider_getDefaultPort(db, pg) == 5432);
    CHECK(pg && DbProvider_getFamily(db, pg) == DB_PROVIDER_FAMILY_SQL);

    const DbProviderSlot *mysql = DbProvider_get(db, "mysql");
    CHECK(mysql && DbProvider_getDefaultPort(db, mysql) == 3306);
    CHECK(mysql && DbProvider_getFamily(db, mysql) == DB_PROVIDER_FAMILY_SQL);

    const DbProviderSlot *dynamo = DbProvider_get(db, "amazon-dynamodb");
    CHECK(dynamo && DbProvider_getFamily(db, dynamo) == DB_PROVIDER_FAMILY_NO_SQL);
    CHECK(dynamo && DbProvider_getDefaultPort(db, dynamo) == 0); // SDK-only

    const DbProviderSlot *h2 = DbProvider_get(db, "h2");
    CHECK(h2 && DbProvider_getFamily(db, h2) == DB_PROVIDER_FAMILY_EMBEDDED);

    const DbProviderSlot *bq = DbProvider_get(db, "google-bigquery");
    CHECK(bq && DbProvider_getFamily(db, bq) == DB_PROVIDER_FAMILY_SQL_COMPAT);

    CHECK(DbProvider_get(db, "amazon-aurora-mysql") != NULL);
    CHECK(DbProvider_get(db, "ibm-db2-luw") != NULL);
    CHECK(DbProvider_get(db, "definitely-not-a-db") == NULL);

    // --- engine lookups --------------------------------------------------------
    const DbProviderSlot *sqlserver = DbProvider_findByEngine(db, "sqlserver");
    CHECK(sqlserver != NULL);
    CHECK(sqlserver && strcmp(DbProvider_getSlug(db, sqlserver),
                              "azure-sql-database") == 0);
    CHECK(DbProvider_findByEngine(db, "postgres") != NULL);
    CHECK(DbProvider_findByEngine(db, "definitely-not-an-engine") == NULL);

    // --- positional + consistency ----------------------------------------------
    CHECK(DbProvider_at(db, 0) != NULL);
    CHECK(DbProvider_at(db, total) == NULL);
    for (uint32_t i = 0; i < total; i++) {
        const DbProviderSlot *row = DbProvider_at(db, i);
        CHECK(row != NULL);
        CHECK(row && DbProvider_getSlug(db, row) != NULL);
        CHECK(row && DbProvider_getDisplayName(db, row) != NULL);
        CHECK(row && DbProvider_getEngine(db, row) != NULL);
    }

    // --- null-safety (Rule 24) ------------------------------------------------
    CHECK(DbProvider_count(NULL) == 0);
    CHECK(DbProvider_get(NULL, NULL) == NULL);
    CHECK(DbProvider_get(db, NULL) == NULL);
    CHECK(DbProvider_findByEngine(NULL, NULL) == NULL);
    CHECK(DbProvider_at(NULL, 0) == NULL);
    CHECK(DbProvider_getSlug(NULL, NULL) == NULL);
    CHECK(DbProvider_getDisplayName(NULL, NULL) == NULL);
    CHECK(DbProvider_getEngine(NULL, NULL) == NULL);
    CHECK(DbProvider_getNote(NULL, NULL) == NULL);
    CHECK(DbProvider_getDefaultPort(NULL, NULL) == 0);
    CHECK(DbProvider_getFamily(NULL, NULL) == DB_PROVIDER_FAMILY_SQL);

    if (sFailures == 0) {
        printf("db_provider_test: ALL CHECKS PASSED (%u data sources)\n", total);
        return 0;
    }
    printf("db_provider_test: %d FAILURES\n", sFailures);
    return 1;
}