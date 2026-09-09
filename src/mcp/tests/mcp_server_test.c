#include "annotation/overview.h"
#include "annotation/intention.h"
#include "mcp/mcp_server.h"

#include <stdio.h>
#include <string.h>

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: McpServerTest (src/mcp/tests/mcp_server_test.c)
 * LEVEL: L2 — Behavior verification (headless protocol conformance, no
 * network, no stdio — lines in, response buffers out)
 * ============================================================================
 * Exercises McpServer_handleLine against literal client frames:
 * initialize version negotiation (supported echo / latest fallback),
 * ping, tools/list (all 4 tools with schemas), tools/call success +
 * isError paths, resources/list + read, unknown tool/method/resource
 * errors, batch rejection, notification silence, string-id echo, and
 * parse-error handling.
 *
 * Exit code 0 = all checks green; 1 = at least one check failed.
 * ============================================================================
 */
;;INTENTION("assert by substring on the response line: exact JSON layout is an implementation detail; contract points (id echo, codes, tool names, result shapes) are asserted")

static int sFailures = 0;

static void check(int ok, const char *what) {
    if (!ok) {
        sFailures++;
        fprintf(stderr, "FAIL: %s\n", what);
    }
}

// Run one line through the engine; assert a response was written.
static const char *run(McpServer *srv, const char *line, char *out,
                       size_t cap, const char *what) {
    out[0] = '\0';
    const bool wrote = McpServer_handleLine(srv, line, strlen(line), out, cap);
    check(wrote, what);
    return out;
}

int main(void) {
    McpServer *srv = McpServer_shared();
    char out[262144];

    // --- initialize: supported version echo -------------------------------
    const char *r = run(srv,
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\","
        "\"params\":{\"protocolVersion\":\"2024-11-05\",\"capabilities\":{},"
        "\"clientInfo\":{\"name\":\"test\",\"version\":\"0\"}}}",
        out, sizeof(out), "initialize writes a response");
    check(strstr(out, "\"id\":1") != NULL, "initialize echoes numeric id");
    check(strstr(out, "\"protocolVersion\":\"2024-11-05\"") != NULL,
          "initialize echoes supported client version");
    check(strstr(out, "\"serverInfo\":{\"name\":\"vexgraph-mcp\"") != NULL,
          "initialize advertises server identity");
    check(McpServer_isInitialized(srv), "initialized flag set after handshake");
    check(strcmp(McpServer_getProtocolVersion(srv), "2024-11-05") == 0,
          "negotiated version getter");

    // --- initialize: unsupported version falls back to latest -------------
    r = run(srv,
        "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"initialize\","
        "\"params\":{\"protocolVersion\":\"2099-01-01\"}}",
        out, sizeof(out), "initialize fallback writes a response");
    check(strstr(out, "\"protocolVersion\":\"2025-06-18\"") != NULL,
          "unsupported version negotiates to latest");

    // --- ping --------------------------------------------------------------
    r = run(srv,
        "{\"jsonrpc\":\"2.0\",\"id\":\"p1\",\"method\":\"ping\"}",
        out, sizeof(out), "ping writes a response");
    check(strstr(out, "\"id\":\"p1\"") != NULL, "ping echoes string id with quotes");
    check(strstr(out, "\"result\":{}") != NULL, "ping has empty result");

    // --- tools/list: all four tools ---------------------------------------
    r = run(srv,
        "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"tools/list\"}",
        out, sizeof(out), "tools/list writes a response");
    check(strstr(out, "\"name\":\"app_detect\"") != NULL, "tool app_detect present");
    check(strstr(out, "\"name\":\"capture_status\"") != NULL,
          "tool capture_status present");
    check(strstr(out, "\"name\":\"ai_provider_lookup\"") != NULL,
          "tool ai_provider_lookup present");
    check(strstr(out, "\"name\":\"db_data_source_lookup\"") != NULL,
          "tool db_data_source_lookup present");
    check(strstr(out, "\"inputSchema\"") != NULL, "tool schemas advertised");

    // --- tools/call: app_detect list --------------------------------------
    r = run(srv,
        "{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"tools/call\","
        "\"params\":{\"name\":\"app_detect\",\"arguments\":{}}}",
        out, sizeof(out), "app_detect call writes a response");
    check(strstr(out, "\"id\":4") != NULL, "app_detect echoes id");
    check(strstr(out, "\"result\":{\"content\":[{\"type\":\"text\",\"text\":\"") != NULL,
          "app_detect renders text content");
    check(strstr(out, "KNOWN APPS (14)") != NULL, "app_detect lists all 14 apps");
    check(strstr(out, "opencode") != NULL && strstr(out, "qwen-code") != NULL,
          "app_detect covers first and last registry rows");
    check(strstr(out, "\"isError\":true") == NULL, "app_detect list is not an error");

    // --- tools/call: app_detect single detail (known + unknown) -----------
    r = run(srv,
        "{\"jsonrpc\":\"2.0\",\"id\":5,\"method\":\"tools/call\","
        "\"params\":{\"name\":\"app_detect\",\"arguments\":{\"name\":\"codex\"}}}",
        out, sizeof(out), "app_detect detail writes a response");
    check(strstr(out, "app: codex") != NULL, "app detail renders key");
    check(strstr(out, "display: ") != NULL, "app detail renders display line");
    check(strstr(out, "running: 0") != NULL || strstr(out, "running: 1") != NULL,
          "app detail renders running state");

    r = run(srv,
        "{\"jsonrpc\":\"2.0\",\"id\":6,\"method\":\"tools/call\","
        "\"params\":{\"name\":\"app_detect\","
        "\"arguments\":{\"name\":\"does-not-exist\"}}}",
        out, sizeof(out), "unknown app writes a response");
    check(strstr(out, "\"isError\":true") != NULL, "unknown app is an error");
    check(strstr(out, "unknown app: does-not-exist") != NULL,
          "unknown app names the culprit");

    // --- tools/call: capture_status (all kinds + kind filter) -------------
    r = run(srv,
        "{\"jsonrpc\":\"2.0\",\"id\":7,\"method\":\"tools/call\","
        "\"params\":{\"name\":\"capture_status\",\"arguments\":{}}}",
        out, sizeof(out), "capture_status writes a response");
    check(strstr(out, "SCREEN:") != NULL && strstr(out, "STREAM:") != NULL &&
              strstr(out, "AUDIO:") != NULL,
          "capture_status renders all three kinds");
    check(strstr(out, "SUMMARY: running all=") != NULL,
          "capture_status renders summary line");
    check(strstr(out, "obs") != NULL && strstr(out, "blackhole") != NULL,
          "capture_status covers screen + audio registry rows");

    r = run(srv,
        "{\"jsonrpc\":\"2.0\",\"id\":8,\"method\":\"tools/call\","
        "\"params\":{\"name\":\"capture_status\","
        "\"arguments\":{\"kind\":\"audio\"}}}",
        out, sizeof(out), "capture_status kind filter writes a response");
    check(strstr(out, "AUDIO:") != NULL && strstr(out, "SCREEN:") == NULL,
          "kind filter keeps only the requested section");

    // --- tools/call: ai_provider_lookup (slug + query) --------------------
    r = run(srv,
        "{\"jsonrpc\":\"2.0\",\"id\":9,\"method\":\"tools/call\","
        "\"params\":{\"name\":\"ai_provider_lookup\","
        "\"arguments\":{\"slug\":\"openai\"}}}",
        out, sizeof(out), "ai slug lookup writes a response");
    check(strstr(out, "provider: openai") != NULL, "ai slug match renders key");
    check(strstr(out, "resolved: https://api.openai.com/v1") != NULL,
          "ai slug match renders resolved base");
    check(strstr(out, "auth: bearer") != NULL, "ai slug match renders auth scheme");

    r = run(srv,
        "{\"jsonrpc\":\"2.0\",\"id\":10,\"method\":\"tools/call\","
        "\"params\":{\"name\":\"ai_provider_lookup\","
        "\"arguments\":{\"query\":\"anthropic\"}}}",
        out, sizeof(out), "ai query lookup writes a response");
    check(strstr(out, "anthropic") != NULL, "ai substring query hits rows");
    check(strstr(out, "matches: ") != NULL, "ai query renders match count");

    r = run(srv,
        "{\"jsonrpc\":\"2.0\",\"id\":11,\"method\":\"tools/call\","
        "\"params\":{\"name\":\"ai_provider_lookup\",\"arguments\":{}}}",
        out, sizeof(out), "ai bare call writes a response");
    check(strstr(out, "\"isError\":true") != NULL, "ai bare call hints instead");
    check(strstr(out, "pass slug=") != NULL,
          "ai bare call explains usage");

    // --- tools/call: db_data_source_lookup (slug + bare list) -------------
    r = run(srv,
        "{\"jsonrpc\":\"2.0\",\"id\":12,\"method\":\"tools/call\","
        "\"params\":{\"name\":\"db_data_source_lookup\","
        "\"arguments\":{\"slug\":\"postgresql\"}}}",
        out, sizeof(out), "db slug lookup writes a response");
    check(strstr(out, "data-source: postgresql") != NULL, "db slug match");
    check(strstr(out, "port: 5432") != NULL, "db slug match renders port");

    r = run(srv,
        "{\"jsonrpc\":\"2.0\",\"id\":13,\"method\":\"tools/call\","
        "\"params\":{\"name\":\"db_data_source_lookup\",\"arguments\":{}}}",
        out, sizeof(out), "db bare list writes a response");
    check(strstr(out, "- postgresql |") != NULL && strstr(out, "engine=postgres | port=5432 | sql") != NULL,
          "db bare list renders full table");

    // --- tools/call: unknown tool -----------------------------------------
    r = run(srv,
        "{\"jsonrpc\":\"2.0\",\"id\":14,\"method\":\"tools/call\","
        "\"params\":{\"name\":\"nope\",\"arguments\":{}}}",
        out, sizeof(out), "unknown tool writes a response");
    check(strstr(out, "\"code\":-32602") != NULL && strstr(out, "\"isError\":true") == NULL,
          "unknown tool is an invalid-params error");

    // --- resources/list + read --------------------------------------------
    r = run(srv,
        "{\"jsonrpc\":\"2.0\",\"id\":15,\"method\":\"resources/list\"}",
        out, sizeof(out), "resources/list writes a response");
    check(strstr(out, "\"uri\":\"system://apps\"") != NULL &&
              strstr(out, "\"uri\":\"system://capture\"") != NULL &&
              strstr(out, "\"uri\":\"db://data-sources\"") != NULL,
          "all three resource URIs advertised");
    check(strstr(out, "\"mimeType\":\"text/plain\"") != NULL,
          "resources carry a mime type");

    r = run(srv,
        "{\"jsonrpc\":\"2.0\",\"id\":16,\"method\":\"resources/read\","
        "\"params\":{\"uri\":\"system://apps\"}}",
        out, sizeof(out), "resources/read writes a response");
    check(strstr(out, "\"uri\":\"system://apps\"") != NULL,
          "resource read echoes the uri");
    check(strstr(out, "KNOWN APPS (14)") != NULL,
          "resource read renders the same body as the tool");

    r = run(srv,
        "{\"jsonrpc\":\"2.0\",\"id\":17,\"method\":\"resources/read\","
        "\"params\":{\"uri\":\"system://nope\"}}",
        out, sizeof(out), "unknown resource writes a response");
    check(strstr(out, "\"code\":-32602") != NULL, "unknown resource is invalid params");

    // --- protocol errors ---------------------------------------------------
    r = run(srv,
        "{\"jsonrpc\":\"2.0\",\"id\":18,\"method\":\"bogus/method\"}",
        out, sizeof(out), "unknown method writes a response");
    check(strstr(out, "\"code\":-32601") != NULL && strstr(out, "\"message\":\"Method not found\"") != NULL,
          "unknown method is -32601");

    r = run(srv,
        "{\"jsonrpc\":\"2.0\",\"id\":19,\"method\":\"tools/list\",",
        out, sizeof(out), "malformed json writes a parse error");
    check(strstr(out, "\"code\":-32700") != NULL, "malformed line is -32700");

    r = run(srv,
        "[{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"ping\"}]",
        out, sizeof(out), "batch line writes a response");
    check(strstr(out, "\"code\":-32600") != NULL, "batch is rejected with -32600");

    // --- notifications get no response -------------------------------------
    out[0] = '\0';
    const bool wrote = McpServer_handleLine(srv,
        "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}",
        strlen("{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}"),
        out, sizeof(out));
    check(!wrote, "notifications/initialized is silent");
    check((*out) == '\0', "notification leaves the buffer untouched");

    // --- null id behaves like a notification -------------------------------
    out[0] = '\0';
    const bool wrote2 = McpServer_handleLine(srv,
        "{\"jsonrpc\":\"2.0\",\"id\":null,\"method\":\"ping\"}",
        strlen("{\"jsonrpc\":\"2.0\",\"id\":null,\"method\":\"ping\"}"), out,
        sizeof(out));
    check(!wrote2, "null id is silent (notification semantics)");

    if (sFailures == 0) {
        printf("mcp_server_test: all checks green\n");
        return 0;
    }
    printf("mcp_server_test: %d check(s) FAILED\n", sFailures);
    return 1;
}