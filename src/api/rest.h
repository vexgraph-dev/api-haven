#ifndef API_REST_H
#define API_REST_H

#include <stdbool.h>
#include <stddef.h>

#include "api/auth.h"
#include "net/http.h"

// api/rest.h — one REST core for every Shape-B/C driver (R2, vexspoke-only).
//
// Drivers build a JSON body and call Rest_postJson (or Rest_get); URL
// parsing, auth injection, and transport live here exactly once. Responses
// reuse HttpResponse (caller-owned body buffer) — no wrapper types.
// This file is the canonical home of parseUrl; the older copies in
// api/client.c and com/discord/discord.c migrate here on a later turn.

bool Rest_postJson(const char *url, const ApiAuth *auth,
                   const char *jsonBody, size_t jsonLen, HttpResponse *resp);
bool Rest_get(const char *url, const ApiAuth *auth, HttpResponse *resp);

#endif
