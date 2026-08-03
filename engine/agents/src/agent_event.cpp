#include "hexloom/agents/agent_event.hpp"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace hexloom::agents {
namespace {

constexpr std::size_t maximum_pending_bytes = 1024 * 1024;

[[nodiscard]] std::string scalar(
    const YAML::Node& node,
    const char* key
) {
    const auto value = node[key];
    if (!value || !value.IsScalar()) {
        return {};
    }
    return value.as<std::string>();
}

[[nodiscard]] AgentEvent event(
    AgentProvider provider,
    AgentEventType type,
    std::string session_id,
    std::string text,
    std::string_view raw
) {
    return {
        .provider = provider,
        .type = type,
        .session_id = std::move(session_id),
        .text = std::move(text),
        .raw_json = std::string(raw),
    };
}

[[nodiscard]] std::vector<AgentEvent> normalize_codex(
    const YAML::Node& root,
    std::string_view raw
) {
    const auto type = scalar(root, "type");
    if (type == "thread.started") {
        return {event(
            AgentProvider::codex,
            AgentEventType::session_started,
            scalar(root, "thread_id"),
            {},
            raw
        )};
    }
    if (type == "turn.started") {
        return {event(
            AgentProvider::codex,
            AgentEventType::progress,
            {},
            "turn_started",
            raw
        )};
    }
    if (type == "item.started" || type == "item.completed") {
        const auto item = root["item"];
        const auto item_type = scalar(item, "type");
        if (item_type == "agent_message") {
            return {event(
                AgentProvider::codex,
                AgentEventType::message,
                {},
                scalar(item, "text"),
                raw
            )};
        }
        const bool completed = type == "item.completed";
        auto description = scalar(item, "command");
        if (description.empty()) {
            description = item_type;
        }
        return {event(
            AgentProvider::codex,
            completed
                ? AgentEventType::tool_completed
                : AgentEventType::tool_started,
            {},
            std::move(description),
            raw
        )};
    }
    if (type == "turn.completed") {
        return {event(
            AgentProvider::codex,
            AgentEventType::completed,
            {},
            {},
            raw
        )};
    }
    if (type == "turn.failed" || type == "error") {
        auto message = scalar(root, "message");
        if (message.empty()) {
            message = scalar(root["error"], "message");
        }
        return {event(
            AgentProvider::codex,
            AgentEventType::failed,
            {},
            std::move(message),
            raw
        )};
    }
    return {event(
        AgentProvider::codex,
        AgentEventType::progress,
        {},
        type,
        raw
    )};
}

[[nodiscard]] std::vector<AgentEvent> normalize_claude(
    const YAML::Node& root,
    std::string_view raw
) {
    const auto type = scalar(root, "type");
    const auto session_id = scalar(root, "session_id");
    if (type == "system" && scalar(root, "subtype") == "init") {
        return {event(
            AgentProvider::claude,
            AgentEventType::session_started,
            session_id,
            {},
            raw
        )};
    }
    if (type == "assistant") {
        std::vector<AgentEvent> events;
        const auto content = root["message"]["content"];
        if (content && content.IsSequence()) {
            for (const auto& block : content) {
                const auto block_type = scalar(block, "type");
                if (block_type == "text") {
                    events.push_back(event(
                        AgentProvider::claude,
                        AgentEventType::message,
                        session_id,
                        scalar(block, "text"),
                        raw
                    ));
                } else if (block_type == "tool_use") {
                    events.push_back(event(
                        AgentProvider::claude,
                        AgentEventType::tool_started,
                        session_id,
                        scalar(block, "name"),
                        raw
                    ));
                }
            }
        }
        if (!events.empty()) {
            return events;
        }
    }
    if (type == "user") {
        return {event(
            AgentProvider::claude,
            AgentEventType::tool_completed,
            session_id,
            "tool_result",
            raw
        )};
    }
    // Claude reports quota accounting alongside the work. While the quota
    // allows the run this is not execution history, and surfacing it would
    // put a protocol word in front of the creator for no reason.
    if (type == "rate_limit_event") {
        const auto status = scalar(root["rate_limit_info"], "status");
        if (status.empty() || status == "allowed") {
            return {};
        }
        return {event(
            AgentProvider::claude,
            AgentEventType::progress,
            session_id,
            "Provider rate limit: " + status,
            raw
        )};
    }
    if (type == "result") {
        const bool failed =
            root["is_error"] && root["is_error"].as<bool>();
        return {event(
            AgentProvider::claude,
            failed ? AgentEventType::failed : AgentEventType::completed,
            session_id,
            scalar(root, "result"),
            raw
        )};
    }
    return {event(
        AgentProvider::claude,
        AgentEventType::progress,
        session_id,
        type,
        raw
    )};
}

[[nodiscard]] std::vector<AgentEvent> normalize_antigravity(
    const YAML::Node& root,
    std::string_view raw
) {
    const auto kind = scalar(root, "event");
    if (kind == "init") {
        return {event(
            AgentProvider::antigravity,
            AgentEventType::session_started,
            scalar(root, "conversation_id"),
            {},
            raw
        )};
    }
    if (kind == "step_update") {
        const auto update = root["step_update"];
        const auto step_type = scalar(update, "step_type");
        const auto state = scalar(update, "state");
        if (step_type == "agent_response") {
            return {event(
                AgentProvider::antigravity,
                AgentEventType::message,
                scalar(update, "conversation_id"),
                scalar(update, "text_delta"),
                raw
            )};
        }
        if (step_type.find("tool") != std::string::npos ||
            step_type.find("command") != std::string::npos) {
            return {event(
                AgentProvider::antigravity,
                state == "DONE"
                    ? AgentEventType::tool_completed
                    : AgentEventType::tool_started,
                scalar(update, "conversation_id"),
                step_type,
                raw
            )};
        }
        return {event(
            AgentProvider::antigravity,
            AgentEventType::progress,
            scalar(update, "conversation_id"),
            step_type + ":" + state,
            raw
        )};
    }
    if (kind == "result") {
        const auto result = root["result"];
        const auto status = scalar(result, "status");
        auto text = scalar(result, "response");
        if (text.empty()) {
            text = scalar(result, "error");
        }
        return {event(
            AgentProvider::antigravity,
            status == "SUCCESS"
                ? AgentEventType::completed
                : AgentEventType::failed,
            scalar(result, "conversation_id"),
            std::move(text),
            raw
        )};
    }
    return {event(
        AgentProvider::antigravity,
        AgentEventType::progress,
        {},
        kind,
        raw
    )};
}

}  // namespace

std::string_view to_string(AgentEventType type) {
    switch (type) {
        case AgentEventType::session_started:
            return "session_started";
        case AgentEventType::progress:
            return "progress";
        case AgentEventType::message:
            return "message";
        case AgentEventType::tool_started:
            return "tool_started";
        case AgentEventType::tool_completed:
            return "tool_completed";
        case AgentEventType::completed:
            return "completed";
        case AgentEventType::failed:
            return "failed";
        case AgentEventType::protocol_error:
            return "protocol_error";
    }
    return "unknown";
}

std::vector<AgentEvent> normalize_agent_event(
    AgentProvider provider,
    std::string_view json_line
) {
    try {
        const auto root = YAML::Load(std::string(json_line));
        if (!root.IsMap()) {
            throw YAML::ParserException(
                YAML::Mark::null_mark(),
                "event root is not an object"
            );
        }
        switch (provider) {
            case AgentProvider::codex:
                return normalize_codex(root, json_line);
            case AgentProvider::claude:
                return normalize_claude(root, json_line);
            case AgentProvider::antigravity:
                return normalize_antigravity(root, json_line);
            case AgentProvider::gemini:
                return {event(
                    AgentProvider::gemini,
                    AgentEventType::progress,
                    {},
                    scalar(root, "type"),
                    json_line
                )};
        }
    } catch (const YAML::Exception& error) {
        return {event(
            provider,
            AgentEventType::protocol_error,
            {},
            error.what(),
            json_line
        )};
    }
    return {};
}

AgentEventStream::AgentEventStream(AgentProvider provider)
    : provider_(provider) {}

std::vector<AgentEvent> AgentEventStream::feed(std::string_view chunk) {
    std::vector<AgentEvent> events;
    pending_.append(chunk);

    std::size_t newline = 0;
    while ((newline = pending_.find('\n')) != std::string::npos) {
        auto line = pending_.substr(0, newline);
        pending_.erase(0, newline + 1);
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!line.empty()) {
            auto normalized = normalize_agent_event(provider_, line);
            events.insert(
                events.end(),
                std::make_move_iterator(normalized.begin()),
                std::make_move_iterator(normalized.end())
            );
        }
    }
    if (pending_.size() > maximum_pending_bytes) {
        events.push_back(event(
            provider_,
            AgentEventType::protocol_error,
            {},
            "Agent event exceeded the one MiB line limit.",
            {}
        ));
        pending_.clear();
    }
    return events;
}

std::vector<AgentEvent> AgentEventStream::finish() {
    if (pending_.empty()) {
        return {};
    }
    auto line = std::move(pending_);
    pending_.clear();
    return normalize_agent_event(provider_, line);
}

}  // namespace hexloom::agents
