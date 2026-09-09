#include "annotation/overview.h"
#include "ai/ai_provider.h"
#include "ai/ai_chat.h"
#include "net/json.h"

#include <stdio.h>
#include <string.h>

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: AiProviderTest (src/ai/tests/ai_provider_test.c)
 * LEVEL: L2 — Behavior verification (headless; no network, no accounts)
 * ============================================================================
 * Executable proof of the src/ai/ directory: exercises the AiProvider
 * table (count, at, get, resolve, null-safety, immutable getters) and the
 * AiChat buildRequest path (URL resolution, auth kind, JSON envelope
 * rendering) by parsing the generated body back with net/json.
 *
 * Zero HTTP: AiChat_complete is only asserted for its NULL-resp guard.
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

static const AiMessage sMessages[2] = {
    {"user", "say \"hi\" \n to me"},
    {"assistant", "hello"},
};

int main(int argc, const char **argv) {
    (void)argc;
    (void)argv;

    // --- directory shape ---------------------------------------------------
    AiProvider *dir = AiProvider_shared();
    CHECK(dir != NULL);
    const uint32_t total = AiProvider_count(dir);
    printf("AiProvider_count = %u\n", total);
    CHECK(total >= 255 && total <= 280);
    CHECK(AiProvider_at(dir, 0) != NULL);
    CHECK(AiProvider_at(dir, total) == NULL);
    CHECK(AiProvider_at(dir, total + 5) == NULL);

    // --- verified rows ------------------------------------------------------
    const AiProviderSlot *openai = AiProvider_get(dir, "openai");
    CHECK(openai != NULL);
    CHECK(openai && strcmp(AiProvider_getDisplayName(dir, openai), "OpenAI") == 0);
    CHECK(openai && AiProvider_getFamily(dir, openai) == AI_PROVIDER_FAMILY_OPENAI_COMPAT);
    CHECK(openai && AiProvider_getAuth(dir, openai) == AI_PROVIDER_AUTH_BEARER);
    CHECK(openai && AiProvider_getBaseUrl(dir, openai) != NULL);
    CHECK(openai && strcmp(AiProvider_resolveBaseUrl(dir, openai),
                           "https://api.openai.com/v1") == 0);

    const AiProviderSlot *anthropic = AiProvider_get(dir, "anthropic");
    CHECK(anthropic && AiProvider_getFamily(dir, anthropic) == AI_PROVIDER_FAMILY_ANTHROPIC);
    CHECK(anthropic && AiProvider_getAuth(dir, anthropic) == AI_PROVIDER_AUTH_X_API_KEY);

    const AiProviderSlot *bedrock = AiProvider_get(dir, "amazon-bedrock");
    CHECK(bedrock && AiProvider_getAuth(dir, bedrock) == AI_PROVIDER_AUTH_SPECIAL);
    CHECK(bedrock && AiProvider_getFamily(dir, bedrock) == AI_PROVIDER_FAMILY_NATIVE);

    const AiProviderSlot *zhipu = AiProvider_get(dir, "zhipu-ai");
    CHECK(zhipu && AiProvider_getRegion(dir, zhipu) == AI_PROVIDER_REGION_CHINA);
    CHECK(zhipu && AiProvider_getBaseUrl(dir, zhipu) != NULL);

    CHECK(AiProvider_get(dir, "custom-openai") != NULL);
    CHECK(AiProvider_get(dir, "itera-compute") != NULL);
    CHECK(AiProvider_get(dir, "lilac") != NULL);
    CHECK(AiProvider_get(dir, "definitely-not-a-provider") == NULL);

    // --- family-default resolution on NULL-base rows -----------------------
    const AiProviderSlot *meta = AiProvider_get(dir, "meta");
    CHECK(meta && AiProvider_getBaseUrl(dir, meta) == NULL);
    CHECK(meta && strcmp(AiProvider_resolveBaseUrl(dir, meta),
                         "https://openrouter.ai/api/v1") == 0);

    const AiProviderSlot *vertexAnthropic = AiProvider_get(dir, "vertex-anthropic");
    CHECK(vertexAnthropic && AiProvider_getBaseUrl(dir, vertexAnthropic) == NULL);
    CHECK(vertexAnthropic && strcmp(AiProvider_resolveBaseUrl(dir, vertexAnthropic),
                                    "https://api.anthropic.com/v1") == 0);

    // --- null-safety (Rule 24) ----------------------------------------------
    CHECK(AiProvider_count(NULL) == 0);
    CHECK(AiProvider_get(NULL, NULL) == NULL);
    CHECK(AiProvider_get(dir, NULL) == NULL);
    CHECK(AiProvider_at(NULL, 0) == NULL);
    CHECK(AiProvider_getDisplayName(NULL, NULL) == NULL);
    CHECK(AiProvider_getSlug(NULL, NULL) == NULL);
    CHECK(AiProvider_getNote(NULL, NULL) == NULL);
    CHECK(AiProvider_getFamily(NULL, NULL) == AI_PROVIDER_FAMILY_OPENAI_COMPAT);
    CHECK(AiProvider_resolveBaseUrl(NULL, NULL) == NULL);

    // --- AiChat: URL resolution + auth ---------------------------------------
    AiChat chat = AiChat_2("gpt-4o-mini", "sk-test");
    AiChat_setPeer(&chat, openai);

    char body[2048];
    char url[512];
    ApiAuth auth = ApiAuth_none();

    CHECK(AiChat_buildRequest(&chat, sMessages, 2,
                              body, sizeof(body), url, sizeof(url), &auth));
    CHECK(strcmp(url, "https://api.openai.com/v1/chat/completions") == 0);
    CHECK(auth.kind == API_AUTH_BEARER);
    CHECK(strcmp(auth.credential, "sk-test") == 0);

    // --- AiChat: envelope JSON round-trips through net/json ------------------
    JsonNode nodes[128];
    char scratch[1024];
    JsonDoc doc;
    CHECK(Json_parse(&doc, nodes, 128, scratch, sizeof(scratch), body));
    JsonRef root = Json_root(&doc);

    JsonRef model = Json_member(&doc, root, "model");
    uint32_t modelLen = 0;
    const char *modelView = model >= 0 ? Json_string(&doc, model, &modelLen) : "";
    // Note: Json_string views are NUL-terminated only when escaped into
    // scratch — plain views point into src and need the length (memcmp).
    CHECK(model >= 0 && modelLen == 11 && memcmp(modelView, "gpt-4o-mini", 11) == 0);

    JsonRef msgs = Json_member(&doc, root, "messages");
    CHECK(Json_count(&doc, msgs) == 2);
    JsonRef m0 = Json_at(&doc, msgs, 0);
    JsonRef c0 = Json_member(&doc, m0, "content");
    CHECK(c0 >= 0);
    CHECK(c0 >= 0 && strcmp(Json_string(&doc, c0, NULL), "say \"hi\" \n to me") == 0);
    JsonRef m1 = Json_at(&doc, msgs, 1);
    JsonRef r1 = Json_member(&doc, m1, "role");
    uint32_t roleLen = 0;
    const char *roleView = r1 >= 0 ? Json_string(&doc, r1, &roleLen) : "";
    CHECK(r1 >= 0 && roleLen == 9 && memcmp(roleView, "assistant", 9) == 0);

    // --- AiChat: override + gateway + no-auth paths --------------------------
    AiChat_setBaseUrl(&chat, "https://gateway.example");
    CHECK(AiChat_buildRequest(&chat, sMessages, 2,
                              body, sizeof(body), url, sizeof(url), &auth));
    CHECK(strcmp(url, "https://gateway.example/chat/completions") == 0);

    AiChat bare = AiChat_1("grok-2-latest"); // peer NULL -> gateway
    CHECK(AiChat_buildRequest(&bare, sMessages, 1,
                              body, sizeof(body), url, sizeof(url), &auth));
    CHECK(strcmp(url, "https://openrouter.ai/api/v1/chat/completions") == 0);
    CHECK(auth.kind == API_AUTH_NONE);

    // --- AiChat: guard paths (no network) ------------------------------------
    CHECK(!AiChat_buildRequest(NULL, sMessages, 1,
                               body, sizeof(body), url, sizeof(url), &auth));
    CHECK(!AiChat_buildRequest(&bare, NULL, 1,
                               body, sizeof(body), url, sizeof(url), &auth));
    CHECK(!AiChat_buildRequest(&bare, sMessages, 0,
                               NULL, 0, url, sizeof(url), &auth));
    AiChat noModel = AiChat_0();
    CHECK(!AiChat_buildRequest(&noModel, sMessages, 1,
                               body, sizeof(body), url, sizeof(url), &auth));
    CHECK(!AiChat_complete(&bare, sMessages, 1,
                           body, sizeof(body), NULL));

    // --- AiChat: getters/setters (Rule 24) -----------------------------------
    AiChat c = AiChat_0();
    CHECK(AiChat_getModel(&c) == NULL);
    CHECK(AiChat_getModel(NULL) == NULL);
    AiChat_setProvider(&c, dir);
    AiChat_setPeer(&c, zhipu);
    AiChat_setModel(&c, "glm-4.5");
    AiChat_setApiKey(&c, "k2");
    AiChat_setBaseUrl(&c, "https://override");
    CHECK(AiChat_getProvider(&c) == dir);
    CHECK(AiChat_getPeer(&c) == zhipu);
    CHECK(strcmp(AiChat_getModel(&c), "glm-4.5") == 0);
    CHECK(strcmp(AiChat_getApiKey(&c), "k2") == 0);
    CHECK(strcmp(AiChat_getBaseUrl(&c), "https://override") == 0);

    if (sFailures == 0) {
        printf("ai_provider_test: ALL CHECKS PASSED (%u providers)\n", total);
        return 0;
    }
    printf("ai_provider_test: %d FAILURES\n", sFailures);
    return 1;
}