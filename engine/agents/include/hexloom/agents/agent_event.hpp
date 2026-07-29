#pragma once

#include "hexloom/agents/agent_cli.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace hexloom::agents {

enum class AgentEventType {
    session_started,
    progress,
    message,
    tool_started,
    tool_completed,
    completed,
    failed,
    protocol_error,
};

struct AgentEvent {
    AgentProvider provider;
    AgentEventType type;
    std::string session_id;
    std::string text;
    std::string raw_json;
};

[[nodiscard]] std::string_view to_string(AgentEventType type);

[[nodiscard]] std::vector<AgentEvent> normalize_agent_event(
    AgentProvider provider,
    std::string_view json_line
);

class AgentEventStream {
public:
    explicit AgentEventStream(AgentProvider provider);

    [[nodiscard]] std::vector<AgentEvent> feed(std::string_view chunk);
    [[nodiscard]] std::vector<AgentEvent> finish();

private:
    AgentProvider provider_;
    std::string pending_;
};

}  // namespace hexloom::agents
