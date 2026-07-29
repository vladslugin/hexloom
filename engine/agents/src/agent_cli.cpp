#include "hexloom/agents/agent_cli.hpp"

#include <sstream>
#include <stdexcept>

namespace hexloom::agents {

std::string_view to_string(AgentProvider provider) {
    switch (provider) {
        case AgentProvider::codex:
            return "codex";
        case AgentProvider::claude:
            return "claude";
        case AgentProvider::gemini:
            return "gemini";
    }
    return "unknown";
}

std::string_view to_string(AgentAccess access) {
    switch (access) {
        case AgentAccess::read_only:
            return "read_only";
        case AgentAccess::workspace_write:
            return "workspace_write";
    }
    return "unknown";
}

std::string_view to_string(AgentTransport transport) {
    switch (transport) {
        case AgentTransport::json_lines:
            return "json_lines";
        case AgentTransport::json_rpc:
            return "json_rpc";
    }
    return "unknown";
}

std::optional<AgentProvider> parse_agent_provider(
    std::string_view value
) {
    if (value == "codex") {
        return AgentProvider::codex;
    }
    if (value == "claude") {
        return AgentProvider::claude;
    }
    if (value == "gemini") {
        return AgentProvider::gemini;
    }
    return std::nullopt;
}

AgentLaunchPlan make_cli_launch_plan(
    AgentProvider provider,
    AgentAccess access,
    std::string prompt
) {
    if (prompt.empty()) {
        throw std::invalid_argument("Agent prompt must not be empty.");
    }

    switch (provider) {
        case AgentProvider::codex:
            return {
                .provider = provider,
                .executable = "codex",
                .arguments = {
                    "exec",
                    "--json",
                    "--sandbox",
                    access == AgentAccess::read_only
                        ? "read-only"
                        : "workspace-write",
                    std::move(prompt),
                },
                .transport = AgentTransport::json_lines,
                .reuses_user_login = true,
            };
        case AgentProvider::claude:
            return {
                .provider = provider,
                .executable = "claude",
                .arguments = {
                    "-p",
                    "--output-format",
                    "stream-json",
                    "--verbose",
                    "--permission-mode",
                    access == AgentAccess::read_only
                        ? "plan"
                        : "acceptEdits",
                    std::move(prompt),
                },
                .transport = AgentTransport::json_lines,
                .reuses_user_login = true,
            };
        case AgentProvider::gemini:
            return {
                .provider = provider,
                .executable = "gemini",
                .arguments = {
                    "--prompt",
                    std::move(prompt),
                    "--output-format",
                    "stream-json",
                    "--approval-mode",
                    access == AgentAccess::read_only
                        ? "plan"
                        : "auto_edit",
                },
                .transport = AgentTransport::json_lines,
                .reuses_user_login = true,
            };
    }
    throw std::invalid_argument("Unsupported agent provider.");
}

std::string format_launch_plan(const AgentLaunchPlan& plan) {
    std::ostringstream output;
    output << "provider: " << to_string(plan.provider) << '\n'
           << "executable: " << plan.executable << '\n'
           << "transport: " << to_string(plan.transport) << '\n'
           << "reuses_user_login: "
           << (plan.reuses_user_login ? "true" : "false") << '\n'
           << "argv:\n";
    for (const auto& argument : plan.arguments) {
        output << "  - " << argument << '\n';
    }
    return output.str();
}

}  // namespace hexloom::agents
