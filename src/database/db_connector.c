#include "annotation/intention.h"
#include "annotation/overview.h"
#include "database/db_connector.h"

#include <string.h>

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: DbConnector (database/db_connector)
 * LEVEL: L2 — Behavior (class API surface: constructors, core delegation,
 * setters, getters; consumes the L1 DbCatalog shape by kind)
 * ============================================================================
 * The connector contract layer of api-haven: a DbConnector holds a kind,
 * three backend function pointers, and opaque userdata, and delegates
 * open/query/close with null-safety. Real drivers register backends from
 * outside (db-haven/darkbase, vendored SQLite, test mocks); this class is
 * the shape they implement — Rule 17's registers doctrine at the
 * connector level, zero database engine includes.
 *
 * STRUCT FIELDS (Mirroring database/db_connector.h — exactly this file's
 * class):
 * ----------------------------------------------------------------------------
 *   DbKind kind;      // which store family this backend serves
 *   DbOpenFn open;    // NULL = unsupported (open fails)
 *   DbQueryFn query;  // NULL = unsupported (query fails)
 *   DbCloseFn close;  // NULL = unsupported (close is a no-op)
 *   void *userdata;   // backend-owned state; opaque to this layer
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Constructors (value structs — ApiAuth/DbConnector precedent):
 *   - DbConnector_0()                       : empty, harmless no-op handle
 *   - DbConnector_wrap(kind, open, query, close, userdata)
 *
 * Core Functions (null-safe delegation):
 *   - DbConnector_open(self)                : backend open || false
 *   - DbConnector_query(self, text, outBuf, outCap)
 *                                            : backend query || false
 *   - DbConnector_close(self)               : backend close, else no-op
 *
 * Setters:
 *   - DbConnector_setKind / setOpen / setQuery / setClose / setUserdata
 *
 * Getters:
 *   - DbConnector_getKind / getOpen / getQuery / getClose / getUserdata
 * ============================================================================
 */
;;INTENTION("fn-pointer connector shape only — real drivers live in db-haven/darkbase; api-haven never includes or links a database engine — preferences.md Rule 17 api-haven clause")

// CONSTRUCTORS

DbConnector DbConnector_0(void) {
    DbConnector c;
    memset(&c, 0, sizeof(c));
    return c;
}

DbConnector DbConnector_wrap(DbKind kind,
                             DbOpenFn open, DbQueryFn query, DbCloseFn close,
                             void *userdata) {
    DbConnector c = DbConnector_0();
    c.kind = kind;
    c.open = open;
    c.query = query;
    c.close = close;
    c.userdata = userdata;
    return c;
}

// CORE FUNCTIONS

bool DbConnector_open(const DbConnector *self) {
    if (!self || !(*self).open)
        return false;
    return (*self).open((*self).userdata);
}

bool DbConnector_query(const DbConnector *self, const char *text,
                       char *outBuf, size_t outCap) {
    if (!self || !(*self).query || !text || !outBuf || outCap == 0)
        return false;
    return (*self).query((*self).userdata, text, outBuf, outCap);
}

void DbConnector_close(const DbConnector *self) {
    if (self && (*self).close)
        (*self).close((*self).userdata);
}

// SETTERS

void DbConnector_setKind(DbConnector *self, DbKind kind) {
    if (self)
        (*self).kind = kind;
}

void DbConnector_setOpen(DbConnector *self, DbOpenFn open) {
    if (self)
        (*self).open = open;
}

void DbConnector_setQuery(DbConnector *self, DbQueryFn query) {
    if (self)
        (*self).query = query;
}

void DbConnector_setClose(DbConnector *self, DbCloseFn close) {
    if (self)
        (*self).close = close;
}

void DbConnector_setUserdata(DbConnector *self, void *userdata) {
    if (self)
        (*self).userdata = userdata;
}

// GETTERS

DbKind DbConnector_getKind(const DbConnector *self) {
    return self ? (*self).kind : DB_KIND_SQLITE; // safe default
}

DbOpenFn DbConnector_getOpen(const DbConnector *self) {
    return self ? (*self).open : NULL;
}

DbQueryFn DbConnector_getQuery(const DbConnector *self) {
    return self ? (*self).query : NULL;
}

DbCloseFn DbConnector_getClose(const DbConnector *self) {
    return self ? (*self).close : NULL;
}

void *DbConnector_getUserdata(const DbConnector *self) {
    return self ? (*self).userdata : NULL;
}