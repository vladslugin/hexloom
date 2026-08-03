#include "hexloom/agents/agent_cli.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

int run_process_child(int argc, char** argv);
int run_process_runner_tests(const std::string& executable);
int run_agent_event_tests();
int run_agent_prompt_tests();

int main(int argc, char** argv) {
    if (argc >= 2) {
        const int child_result = run_process_child(argc, argv);
        if (child_result >= 0) {
            return child_result;
        }
    }

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
        claude.arguments[6] == "acceptEdits",
        "Claude write access should use acceptEdits"
    );
    check(
        claude.arguments[1] == "implement the selected task",
        "Claude prompt must precede the variadic tool allowlist"
    );
    // --allowedTools swallows every value that follows it, so anything placed
    // after the allowlist is read as another pattern rather than a prompt.
    const auto allowlist = std::ranges::find(
        claude.arguments,
        "--allowedTools"
    );
    check(
        allowlist != claude.arguments.end(),
        "Claude write access should allow the project's verification commands"
    );
    if (allowlist != claude.arguments.end()) {
        check(
            std::ranges::find(claude.arguments, "Bash(ctest *)") !=
                claude.arguments.end(),
            "an agent that may write must be able to run the tests"
        );
        check(
            allowlist < std::ranges::find(claude.arguments, "Bash(cmake *)"),
            "tool patterns must follow the allowlist flag"
        );
    }

    const auto claude_plan = hexloom::agents::make_cli_launch_plan(
        AgentProvider::claude,
        AgentAccess::read_only,
        "review the specification"
    );
    check(
        std::ranges::find(claude_plan.arguments, "--allowedTools") ==
            claude_plan.arguments.end(),
        "read-only planning should grant no commands"
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

    const auto gemini = hexloom::agents::make_cli_launch_plan(
        AgentProvider::gemini,
        AgentAccess::read_only,
        "review the architecture"
    );
    check(
        gemini.executable == "gemini",
        "Gemini executable should be stable"
    );
    check(
        gemini.arguments[3] == "stream-json",
        "Gemini should emit streaming JSON"
    );
    check(
        gemini.arguments[5] == "plan",
        "Gemini read access should use plan mode"
    );
    check(
        hexloom::agents::parse_agent_provider("gemini") ==
            AgentProvider::gemini,
        "Gemini provider should parse"
    );

    const auto antigravity = hexloom::agents::make_cli_launch_plan(
        AgentProvider::antigravity,
        AgentAccess::workspace_write,
        "implement the selected task"
    );
    check(
        antigravity.executable == "agy",
        "Antigravity executable should be stable"
    );
    check(
        antigravity.arguments[3] == "stream-json",
        "Antigravity should emit streaming JSON"
    );
    check(
        antigravity.arguments[5] == "accept-edits",
        "Antigravity write access should use accept-edits"
    );
    check(
        antigravity.arguments.back() == "--sandbox",
        "Antigravity should run tools in its sandbox"
    );
    check(
        hexloom::agents::parse_agent_provider("antigravity") ==
            AgentProvider::antigravity,
        "Antigravity provider should parse"
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

    failures += run_process_runner_tests(argv[0]);
    failures += run_agent_event_tests();
    failures += run_agent_prompt_tests();
    if (failures == 0) {
        std::cout << "Hexloom agent CLI tests passed.\n";
    }
    return failures == 0 ? 0 : 1;
}
