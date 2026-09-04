# api-haven

Zero-allocation network endpoint schemas and off-heap API integration.

`api-haven` is a lightweight, zero-GC network client and schema library built to connect the `vexgraph` ecosystem to external web services, remote dashboards, and telemetry endpoints.

In typical software, integrating external web APIs introduces massive heap churn: parsing JSON strings, allocating key-value dictionaries, and assembling HTTP multipart bodies with temporary buffers. 

`api-haven` is built under the exact same zero-compromise doctrine as the rest of the stack: **every request payload, JSON object, and telemetry packet is constructed directly in off-heap memory using `vexspoke`'s relational primitives.**

---

## The Production Dogfood: Why Discord First?

Rather than attempting to wrap 50 external APIs at once—resulting in bloated, half-baked boilerplate—`api-haven` deliberately focuses on **Discord webhooks and telemetry streaming as its primary production-validated target**.

By dogfooding the Discord webhook pipeline to its fullest extent:
* **Rich Embed Serialization**: Constructing complex embedded cards, timestamps, color accents, and author thumbnails directly into pre-allocated memory buffers without runtime `malloc`.
* **Multipart File Uploads**: Streaming crash dumps, screenshots, and engine logs over HTTP `multipart/form-data` with zero intermediate copying.
* **Non-Blocking Telemetry**: Transmitting frame-time metrics and engine heartbeats asynchronously so external network latency never stalls Thread 0 or drops presentation frames.

This single, robust implementation serves as the production-proven architectural template for all future web service integrations.

---

## Workspace Integration & How to Use It

`api-haven` sits at Layer 4 in the `@vexgraph-dev` vertical integration stack, depending directly on `vexspoke`:

```
workspace/
├── cmake-build-debug/           # Out-of-tree CMake build artifacts & staged SPVs
├── projects/                    # Vertically integrated subsystem repositories
│   ├── vexspoke/                # Bedrock C23 platform runtime (Layer 1)
│   ├── hotcwap/                 # Dynamic hot-reloading & native OS windowing (Layer 2)
│   ├── darling/                 # Retained-mode UI nodes & Vulkan render passes (Layer 3)
│   ├── api-haven/               # Telemetry schemas & Discord webhook transmitters (this library)
│   │   └── api/                 # C API client, Discord webhook bindings
│   └── [other projects connecting to each other go here]
├── CMakeLists.txt               # Umbrella workspace orchestrator
└── preferences.md               # Engine architectural style preferences (Rules 1–n)
```

### 1. In-Tree Integration (Subdirectory)
When integrated inside an umbrella workspace:

```cmake
# In your top-level CMakeLists.txt
add_subdirectory(projects/api-haven)

add_executable(my_app main.c)
target_link_libraries(my_app PRIVATE api-haven vexspoke)
```

### 2. Standalone Integration (FetchContent Seam)
When building standalone or in downstream projects:

```cmake
if(NOT TARGET api-haven)
    include(FetchContent)
    FetchContent_Declare(
        api-haven
        GIT_REPOSITORY https://github.com/vexgraph-dev/api-haven.git
        GIT_TAG main
    )
    FetchContent_MakeAvailable(api-haven)
endif()

target_link_libraries(my_app PRIVATE api-haven)
```

---

## What's in this repo

* **`api/client.h/.c`** — Base C API client: request configuration, authentication header injection, and connection pooling.
* **`api/discord.h/.c`** — Off-heap Discord webhook transmitter: rich embeds, fields, and file upload serializers.
* **`com/discord/DiscordWebhook.java`** & **`APIClient.java`** — Legacy reference implementation and API contract definitions.
* **`CMakeLists.txt`** — Standalone and monorepo build definitions.

---

## Requirements

* C23 compiler (Clang with `-std=gnu23`).
* `vexspoke` runtime (for `net/http`, `net/json`, `nio/mem`, and `primitive/string`).
* POSIX sockets or Apple SecureTransport/cURL.
* CMake $\ge$ 4.3.
