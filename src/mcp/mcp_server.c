#include "annotation/intention.h"
#include "annotation/overview.h"
#include "mcp/mcp_server.h"

#include "ai/ai_provider.h"
#include "database/db_provider.h"
#include "net/json.h"
#include "system/app_detect.h"
#include "system/capture_tool.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: McpServer (mcp/mcp_server)
 * LEVEL: L3 — Module behavior (protocol engine running inside the
 * api-haven CLI/MCP tool server; no OS/window/memory management)
 * ============================================================================
 * The Model Context Protocol engine: newline-delimited JSON-RPC 2.0
 * server core that hosts the connector shapes as MCP tools and
 * resources. Handles the handshake (initialize), liveness (ping), and
 * the tool/resource surface, rendering responses into caller-owned
 * buffers. Immutable describer tables (McpToolSlot / McpResourceSlot)
 * hang off this class (Rule 3 slot-record co-location); each handler is
 * a bare fn pointer into registry-render logic — zero allocation, no
 * network, single-threaded.
 *
 * STRUCT FIELDS (Mirroring mcp/mcp_server.h — exactly this file's class):
 * ----------------------------------------------------------------------------
 *   char protocolVersion[32]; // negotiated client version or latest
 *   uint32_t initialized;     // 1 after a successful initialize handshake
 *   const char *name;         // "vexgraph-mcp"
 *   const char *version;      // MCP_SERVER_VERSION
 *
 * SLOT RECORD (McpToolSlot — Rule 3 co-location, behavior deferred to the
 * fn-pointer handler; one row per exposed tool):
 * ----------------------------------------------------------------------------
 *   const char *name;         // tool key, e.g. "app_detect"
 *   const char *description;  // one-line MCP description
 *   const char *inputSchema;  // JSON Schema (pre-rendered static text)
 *   McpToolFn handle;         // bool (*)(doc, argsRef, out, cap)
 *
 * SLOT RECORD (McpResourceSlot — same co-location; one row per URI):
 * ----------------------------------------------------------------------------
 *   const char *uri;          // e.g. "system://apps"
 *   const char *name;         // human label
 *   const char *mimeType;     // "text/plain"
 *   McpToolFn render;         // bool (*)(doc, -1, out, cap) — no args
 *
 * PRIVATE HELPERS (static, file-local — signatures + roles so the file
 * reads from the overview alone; each is pure render/protocol logic with
 * zero state):
 * ----------------------------------------------------------------------------
 *   appendStr / appendFmt(buf, cap, pos, ...)   — bounded builders
 *   familyName / authName / regionName / dbFamilyName / kindName(...) —
 *                          enum→label maps with fallbacks
 *   containsFold(haystack, needle) — ASCII case-insensitive substring
 *                          (registry rows are ASCII identifiers)
 *   escapeJsonText(src, out, cap)   — \" \\ \n \r \t escaping for body text
 *   methodIs(doc, ref, want)        — view-safe method compare
 *   readStringArg(doc, argsRef, key, out, cap) — copy an args string;
 *                          false when absent/not-a-string (args=-1 ⇒ false)
 *   rawIdExtract(line, len, out, cap) — copy the raw "id" token verbatim
 *                          (quoted string id keeps its quotes; null/absent
 *                          ⇒ false so the caller treats it as notification)
 *   respondError(code, message, id, out, cap) — JSON-RPC error envelope
 *   respondPing(id, out, cap)                  — empty result
 *   respondToolsList(id, out, cap)             — static tool table
 *   respondResourcesList(id, out, cap)         — static resource table
 *   respondInitializeReal(self, doc, id, out, cap) — negotiate protocol
 *                          version + capabilities handshake
 *   renderAppDetect(doc, argsRef, out, cap)    — tool + resource body for
 *                          system://apps (14 known apps; optional "name"
 *                          arg for a single app detail)
 *   renderCaptureStatus(doc, argsRef, out, cap) — system://capture body:
 *                          liveness per kind, optional "kind" filter
 *   renderAiLookup(doc, argsRef, out, cap)     — provider row by "slug" or
 *                          substring "query" (cap 20 matches)
 *   renderDbLookup(doc, argsRef, out, cap)     — db source by "slug",
 *                          "engine", or "query"; all rows when bare
 *   findResource(uri)                          — resource table lookup
 * ============================================================================
 */
;;INTENTION("protocol boundary: tools read registries/probes only — no exec, no writes; MCP input never triggers side effects in v1 (evidence-gathering surface)")
;;INTENTION("no pagination (cursor): the full small tables ship in one page; capabilities advertise no listChanged/subscriptions")
;;INTENTION("id echo is a raw token slice of the client line (numbers and strings, quotes preserved); null/missing id ⇒ notification, no response — per JSON-RPC 2.0")
;;INTENTION("privacy wall documented in renderCaptureStatus: macOS has no public API for other apps' ScreenCaptureKit sessions — liveness is the honest contract")

// One tool handler: writes the plain-text tool body into out (cap bytes).
// Return true = success (isError false), false = failure (isError true).
typedef bool (*McpToolFn)(const JsonDoc *doc, JsonRef args,
                          char *out, size_t cap);

typedef struct McpToolSlot {
    const char *name;
    const char *description;
    const char *inputSchema;
    McpToolFn handle;
} McpToolSlot;

typedef struct McpResourceSlot {
    const char *uri;
    const char *name;
    const char *mimeType;
    McpToolFn render;
} McpResourceSlot;

// --- static tool handler forward declarations ---
static bool renderAppDetect(const JsonDoc *doc, JsonRef args,
                            char *out, size_t cap);
static bool renderCaptureStatus(const JsonDoc *doc, JsonRef args,
                                char *out, size_t cap);
static bool renderAiLookup(const JsonDoc *doc, JsonRef args,
                           char *out, size_t cap);
static bool renderDbLookup(const JsonDoc *doc, JsonRef args,
                           char *out, size_t cap);

// HOSTED TOOLS — the connector surface exposed to AI clients.
static const McpToolSlot kMcpTools[] = {
    {
        "app_detect",
        "Known developer-tool presence: list the 14 known coding CLIs/agents "
        "(opencode, codex, claude, t3, cursor, hermes, nous, grok, gemini, "
        "aider, goose, cline, qwen-code, continue) with PATH/bundle/live-"
        "process state, or detail one by name.",
        "{\"type\":\"object\",\"properties\":{\"name\":{\"type\":\"string\","
        "\"description\":\"exact known-app key to detail; omit to list all\"}},"
        "\"additionalProperties\":false}",
        renderAppDetect,
    },
    {
        "capture_status",
        "Screen/audio capture liveness right now: which known capture tools "
        "(OBS, QuickTime Player, macOS Screenshot, Screen Studio, CleanShot X, "
        "Kap, Loom, ScreenFlow, Camtasia, Filmage, Zoom, Voice Memos, "
        "Loopback, BlackHole, Soundflower) are running or installed, per kind. "
        "Privacy wall: other apps' capture sessions are invisible to macOS "
        "APIs — this reports process liveness, the honest contract.",
        "{\"type\":\"object\",\"properties\":{\"kind\":{\"type\":\"string\","
        "\"enum\":[\"screen\",\"stream\",\"audio\"],"
        "\"description\":\"filter by capture kind; omit for all\"}},"
        "\"additionalProperties\":false}",
        renderCaptureStatus,
    },
    {
        "ai_provider_lookup",
        "AI provider directory lookup across 260 providers: exact slug "
        "(openai, anthropic, zhipu-ai, nous-research, ...) with resolved base "
        "URL and auth scheme, or a substring query over names.",
        "{\"type\":\"object\",\"properties\":{\"slug\":{\"type\":\"string\","
        "\"description\":\"exact provider slug, e.g. openai\"},"
        "\"query\":{\"type\":\"string\",\"description\":\"case-insensitive "
        "substring over slug/display/note\"}},\"additionalProperties\":false}",
        renderAiLookup,
    },
    {
        "db_data_source_lookup",
        "Database data-source directory: 25 sources with complete driver "
        "support (PostgreSQL, MySQL, Oracle, ...). Lookup by slug, by driver "
        "engine key, or list all with ports and wire families.",
        "{\"type\":\"object\",\"properties\":{\"slug\":{\"type\":\"string\","
        "\"description\":\"exact data-source slug, e.g. postgresql\"},"
        "\"engine\":{\"type\":\"string\",\"description\":\"driver/engine key, "
        "e.g. postgres\"},\"query\":{\"type\":\"string\",\"description\":"
        "\"case-insensitive substring\"}},\"additionalProperties\":false}",
        renderDbLookup,
    },
};

// HOSTED RESOURCES — plain-text registry dumps under stable URIs.
static const McpResourceSlot kMcpResources[] = {
    {
        "system://apps",
        "Known developer tools (14 app registry rows)",
        "text/plain",
        renderAppDetect,
    },
    {
        "system://capture",
        "Capture tools and their live liveness",
        "text/plain",
        renderCaptureStatus,
    },
    {
        "db://data-sources",
        "Database data sources with complete driver support",
        "text/plain",
        renderDbLookup,
    },
};

static const size_t kMcpToolCount = sizeof(kMcpTools) / sizeof(kMcpTools[0]);
static const size_t kMcpResourceCount =
    sizeof(kMcpResources) / sizeof(kMcpResources[0]);

// --- bounded builders -------------------------------------------------------

static void appendStr(char *buf, size_t cap, size_t *pos, const char *s) {
    if ((*pos) >= cap)
        return;
    size_t room = cap - (*pos) - 1;
    size_t take = strlen(s);
    if (take > room)
        take = room;
    memcpy(buf + (*pos), s, take);
    (*pos) += take;
    buf[*pos] = '\0';
}

static void appendFmt(char *buf, size_t cap, size_t *pos, const char *fmt, ...) {
    if ((*pos) >= cap)
        return;
    va_list ap;
    va_start(ap, fmt);
    int want = vsnprintf(buf + (*pos), cap - (*pos), fmt, ap);
    va_end(ap);
    if (want > 0) {
        (*pos) += (size_t)want;
        if ((*pos) > cap - 1)
            (*pos) = cap - 1;
    }
}

// --- enum → label maps ------------------------------------------------------

static const char *familyName(AiProviderFamily f) {
    switch (f) {
        case AI_PROVIDER_FAMILY_OPENAI_COMPAT: return "openai-compat";
        case AI_PROVIDER_FAMILY_ANTHROPIC:     return "anthropic";
        case AI_PROVIDER_FAMILY_ROUTER:        return "router";
        case AI_PROVIDER_FAMILY_NATIVE:        return "native";
        case AI_PROVIDER_FAMILY_LOCAL:         return "local";
    }
    return "?";
}

static const char *authName(AiProviderAuth a) {
    switch (a) {
        case AI_PROVIDER_AUTH_BEARER:    return "bearer";
        case AI_PROVIDER_AUTH_API_KEY:   return "api-key-header";
        case AI_PROVIDER_AUTH_X_API_KEY: return "x-api-key";
        case AI_PROVIDER_AUTH_NONE:      return "none";
        case AI_PROVIDER_AUTH_SPECIAL:   return "special";
    }
    return "?";
}

static const char *regionName(AiProviderRegion r) {
    switch (r) {
        case AI_PROVIDER_REGION_GLOBAL: return "global";
        case AI_PROVIDER_REGION_CHINA:  return "china";
        case AI_PROVIDER_REGION_EUROPE: return "europe";
        case AI_PROVIDER_REGION_OTHER:  return "other";
    }
    return "?";
}

static const char *dbFamilyName(DbProviderFamily f) {
    switch (f) {
        case DB_PROVIDER_FAMILY_SQL:        return "sql";
        case DB_PROVIDER_FAMILY_SQL_COMPAT: return "sql-compat";
        case DB_PROVIDER_FAMILY_NO_SQL:     return "nosql";
        case DB_PROVIDER_FAMILY_EMBEDDED:   return "embedded";
    }
    return "?";
}

static const char *kindName(CaptureKind kind) {
    switch (kind) {
        case CAPTURE_KIND_SCREEN: return "screen";
        case CAPTURE_KIND_STREAM: return "stream";
        case CAPTURE_KIND_AUDIO:  return "audio";
    }
    return "?";
}

// --- string helpers ---------------------------------------------------------

static bool containsFold(const char *haystack, const char *needle) {
    if (!haystack || !needle)
        return false;
    const size_t nl = strlen(needle);
    if (nl == 0)
        return true;
    const size_t hl = strlen(haystack);
    if (nl > hl)
        return false;
    for (size_t i = 0; i + nl <= hl; i++) {
        size_t j = 0;
        while (j < nl) {
            char a = haystack[i + j];
            char b = needle[j];
            if (a >= 'A' && a <= 'Z')
                a = (char)(a + 32);
            if (b >= 'A' && b <= 'Z')
                b = (char)(b + 32);
            if (a != b)
                break;
            j++;
        }
        if (j == nl)
            return true;
    }
    return false;
}

static void escapeJsonText(const char *src, char *out, size_t cap) {
    size_t o = 0;
    for (const char *p = src; *p && o + 6 < cap; p++) {
        char c = *p;
        switch (c) {
            case '"':  appendStr(out, cap, &o, "\\\""); break;
            case '\\': appendStr(out, cap, &o, "\\\\"); break;
            case '\n': appendStr(out, cap, &o, "\\n"); break;
            case '\r': appendStr(out, cap, &o, "\\r"); break;
            case '\t': appendStr(out, cap, &o, "\\t"); break;
            default:
                out[o++] = c;
                out[o] = '\0';
        }
    }
    out[o] = '\0';
}

static bool methodIs(const JsonDoc *doc, JsonRef method, const char *want) {
    uint32_t len = 0;
    const char *s = Json_string(doc, method, &len);
    if (!s)
        return false;
    const size_t wl = strlen(want);
    return (size_t)len == wl && memcmp(s, want, wl) == 0;
}

// Copy one string argument from an args subobject. Returns false when the
// member is absent or not a string (out stays ""). Safe for args < 0.
static bool readStringArg(const JsonDoc *doc, JsonRef args,
                          const char *key, char *out, size_t cap) {
    out[0] = '\0';
    if (args < 0 || !out || cap == 0)
        return false;
    JsonRef v = Json_member(doc, args, key);
    if (v < 0)
        return false;
    uint32_t len = 0;
    const char *s = Json_string(doc, v, &len);
    if (!s)
        return false;
    if (len >= cap)
        len = (uint32_t)(cap - 1);
    memcpy(out, s, len);
    out[len] = '\0';
    return true;
}

// Raw "id" token extraction from the client line — echoed verbatim per
// JSON-RPC 2.0 (a quoted string id keeps its quotes; a bare numeric id or
// "null" literal is copied as-is). Returns false when absent so the caller
// treats the message as a notification.
static bool rawIdExtract(const char *line, size_t lineLen,
                         char *out, size_t cap) {
    if (!line || !out || cap == 0)
        return false;
    out[0] = '\0';
    const char *end = line + lineLen;
    const char *p = line;
    bool found = false;
    while (p + 4 <= end) {
        // the exact 4-byte token "id"
        if (p[0] == '"' && p[1] == 'i' && p[2] == 'd' && p[3] == '"') {
            found = true;
            break;
        }
        p++;
    }
    if (!found)
        return false;
    p += 4;
    while (p < end && ((*p) == ' ' || (*p) == '\t'))
        p++;
    if (p < end && (*p) == ':')
        p++;
    while (p < end && ((*p) == ' ' || (*p) == '\t'))
        p++;
    if (p >= end)
        return false;
    size_t o = 0;
    if ((*p) == '"') {
        // quoted string id — copy through the closing unescaped quote
        if (o + 1 < cap)
            out[o++] = (*p); // opening quote
        p++;
        while (p < end && o + 1 < cap) {
            char c = *p;
            if (c == '\\' && p + 1 < end) {
                if (o + 2 < cap) {
                    out[o++] = *p;
                    out[o++] = *(p + 1);
                }
                p += 2;
                continue;
            }
            if (c == '"') {
                if (o + 1 < cap)
                    out[o++] = c; // closing quote
                out[o] = '\0';
                return true;
            }
            if (o + 1 < cap)
                out[o++] = c;
            p++;
        }
        return false;
    }
    // number or null literal
    while (p < end && o + 1 < cap) {
        char c = *p;
        if (c == ',' || c == '}' || c == ' ' || c == '\t' || c == '\n')
            break;
        out[o++] = c;
        p++;
    }
    out[o] = '\0';
    return o > 0;
}

// --- error envelope ---------------------------------------------------------

static void respondError(int code, const char *message, const char *id,
                         char *out, size_t cap) {
    size_t pos = 0;
    appendStr(out, cap, &pos, "{\"jsonrpc\":\"2.0\",\"id\":");
    appendStr(out, cap, &pos, id && (*id) != '\0' ? id : "null");
    appendFmt(out, cap, &pos, ",\"error\":{\"code\":%d,\"message\":\"%s\"}}",
              code, message);
}

// --- plain responses --------------------------------------------------------

static void respondPing(const char *id, char *out, size_t cap) {
    size_t pos = 0;
    appendStr(out, cap, &pos, "{\"jsonrpc\":\"2.0\",\"id\":");
    appendStr(out, cap, &pos, id);
    appendStr(out, cap, &pos, ",\"result\":{}}");
}

static void respondToolsList(const char *id, char *out, size_t cap) {
    size_t pos = 0;
    appendStr(out, cap, &pos, "{\"jsonrpc\":\"2.0\",\"id\":");
    appendStr(out, cap, &pos, id);
    appendStr(out, cap, &pos, ",\"result\":{\"tools\":[");
    for (size_t i = 0; i < kMcpToolCount; i++) {
        if (i > 0)
            appendStr(out, cap, &pos, ",");
        appendStr(out, cap, &pos, "{\"name\":\"");
        appendStr(out, cap, &pos, kMcpTools[i].name);
        appendStr(out, cap, &pos, "\",\"description\":\"");
        appendStr(out, cap, &pos, kMcpTools[i].description);
        appendStr(out, cap, &pos, "\",\"inputSchema\":");
        appendStr(out, cap, &pos, kMcpTools[i].inputSchema);
        appendStr(out, cap, &pos, "}");
    }
    appendStr(out, cap, &pos, "]}}");
}

static void respondResourcesList(const char *id, char *out, size_t cap) {
    size_t pos = 0;
    appendStr(out, cap, &pos, "{\"jsonrpc\":\"2.0\",\"id\":");
    appendStr(out, cap, &pos, id);
    appendStr(out, cap, &pos, ",\"result\":{\"resources\":[");
    for (size_t i = 0; i < kMcpResourceCount; i++) {
        if (i > 0)
            appendStr(out, cap, &pos, ",");
        appendStr(out, cap, &pos, "{\"uri\":\"");
        appendStr(out, cap, &pos, kMcpResources[i].uri);
        appendStr(out, cap, &pos, "\",\"name\":\"");
        appendStr(out, cap, &pos, kMcpResources[i].name);
        appendStr(out, cap, &pos, "\",\"mimeType\":\"");
        appendStr(out, cap, &pos, kMcpResources[i].mimeType);
        appendStr(out, cap, &pos, "\"}");
    }
    appendStr(out, cap, &pos, "]}}");
}

static bool respondInitializeReal(McpServer *self, const JsonDoc *doc,
                                  const char *id, char *out, size_t cap) {
    // negotiate: echo a supported client version, else our latest
    JsonRef params = Json_member(doc, Json_root(doc), "params");
    JsonRef pv = params >= 0 ? Json_member(doc, params, "protocolVersion") : -1;
    const char *chosen = MCP_PROTOCOL_LATEST;
    if (pv >= 0) {
        uint32_t len = 0;
        const char *s = Json_string(doc, pv, &len);
        if (s) {
            char buf[40];
            if (len >= sizeof(buf))
                len = (uint32_t)(sizeof(buf) - 1);
            memcpy(buf, s, len);
            buf[len] = '\0';
            if (strcmp(buf, "2024-11-05") == 0)
                chosen = "2024-11-05";
            else if (strcmp(buf, "2025-03-26") == 0)
                chosen = "2025-03-26";
            else if (strcmp(buf, "2025-06-18") == 0)
                chosen = "2025-06-18";
        }
    }
    snprintf((*self).protocolVersion, sizeof((*self).protocolVersion), "%s",
             chosen);
    (*self).initialized = 1;
    size_t pos = 0;
    appendStr(out, cap, &pos, "{\"jsonrpc\":\"2.0\",\"id\":");
    appendStr(out, cap, &pos, id);
    appendFmt(out, cap, &pos,
              ",\"result\":{\"protocolVersion\":\"%s\","
              "\"capabilities\":{\"tools\":{},\"resources\":{}},"
              "\"serverInfo\":{\"name\":\"%s\",\"version\":\"%s\"}}}",
              (*self).protocolVersion, (*self).name, (*self).version);
    return true;
}

// --- tool renderers (plain text bodies) -------------------------------------

static bool renderAppDetect(const JsonDoc *doc, JsonRef args,
                            char *out, size_t cap) {
    size_t pos = 0;
    AppDetect *detect = AppDetect_shared();
    char name[128];
    const bool haveName = readStringArg(doc, args, "name", name, sizeof(name));
    if (haveName && (*name) != '\0') {
        // single-app detail
        const AppSlot *slot = AppDetect_get(detect, name);
        if (!slot) {
            appendFmt(out, cap, &pos, "unknown app: %s", name);
            return false;
        }
        const char *nm = AppDetect_getName(detect, slot);
        appendFmt(out, cap, &pos, "app: %s\n", nm);
        appendFmt(out, cap, &pos, "  display: %s\n",
                  AppDetect_getDisplayName(detect, slot));
        appendFmt(out, cap, &pos, "  onPath: %d\n",
                  AppDetect_isOnPath(detect, nm) ? 1 : 0);
        appendFmt(out, cap, &pos, "  appBundle: %d\n",
                  AppDetect_isAppBundle(detect, nm) ? 1 : 0);
        appendFmt(out, cap, &pos, "  running: %d\n",
                  AppDetect_isRunning(detect, slot) ? 1 : 0);
        const char *note = AppDetect_getNote(detect, slot);
        if (note)
            appendFmt(out, cap, &pos, "  note: %s\n", note);
        return true;
    }
    const uint32_t total = AppDetect_count(detect);
    appendFmt(out, cap, &pos, "KNOWN APPS (%u) — onPath | appBundle | running\n",
              total);
    for (uint32_t i = 0; i < total; i++) {
        const AppSlot *slot = AppDetect_at(detect, i);
        const char *nm = AppDetect_getName(detect, slot);
        appendFmt(out, cap, &pos, "- %s | %s | onPath=%d bundle=%d running=%d\n",
                  nm, AppDetect_getDisplayName(detect, slot),
                  AppDetect_isOnPath(detect, nm) ? 1 : 0,
                  AppDetect_isAppBundle(detect, nm) ? 1 : 0,
                  AppDetect_isRunning(detect, slot) ? 1 : 0);
    }
    return true;
}

static bool renderCaptureStatus(const JsonDoc *doc, JsonRef args,
                                char *out, size_t cap) {
    size_t pos = 0;
    CaptureTool *tools = CaptureTool_shared();
    char kind[32];
    const bool haveKind = readStringArg(doc, args, "kind", kind, sizeof(kind));
    const bool wantScreen = !haveKind || strcmp(kind, "screen") == 0;
    const bool wantStream = !haveKind || strcmp(kind, "stream") == 0;
    const bool wantAudio = !haveKind || strcmp(kind, "audio") == 0;
    appendStr(out, cap, &pos,
              "CAPTURE STATUS — known capture tools alive now\n"
              "(privacy wall: other apps' screen-capture sessions are not\n"
              "visible to macOS APIs; liveness is the honest contract)\n");
    const uint32_t total = CaptureTool_count(tools);
    const char *sections[3] = {"SCREEN:", "STREAM:", "AUDIO:"};
    const bool wants[3] = {wantScreen, wantStream, wantAudio};
    const CaptureKind kinds[3] = {CAPTURE_KIND_SCREEN, CAPTURE_KIND_STREAM,
                                  CAPTURE_KIND_AUDIO};
    for (uint32_t s = 0; s < 3; s++) {
        if (!wants[s])
            continue;
        appendFmt(out, cap, &pos, "%s\n", sections[s]);
        for (uint32_t i = 0; i < total; i++) {
            const CaptureSlot *slot = CaptureTool_at(tools, i);
            if (CaptureTool_getKind(tools, slot) != kinds[s])
                continue;
            const bool isDriver = CaptureTool_getProcKey(tools, slot) == NULL;
            appendFmt(out, cap, &pos, "- %s | %s | running=%d installed=%d%s\n",
                      CaptureTool_getSlug(tools, slot),
                      CaptureTool_getDisplayName(tools, slot),
                      CaptureTool_isRunning(tools, slot) ? 1 : 0,
                      CaptureTool_isInstalled(tools, slot) ? 1 : 0,
                      isDriver ? " (driver)" : "");
        }
        appendFmt(out, cap, &pos, "running %s: %u\n",
                  kindName(kinds[s]), CaptureTool_countRunning(tools, kinds[s]));
    }
    appendFmt(out, cap, &pos,
              "SUMMARY: running all=%u screen=%u stream=%u audio=%u\n",
              CaptureTool_countRunningAll(tools),
              CaptureTool_countRunning(tools, CAPTURE_KIND_SCREEN),
              CaptureTool_countRunning(tools, CAPTURE_KIND_STREAM),
              CaptureTool_countRunning(tools, CAPTURE_KIND_AUDIO));
    return true;
}

static bool renderAiLookup(const JsonDoc *doc, JsonRef args,
                           char *out, size_t cap) {
    size_t pos = 0;
    AiProvider *dir = AiProvider_shared();
    char slug[128];
    char query[128];
    const bool haveSlug = readStringArg(doc, args, "slug", slug, sizeof(slug));
    const bool haveQuery = readStringArg(doc, args, "query", query,
                                         sizeof(query));

    if (haveSlug && (*slug) != '\0') {
        const AiProviderSlot *slot = AiProvider_get(dir, slug);
        if (!slot) {
            appendFmt(out, cap, &pos, "unknown provider: %s", slug);
            return false;
        }
        appendFmt(out, cap, &pos, "provider: %s\n",
                  AiProvider_getSlug(dir, slot));
        appendFmt(out, cap, &pos, "  display: %s\n",
                  AiProvider_getDisplayName(dir, slot));
        appendFmt(out, cap, &pos, "  family: %s\n",
                  familyName(AiProvider_getFamily(dir, slot)));
        appendFmt(out, cap, &pos, "  auth: %s\n",
                  authName(AiProvider_getAuth(dir, slot)));
        appendFmt(out, cap, &pos, "  region: %s\n",
                  regionName(AiProvider_getRegion(dir, slot)));
        const char *base = AiProvider_getBaseUrl(dir, slot);
        appendFmt(out, cap, &pos, "  base: %s\n",
                  base ? base : "(null — family default)");
        const char *resolved = AiProvider_resolveBaseUrl(dir, slot);
        appendFmt(out, cap, &pos, "  resolved: %s\n",
                  resolved ? resolved : "?");
        const char *note = AiProvider_getNote(dir, slot);
        if (note)
            appendFmt(out, cap, &pos, "  note: %s\n", note);
        return true;
    }

    if (!haveQuery || (*query) == '\0') {
        appendStr(out, cap, &pos,
                  "pass slug=... (exact provider key) or query=... (substring); "
                  "examples: openai, anthropic, zhipu-ai, nous-research");
        return false;
    }

    // substring scan (cap 20 matches, counted)
    const uint32_t total = AiProvider_count(dir);
    uint32_t shown = 0;
    uint32_t hits = 0;
    for (uint32_t i = 0; i < total; i++) {
        const AiProviderSlot *slot = AiProvider_at(dir, i);
        const char *sl = AiProvider_getSlug(dir, slot);
        if (!containsFold(sl, query)) {
            const char *display = AiProvider_getDisplayName(dir, slot);
            const char *note = AiProvider_getNote(dir, slot);
            if (!containsFold(display, query) && !containsFold(note, query))
                continue;
        }
        hits++;
        if (shown < 20) {
            const char *resolved = AiProvider_resolveBaseUrl(dir, slot);
            appendFmt(out, cap, &pos, "- %s | %s | %s | %s | %s\n",
                      sl, AiProvider_getDisplayName(dir, slot),
                      familyName(AiProvider_getFamily(dir, slot)),
                      authName(AiProvider_getAuth(dir, slot)),
                      resolved != NULL ? resolved : "?");
            shown++;
        }
    }
    appendFmt(out, cap, &pos, "matches: %u (showing %u)\n", hits, shown);
    return true;
}

static bool renderDbLookup(const JsonDoc *doc, JsonRef args,
                           char *out, size_t cap) {
    size_t pos = 0;
    DbProvider *dir = DbProvider_shared();
    char slug[128];
    char engine[128];
    char query[128];
    const bool haveSlug = readStringArg(doc, args, "slug", slug, sizeof(slug));
    const bool haveEngine = readStringArg(doc, args, "engine", engine,
                                          sizeof(engine));
    const bool haveQuery = readStringArg(doc, args, "query", query,
                                         sizeof(query));

    if (haveSlug && (*slug) != '\0') {
        const DbProviderSlot *slot = DbProvider_get(dir, slug);
        if (!slot) {
            appendFmt(out, cap, &pos, "unknown data source: %s", slug);
            return false;
        }
        appendFmt(out, cap, &pos, "data-source: %s\n",
                  DbProvider_getSlug(dir, slot));
        appendFmt(out, cap, &pos, "  display: %s\n",
                  DbProvider_getDisplayName(dir, slot));
        appendFmt(out, cap, &pos, "  engine: %s\n",
                  DbProvider_getEngine(dir, slot));
        appendFmt(out, cap, &pos, "  port: %u\n",
                  DbProvider_getDefaultPort(dir, slot));
        appendFmt(out, cap, &pos, "  family: %s\n",
                  dbFamilyName(DbProvider_getFamily(dir, slot)));
        const char *note = DbProvider_getNote(dir, slot);
        if (note)
            appendFmt(out, cap, &pos, "  note: %s\n", note);
        return true;
    }

    if (haveEngine && (*engine) != '\0') {
        const DbProviderSlot *slot = DbProvider_findByEngine(dir, engine);
        if (!slot) {
            appendFmt(out, cap, &pos, "unknown engine: %s", engine);
            return false;
        }
        appendFmt(out, cap, &pos, "- %s | %s | engine=%s | port=%u | %s\n",
                  DbProvider_getSlug(dir, slot),
                  DbProvider_getDisplayName(dir, slot),
                  DbProvider_getEngine(dir, slot),
                  DbProvider_getDefaultPort(dir, slot),
                  dbFamilyName(DbProvider_getFamily(dir, slot)));
        return true;
    }

    // bare (or query-filtered) listing of the full 25-row table
    const uint32_t total = DbProvider_count(dir);
    for (uint32_t i = 0; i < total; i++) {
        const DbProviderSlot *slot = DbProvider_at(dir, i);
        if (haveQuery && (*query) != '\0') {
            const char *sl = DbProvider_getSlug(dir, slot);
            const char *display = DbProvider_getDisplayName(dir, slot);
            const char *note = DbProvider_getNote(dir, slot);
            if (!containsFold(sl, query) && !containsFold(display, query) &&
                !containsFold(note, query))
                continue;
        }
        appendFmt(out, cap, &pos, "- %s | %s | engine=%s | port=%u | %s\n",
                  DbProvider_getSlug(dir, slot),
                  DbProvider_getDisplayName(dir, slot),
                  DbProvider_getEngine(dir, slot),
                  DbProvider_getDefaultPort(dir, slot),
                  dbFamilyName(DbProvider_getFamily(dir, slot)));
    }
    return true;
}

// --- resource read ----------------------------------------------------------

static const McpResourceSlot *findResource(const char *uri) {
    if (!uri)
        return NULL;
    for (size_t i = 0; i < kMcpResourceCount; i++) {
        if (strcmp(kMcpResources[i].uri, uri) == 0)
            return &kMcpResources[i];
    }
    return NULL;
}

// --- main dispatch ----------------------------------------------------------

bool McpServer_handleLine(McpServer *self, const char *line, size_t lineLen,
                          char *outBuf, size_t outCap) {
    if (!self || !line || !outBuf || outCap < 2)
        return false;
    outBuf[0] = '\0';

    // Raw id echo (verbatim, per JSON-RPC).
    char id[64];
    const bool hasId = rawIdExtract(line, lineLen, id, sizeof(id));
    const bool idNull = hasId && strcmp(id, "null") == 0;
    const char *idS = idNull ? "null" : (hasId ? id : "null");

    // Reject batches deterministically (the core is line-oriented).
    const char *first = line;
    while (first < line + lineLen && ((*first) == ' ' || (*first) == '\t'))
        first++;
    if (first < line + lineLen && (*first) == '[') {
        respondError(-32600, "Batch requests are not supported", idS,
                     outBuf, outCap);
        return true;
    }

    // Parse (failed parse => parse error). Note: line must be
    // NUL-terminated by the caller (in practice a read-line buffer).
    JsonNode nodes[64];
    char scratch[2048];
    JsonDoc doc;
    if (!Json_parse(&doc, nodes, 64, scratch, sizeof(scratch), line)) {
        respondError(-32700, "Parse error", idS, outBuf, outCap);
        return true;
    }

    JsonRef root = Json_root(&doc);
    JsonRef method = Json_member(&doc, root, "method");
    if (method < 0) {
        if (hasId && !idNull)
            respondError(-32600, "Invalid Request", idS, outBuf, outCap);
        return hasId && !idNull;
    }

    // Notifications never answer (JSON-RPC 2.0).
    if (!hasId || idNull)
        return false;

    if (methodIs(&doc, method, "initialize"))
        return respondInitializeReal(self, &doc, idS, outBuf, outCap);
    if (methodIs(&doc, method, "ping")) {
        respondPing(idS, outBuf, outCap);
        return true;
    }
    if (methodIs(&doc, method, "tools/list")) {
        respondToolsList(idS, outBuf, outCap);
        return true;
    }
    if (methodIs(&doc, method, "resources/list")) {
        respondResourcesList(idS, outBuf, outCap);
        return true;
    }
    if (methodIs(&doc, method, "resources/read")) {
        JsonRef params = Json_member(&doc, root, "params");
        char uri[256];
        readStringArg(&doc, params, "uri", uri, sizeof(uri));
        const McpResourceSlot *res = findResource(uri);
        if (!res) {
            respondError(-32602, "Unknown resource", idS, outBuf, outCap);
            return true;
        }
        char body[65536];
        body[0] = '\0';
        (void)(*res).render(&doc, -1, body, sizeof(body));
        char escaped[131072];
        escapeJsonText(body, escaped, sizeof(escaped));
        size_t pos = 0;
        appendStr(outBuf, outCap, &pos, "{\"jsonrpc\":\"2.0\",\"id\":");
        appendStr(outBuf, outCap, &pos, idS);
        appendFmt(outBuf, outCap, &pos,
                  ",\"result\":{\"contents\":[{\"uri\":\"%s\",\"mimeType\":"
                  "\"%s\",\"text\":\"%s\"}]}}",
                  (*res).uri, (*res).mimeType, escaped);
        return true;
    }
    if (methodIs(&doc, method, "tools/call")) {
        JsonRef params = Json_member(&doc, root, "params");
        char toolName[128];
        readStringArg(&doc, params, "name", toolName, sizeof(toolName));
        if ((*toolName) == '\0') {
            respondError(-32602, "Unknown tool", idS, outBuf, outCap);
            return true;
        }
        const McpToolSlot *tool = NULL;
        for (size_t i = 0; i < kMcpToolCount; i++) {
            if (strcmp(kMcpTools[i].name, toolName) == 0) {
                tool = &kMcpTools[i];
                break;
            }
        }
        if (!tool) {
            respondError(-32602, "Unknown tool", idS, outBuf, outCap);
            return true;
        }
        JsonRef args = Json_member(&doc, params, "arguments");
        char body[65536];
        body[0] = '\0';
        const bool ok = (*tool).handle(&doc, args, body, sizeof(body));
        char escaped[131072];
        escapeJsonText(body, escaped, sizeof(escaped));
        size_t pos = 0;
        appendStr(outBuf, outCap, &pos, "{\"jsonrpc\":\"2.0\",\"id\":");
        appendStr(outBuf, outCap, &pos, idS);
        if (ok)
            appendStr(outBuf, outCap, &pos, ",\"result\":{\"content\":[");
        else
            appendStr(outBuf, outCap, &pos,
                      ",\"result\":{\"isError\":true,\"content\":[");
        appendFmt(outBuf, outCap, &pos, "{\"type\":\"text\",\"text\":\"%s\"}]}}",
                  escaped);
        return true;
    }

    respondError(-32601, "Method not found", idS, outBuf, outCap);
    return true;
}

// CONSTRUCTORS

McpServer *McpServer_shared(void) {
    static McpServer sServerShared; // zero-init singleton
    sServerShared.name = "vexgraph-mcp";
    sServerShared.version = MCP_SERVER_VERSION;
    return &sServerShared;
}

// GETTERS

const char *McpServer_getName(const McpServer *self) {
    return self ? self->name : NULL;
}

const char *McpServer_getVersion(const McpServer *self) {
    return self ? self->version : NULL;
}

const char *McpServer_getProtocolVersion(const McpServer *self) {
    return self ? (*self).protocolVersion : NULL;
}

bool McpServer_isInitialized(const McpServer *self) {
    return self && (*self).initialized != 0;
}