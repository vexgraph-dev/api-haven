#include "annotation/overview.h"
#include "apps/app_detect.h"

#include <stdio.h>
#include <string.h>

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: AppDetectTest (src/apps/tests/app_detect_test.c)
 * LEVEL: L2 — Behavior verification (headless; POSIX probes only)
 * ============================================================================
 * Executable proof of the apps/ directory: registry shape (count, get,
 * getters), PATH resolution (which/isOnPath), /Applications bundle
 * scanning, and every null-safety guard.
 *
 * Environment-tolerant assertions: `ls`/`sh` are guaranteed on any POSIX
 * PATH; bundle checks assert absence for a nonsense name and report
 * known-tool install state (opencode, hermes, nous) without failing on
 * machines where they are absent.
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

    AppDetect *detect = AppDetect_shared();
    CHECK(detect != NULL);
    printf("AppDetect_count = %u\n", AppDetect_count(detect));
    CHECK(AppDetect_count(detect) == 6);

    // --- registry ------------------------------------------------------------
    const AppSlot *opencode = AppDetect_get(detect, "opencode");
    CHECK(opencode != NULL);
    CHECK(opencode && strcmp(AppDetect_getName(detect, opencode), "opencode") == 0);
    CHECK(opencode && AppDetect_getDisplayName(detect, opencode) != NULL);
    CHECK(AppDetect_get(detect, "hermes") != NULL);
    CHECK(AppDetect_get(detect, "nous") != NULL);
    CHECK(AppDetect_at(detect, 0) != NULL);
    CHECK(AppDetect_at(detect, AppDetect_count(detect)) == NULL);

    // --- PATH resolution ------------------------------------------------------
    char path[4096];
    CHECK(AppDetect_which(detect, "ls", path, sizeof(path)));
    CHECK(strstr(path, "/ls") != NULL);
    CHECK(AppDetect_isOnPath(detect, "sh"));
    CHECK(!AppDetect_isOnPath(detect, "definitely-not-a-bin-xyzzy"));
    CHECK(!AppDetect_which(detect, "definitely-not-a-bin-xyzzy", path, sizeof(path)));

    // --- app bundles ----------------------------------------------------------
    CHECK(!AppDetect_isAppBundle(detect, "definitely-not-an-app-xyzzy"));
    printf("isAppBundle(Safari) = %s\n",
           AppDetect_isAppBundle(detect, "Safari") ? "yes" : "no");
    printf("isInstalled(opencode) = %s\n",
           AppDetect_isInstalled(detect, opencode) ? "yes" : "no");
    printf("isInstalled(hermes) = %s\n",
           AppDetect_isInstalled(detect, AppDetect_get(detect, "hermes")) ? "yes" : "no");
    printf("isInstalled(nous) = %s\n",
           AppDetect_isInstalled(detect, AppDetect_get(detect, "nous")) ? "yes" : "no");

    // --- null-safety (Rule 24) ------------------------------------------------
    CHECK(AppDetect_count(NULL) == 0);
    CHECK(AppDetect_get(NULL, NULL) == NULL);
    CHECK(AppDetect_get(detect, NULL) == NULL);
    CHECK(AppDetect_at(NULL, 0) == NULL);
    CHECK(AppDetect_getName(NULL, NULL) == NULL);
    CHECK(AppDetect_getNote(NULL, NULL) == NULL);
    CHECK(AppDetect_isOnPath(NULL, "ls") == false);
    CHECK(AppDetect_isAppBundle(NULL, "Safari") == false);
    CHECK(AppDetect_isInstalled(NULL, NULL) == false);
    CHECK(!AppDetect_which(NULL, "ls", path, sizeof(path)));
    CHECK(!AppDetect_which(detect, "ls", NULL, 0));

    if (sFailures == 0) {
        printf("app_detect_test: ALL CHECKS PASSED\n");
        return 0;
    }
    printf("app_detect_test: %d FAILURES\n", sFailures);
    return 1;
}