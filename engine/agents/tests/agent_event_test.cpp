#include "hexloom/agents/agent_event.hpp"

#include <iostream>
#include <string>

int run_agent_event_tests() {
    using hexloom::agents::AgentEventStream;
    using hexloom::agents::AgentEventType;
    using hexloom::agents::AgentProvider;

    int failures = 0;
    const auto check = [&failures](bool condition, const char* message) {
        if (!condition) {
            std::cerr << "FAIL: " << message << '\n';
            ++failures;
        }
    };

    const auto codex = hexloom::agents::normalize_agent_event(
        AgentProvider::codex,
        R"({"type":"item.completed","item":{"type":"agent_message","text":"ready"}})"
    );
    check(codex.size() == 1, "Codex message should emit one event");
    check(
        codex.front().type == AgentEventType::message &&
            codex.front().text == "ready",
        "Codex agent message should normalize"
    );

    const auto claude = hexloom::agents::normalize_agent_event(
        AgentProvider::claude,
        R"({"type":"assistant","session_id":"s1","message":{"content":[{"type":"text","text":"hello"},{"type":"tool_use","name":"Read"}]}})"
    );
    check(claude.size() == 2, "Claude mixed content should emit two events");
    check(
        claude[0].type == AgentEventType::message &&
            claude[1].type == AgentEventType::tool_started,
        "Claude text and tool use should normalize in order"
    );

    // Shapes taken from a real authenticated Claude Code run.
    const auto allowed_quota = hexloom::agents::normalize_agent_event(
        AgentProvider::claude,
        R"({"type":"rate_limit_event","rate_limit_info":{"status":"allowed","rateLimitType":"five_hour"},"session_id":"s1"})"
    );
    check(
        allowed_quota.empty(),
        "an allowing rate limit report should not become execution history"
    );

    const auto limited_quota = hexloom::agents::normalize_agent_event(
        AgentProvider::claude,
        R"({"type":"rate_limit_event","rate_limit_info":{"status":"rejected"},"session_id":"s1"})"
    );
    check(
        limited_quota.size() == 1 &&
            limited_quota.front().text == "Provider rate limit: rejected",
        "a refusing rate limit report should be readable"
    );

    const auto claude_result = hexloom::agents::normalize_agent_event(
        AgentProvider::claude,
        R"({"is_error":false,"session_id":"s1","subtype":"success","result":"READY","type":"result"})"
    );
    check(
        claude_result.size() == 1 &&
            claude_result.front().type == AgentEventType::completed &&
            claude_result.front().text == "READY",
        "a successful Claude result should complete with its text"
    );

    const auto claude_failure = hexloom::agents::normalize_agent_event(
        AgentProvider::claude,
        R"({"is_error":true,"session_id":"s1","subtype":"success","result":"Not logged in","type":"result"})"
    );
    check(
        claude_failure.size() == 1 &&
            claude_failure.front().type == AgentEventType::failed,
        "is_error should outrank a success subtype"
    );

    AgentEventStream antigravity(AgentProvider::antigravity);
    auto first = antigravity.feed(
        R"({"event":"init","conversation_id":"agy-1"})"
        "\n{\"event\":\"step_up"
    );
    check(
        first.size() == 1 &&
            first.front().type == AgentEventType::session_started,
        "complete Antigravity line should emit immediately"
    );
    const auto second = antigravity.feed(
        "date\",\"step_update\":{\"conversation_id\":\"agy-1\","
        "\"state\":\"DONE\",\"step_type\":\"agent_response\","
        "\"text_delta\":\"ok\"}}\n"
    );
    check(
        second.size() == 1 &&
            second.front().type == AgentEventType::message &&
            second.front().text == "ok",
        "split Antigravity line should be reassembled"
    );

    const auto malformed = hexloom::agents::normalize_agent_event(
        AgentProvider::antigravity,
        "{not-json"
    );
    check(
        malformed.size() == 1 &&
            malformed.front().type == AgentEventType::protocol_error,
        "malformed provider output should become a protocol error"
    );

    return failures;
}
