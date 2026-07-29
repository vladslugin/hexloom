#include "hexloom/agents/agent_cli.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

int main() {
    using hexloom::agents::AgentAccess;
    using hexloom::agents::AgentProvider;

    int failures = 0;
    const auto check = [&failures](bool condition, const char* message) {
        if (!condition) {
            std::cerr << "FAIL: " << message << '\n';
            ++failures;
        }
    };

    const auto codex = hexloom::agents::make_cli_launch_plan(
        AgentProvider::codex,
        AgentAccess::read_only,
        "inspect; do not let the shell parse this"
    );
    check(codex.executable == "codex", "Codex executable should be stable");
    check(
        codex.arguments.size() == 5,
        "Codex launch should have five arguments"
    );
    check(
        codex.arguments[3] == "read-only",
        "Codex should default to its read-only sandbox"
    );
    check(
        codex.arguments.back() == "inspect; do not let the shell parse this",
        "prompt should remain one argv component"
    );

    const auto claude = hexloom::agents::make_cli_launch_plan(
        AgentProvider::claude,
        AgentAccess::workspace_write,
        "implement the selected task"
    );
    check(
        claude.arguments[5] == "acceptEdits",
        "Claude write access should use acceptEdits"
    );
    check(
        hexloom::agents::parse_agent_provider("claude") ==
            AgentProvider::claude,
        "Claude provider should parse"
    );
    check(
        !hexloom::agents::parse_agent_provider("unknown").has_value(),
        "unknown providers should be rejected"
    );

    bool empty_prompt_rejected = false;
    try {
        static_cast<void>(hexloom::agents::make_cli_launch_plan(
            AgentProvider::codex,
            AgentAccess::read_only,
            ""
        ));
    } catch (const std::invalid_argument&) {
        empty_prompt_rejected = true;
    }
    check(empty_prompt_rejected, "empty prompts should be rejected");

    if (failures == 0) {
        std::cout << "Hexloom agent CLI tests passed.\n";
    }
    return failures == 0 ? 0 : 1;
}
