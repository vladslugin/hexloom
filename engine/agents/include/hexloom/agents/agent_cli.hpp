#pragma once

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace hexloom::agents {

enum class AgentProvider {
    codex,
    claude,
    gemini,
};

enum class AgentAccess {
    read_only,
    workspace_write,
};

enum class AgentTransport {
    json_lines,
    json_rpc,
};

struct AgentLaunchPlan {
    AgentProvider provider;
    std::string executable;
    std::vector<std::string> arguments;
    AgentTransport transport;
    bool reuses_user_login;
};

[[nodiscard]] std::string_view to_string(AgentProvider provider);
[[nodiscard]] std::string_view to_string(AgentAccess access);
[[nodiscard]] std::string_view to_string(AgentTransport transport);
[[nodiscard]] std::optional<AgentProvider> parse_agent_provider(
    std::string_view value
);

// Creates argv components, not a shell command. The process supervisor must
// pass them directly to the OS so prompts cannot be interpreted by a shell.
[[nodiscard]] AgentLaunchPlan make_cli_launch_plan(
    AgentProvider provider,
    AgentAccess access,
    std::string prompt
);

[[nodiscard]] std::string format_launch_plan(
    const AgentLaunchPlan& plan
);

}  // namespace hexloom::agents
