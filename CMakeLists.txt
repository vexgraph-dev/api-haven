cmake_minimum_required(VERSION 3.28)
project(api_haven C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Release: dead-code stripping so unused symbols cost zero bytes in artifacts.
# (Duplicated per repo for standalone builds; harmless when built in-tree.)
if(APPLE)
    add_compile_options($<$<CONFIG:Release>:-ffunction-sections> $<$<CONFIG:Release>:-fdata-sections>)
    add_link_options($<$<CONFIG:Release>:-Wl,-dead_strip>)
elseif(UNIX)
    add_compile_options($<$<CONFIG:Release>:-ffunction-sections> $<$<CONFIG:Release>:-fdata-sections>)
    add_link_options($<$<CONFIG:Release>:-Wl,--gc-sections>)
endif()

# C-transition seam for the Java API surface at the repo root
# (APIClient.java, com/discord/DiscordWebhook.java).
#
# The Java originals speak the old engine stack (net/HTTPClient, net/JSON,
# primitive/string, nio/ForeignMemory, telemetry/CrashDumper). Their C ports
# land in src/api/ and speak vexspoke instead: src/net (http, json,
# netfacade, url, tls), nio/mem, primitive/string.
#
# Header-only until the first port lands; switch to STATIC with sources
# as files arrive.
if(NOT TARGET api_haven)
add_library(api_haven STATIC
    src/api/client.c
    src/api/auth.c
    src/api/rest.c
    src/com/discord/discord.c
    src/com/slack/slack.c
    src/ai/ai_provider.c
    src/ai/ai_chat.c
)
target_include_directories(api_haven PUBLIC src)
target_compile_options(api_haven PRIVATE -Wall -Wextra -Werror)

if(TARGET vexspoke)
    target_link_libraries(api_haven PUBLIC vexspoke)
else()
    option(API_HAVEN_USE_VEXSPOKE "Depend on vexspoke core via FetchContent" OFF)
    if(API_HAVEN_USE_VEXSPOKE)
        include(FetchContent)
        FetchContent_Declare(
            vexspoke
            GIT_REPOSITORY https://github.com/vexgraph-dev/vexspoke.git
            GIT_TAG main
        )
        FetchContent_MakeAvailable(vexspoke)
        target_include_directories(api_haven PUBLIC ${vexspoke_SOURCE_DIR}/src)
        target_link_libraries(api_haven PUBLIC vexspoke)
    endif()
endif()
endif() # NOT TARGET api_haven

# Port queue (Java => C, in dependency order):
# 1. src/api/client.c — APIClient.sendTelemetry (needs net/http + net/json)
# 2. src/api/discord.c — DiscordWebhook sendText/sendEmbed/sendRichEmbed/
#    sendEmbedWithFile/sendTextFile/sendFile + transmit() (needs net/http
#    request with headers + bodyLen, io/file for attachments)

# Shape-A/B loopback proof: headless, no accounts (server on 127.0.0.1).
if(NOT TARGET webhook_loopback_test)
    add_executable(webhook_loopback_test
        src/tests/webhook_loopback_test.c
    )
    target_link_libraries(webhook_loopback_test PRIVATE api_haven)
endif()

# src/ai directory proof: provider table + OpenAI-compatible envelope.
# Headless: no network; buildRequest only (see ai_provider_test.c overview).
if(NOT TARGET ai_provider_test)
    add_executable(ai_provider_test
        src/ai/tests/ai_provider_test.c
    )
    target_link_libraries(ai_provider_test PRIVATE api_haven)
endif()
