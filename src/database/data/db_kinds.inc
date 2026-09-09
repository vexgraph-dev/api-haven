// ============================================================================
// DATABASE KINDS: db-kind descriptors — src/database/data/db_kinds.inc
// Hand-curated (small, stable) — edit directly, unlike ai/data .inc files
// which are generator-owned. Included ONLY by db_catalog.c.
//
// 10 DbCatalogSlot rows: the store families the connector layer is shaped
// for. Descriptors only — default ports are the canonical wire ports;
// drivers and engines live in db-haven/darkbase (see ;;INTENTION in
// db_catalog.c).
// ============================================================================
static const DbCatalogSlot kDbKinds[] = {

    {
        DB_KIND_SQLITE,
        "SQLite",
        0,
        "SQLite Consortium",
        "embedded file store; driver may be vendored",
    },
    {
        DB_KIND_POSTGRES,
        "PostgreSQL",
        5432,
        "PostgreSQL Global Development Group",
        NULL,
    },
    {
        DB_KIND_MYSQL,
        "MySQL",
        3306,
        "Oracle",
        NULL,
    },
    {
        DB_KIND_MARIADB,
        "MariaDB",
        3306,
        "MariaDB Foundation",
        "wire-compatible with MySQL drivers",
    },
    {
        DB_KIND_REDIS,
        "Redis",
        6379,
        "Redis Ltd",
        "RESP wire protocol",
    },
    {
        DB_KIND_DUCKDB,
        "DuckDB",
        0,
        "DuckDB Labs",
        "embedded analytical store",
    },
    {
        DB_KIND_SQLSERVER,
        "SQL Server",
        1433,
        "Microsoft",
        "TDS wire protocol",
    },
    {
        DB_KIND_MONGODB,
        "MongoDB",
        27017,
        "MongoDB Inc",
        NULL,
    },
    {
        DB_KIND_ELASTICSEARCH,
        "Elasticsearch",
        9200,
        "Elastic",
        "HTTP JSON API",
    },
    {
        DB_KIND_NEO4J,
        "Neo4j",
        7687,
        "Neo4j Inc",
        "Bolt wire protocol",
    },
};