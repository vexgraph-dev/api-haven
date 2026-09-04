cmake_minimum_required(VERSION 3.28)
project(api_haven C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

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
    src/com/discord/discord.c
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
