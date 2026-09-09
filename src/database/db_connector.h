#ifndef DB_CONNECTOR_H
#define DB_CONNECTOR_H

#include <stdbool.h>
#include <stddef.h>

#include "database/db_catalog.h"

// database/db_connector.h — the database connector contract (L2 shape).
//
// A DbConnector is a fn-pointer client: the driver implementation lives
// OUTSIDE api-haven (db-haven/darkbase, app-side SQLite, custom mocks)
// and registers open/query/close backends on the struct, then hands the
// handle around. This is the connector layer's "registers ◀ opaque
// handles + callbacks" doctrine from Rule 17 — api-haven defines the
// SHAPE, never the engines.

// Backend signatures (dest-last per Rule 9; caller-owned scratch).
// open:      establish the backend session; false = unavailable/failed.
// query:     run one SQL/text command; results land in outBuf (NUL-
//            terminated); false = command rejected or buffer too small.
// close:     release the backend session.
typedef bool (*DbOpenFn)(void *userdata);
typedef bool (*DbQueryFn)(void *userdata, const char *text,
                          char *outBuf, size_t outCap);
typedef void (*DbCloseFn)(void *userdata);

typedef struct DbConnector {
    DbKind kind;      // which store family this backend serves
    DbOpenFn open;    // NULL = unsupported (open fails)
    DbQueryFn query;  // NULL = unsupported (query fails)
    DbCloseFn close;  // NULL = unsupported (close is a no-op)
    void *userdata;   // backend-owned state; opaque to this layer
} DbConnector;

// --- Constructors (value structs, no allocation) ---
// All backends NULL, kind SQLITE, userdata NULL (harmless no-op handle).
DbConnector DbConnector_0(void);
// Populated handle: kind + three backends + opaque state.
DbConnector DbConnector_wrap(DbKind kind,
                             DbOpenFn open, DbQueryFn query, DbCloseFn close,
                             void *userdata);

// --- Core functions (null-safe delegation) ---
bool DbConnector_open(const DbConnector *self);
bool DbConnector_query(const DbConnector *self, const char *text,
                       char *outBuf, size_t outCap);
void DbConnector_close(const DbConnector *self);

// --- Setters ---
void DbConnector_setKind(DbConnector *self, DbKind kind);
void DbConnector_setOpen(DbConnector *self, DbOpenFn open);
void DbConnector_setQuery(DbConnector *self, DbQueryFn query);
void DbConnector_setClose(DbConnector *self, DbCloseFn close);
void DbConnector_setUserdata(DbConnector *self, void *userdata);

// --- Getters (Rule 24, null-safe) ---
DbKind DbConnector_getKind(const DbConnector *self);
DbOpenFn DbConnector_getOpen(const DbConnector *self);
DbQueryFn DbConnector_getQuery(const DbConnector *self);
DbCloseFn DbConnector_getClose(const DbConnector *self);
void *DbConnector_getUserdata(const DbConnector *self);

#endif