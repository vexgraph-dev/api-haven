// ============================================================================
// APPS: known developer tools — src/apps/apps.inc
// Hand-curated (small, stable) — edit directly, unlike ai/data .inc files
// which are generator-owned. Included ONLY by app_detect.c.
//
// 6 AppSlot rows: the CLI/agents the engine probes for when deciding
// which external tooling is available (opencode orchestrators, coding
// agents, Hermes/Nous tooling). Detection is name-based; see
// ;;INTENTION in app_detect.c for scope boundaries.
// ============================================================================
static const AppSlot kKnownApps[] = {

    {
        "opencode",
        "opencode",
        "agentic coding CLI (opencode.ai)",
    },
    {
        "codex",
        "codex",
        "OpenAI codex CLI",
    },
    {
        "claude",
        "claude",
        "Anthropic Claude Code CLI",
    },
    {
        "cursor",
        "Cursor",
        "Cursor editor app (canonical CLI cursor-agent)",
    },
    {
        "hermes",
        "Hermes",
        "Nous Research Hermes tooling / API client",
    },
    {
        "nous",
        "Nous Research",
        "Nous Research CLI / provider tooling",
    },
};