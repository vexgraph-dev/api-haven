#include "annotation/overview.h"
#include "database/db_connector.h"

#include <stdio.h>
#include <string.h>

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: DbConnectorTest (src/database/tests/db_connector_test.c)
 * LEVEL: L2 — Behavior verification (headless; fake backend, no drivers,
 * no network)
 * ============================================================================
 * Executable proof of the connector contract: wrap() construction, opaque
 * userdata round-trip, open/query/close delegation through the three
 * function pointers, setter mutations, and every null-safety guard.
 * The fake backend proves the "registers opaque handles + callbacks"
 * doctrine from outside api-haven (what db-haven/darkbase will do).
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

typedef struct FakeState {
    int opens;
    int queries;
    int closes;
    char lastText[64];
} FakeState;

static bool fakeOpen(void *userdata) {
    FakeState *s = (FakeState*) userdata;
    (*s).opens++;
    return true;
}

static bool fakeQuery(void *userdata, const char *text,
                      char *outBuf, size_t outCap) {
    FakeState *s = (FakeState*) userdata;
    (*s).queries++;
    snprintf((*s).lastText, sizeof((*s).lastText), "%s", text);
    const size_t n = strlen(text);
    if (n + 1 > outCap)
        return false;
    snprintf(outBuf, outCap, "rows:%zu", n);
    return true;
}

static void fakeClose(void *userdata) {
    FakeState *s = (FakeState*) userdata;
    (*s).closes++;
}

int main(int argc, const char **argv) {
    (void)argc;
    (void)argv;

    FakeState state;
    memset(&state, 0, sizeof(state));

    // --- wrap + getter round-trip ---------------------------------------------
    DbConnector c = DbConnector_wrap(DB_KIND_REDIS, fakeOpen, fakeQuery, fakeClose,
                                     &state);
    CHECK(DbConnector_getKind(&c) == DB_KIND_REDIS);
    CHECK(DbConnector_getOpen(&c) == fakeOpen);
    CHECK(DbConnector_getQuery(&c) == fakeQuery);
    CHECK(DbConnector_getClose(&c) == fakeClose);
    CHECK(DbConnector_getUserdata(&c) == &state);

    // --- delegation through the handle ----------------------------------------
    CHECK(DbConnector_open(&c));
    char out[128];
    CHECK(DbConnector_query(&c, "SELECT 1", out, sizeof(out)));
    CHECK(strcmp(out, "rows:8") == 0);
    DbConnector_close(&c);
    CHECK(state.opens == 1 && state.queries == 1 && state.closes == 1);
    CHECK(strcmp(state.lastText, "SELECT 1") == 0);

    // --- backend failure paths -------------------------------------------------
    CHECK(!DbConnector_query(&c, "SELECT 1", out, 4)); // outCap too small

    // --- empty connector: harmless no-ops --------------------------------------
    DbConnector empty = DbConnector_0();
    CHECK(DbConnector_getKind(&empty) == DB_KIND_SQLITE);
    CHECK(DbConnector_getOpen(&empty) == NULL);
    CHECK(!DbConnector_open(&empty));
    CHECK(!DbConnector_query(&empty, "x", out, sizeof(out)));
    DbConnector_close(&empty); // must not crash

    // --- setter mutation --------------------------------------------------------
    DbConnector built = DbConnector_0();
    DbConnector_setKind(&built, DB_KIND_POSTGRES);
    DbConnector_setOpen(&built, fakeOpen);
    DbConnector_setQuery(&built, fakeQuery);
    DbConnector_setClose(&built, fakeClose);
    DbConnector_setUserdata(&built, &state);
    CHECK(DbConnector_getKind(&built) == DB_KIND_POSTGRES);
    CHECK(DbConnector_getUserdata(&built) == &state);
    DbConnector_setOpen(&built, NULL);
    CHECK(!DbConnector_open(&built));
    DbConnector_setOpen(&built, fakeOpen);
    CHECK(DbConnector_open(&built));

    // --- null-safety (Rule 24) --------------------------------------------------
    CHECK(DbConnector_getKind(NULL) == DB_KIND_SQLITE);
    CHECK(DbConnector_getOpen(NULL) == NULL);
    CHECK(DbConnector_getQuery(NULL) == NULL);
    CHECK(DbConnector_getClose(NULL) == NULL);
    CHECK(DbConnector_getUserdata(NULL) == NULL);
    CHECK(!DbConnector_open(NULL));
    CHECK(!DbConnector_query(NULL, "x", out, sizeof(out)));
    CHECK(!DbConnector_query(&c, NULL, out, sizeof(out)));
    CHECK(!DbConnector_query(&c, "x", NULL, 0));
    DbConnector_close(NULL); // must not crash

    if (sFailures == 0) {
        printf("db_connector_test: ALL CHECKS PASSED\n");
        return 0;
    }
    printf("db_connector_test: %d FAILURES\n", sFailures);
    return 1;
}