#include "hexloom/godot/hexloom_agent_bridge.hpp"

#include "hexloom/agents/agent_cli.hpp"
#include "hexloom/agents/process_runner.hpp"

#include <godot_cpp/core/class_db.hpp>

#include <chrono>
#include <exception>
#include <filesystem>
#include <string>
#include <utility>

namespace hexloom::godot_adapter {
namespace {

[[nodiscard]] std::string to_std_string(const godot::String& value) {
    return value.utf8().get_data();
}

[[nodiscard]] godot::Dictionary status(
    bool started,
    const std::string& error = {}
) {
    godot::Dictionary result;
    result["started"] = started;
    result["error"] = godot::String(error.c_str());
    return result;
}

[[nodiscard]] hexloom::agents::AgentEvent terminal_event(
    hexloom::agents::AgentProvider provider,
    hexloom::agents::AgentEventType type,
    std::string text
) {
    return {
        .provider = provider,
        .type = type,
        .session_id = {},
        .text = std::move(text),
        .raw_json = {},
    };
}

}  // namespace

HexloomAgentBridge::~HexloomAgentBridge() {
    stop_and_join();
}

godot::Dictionary HexloomAgentBridge::start(
    const godot::String& provider_name,
    const godot::String& access_name,
    const godot::String& working_directory,
    const godot::String& prompt
) {
    if (running_.load()) {
        return status(false, "An agent is already running.");
    }
    if (worker_.joinable()) {
        worker_.join();
    }

    const auto provider = hexloom::agents::parse_agent_provider(
        to_std_string(provider_name)
    );
    if (!provider.has_value()) {
        return status(false, "Unknown agent provider.");
    }

    const auto access_text = to_std_string(access_name);
    hexloom::agents::AgentAccess access;
    if (access_text == "read_only") {
        access = hexloom::agents::AgentAccess::read_only;
    } else if (access_text == "workspace_write") {
        access = hexloom::agents::AgentAccess::workspace_write;
    } else {
        return status(false, "Unknown agent access mode.");
    }

    const auto prompt_text = to_std_string(prompt);
    if (prompt_text.empty()) {
        return status(false, "Agent prompt must not be empty.");
    }
    if (prompt_text.size() > 32 * 1024) {
        return status(false, "Agent prompt exceeds 32 KiB.");
    }

    const std::filesystem::path directory =
        to_std_string(working_directory);
    if (!std::filesystem::is_directory(directory)) {
        return status(false, "Working directory does not exist.");
    }

    hexloom::agents::AgentLaunchPlan plan;
    try {
        plan = hexloom::agents::make_cli_launch_plan(
            *provider,
            access,
            prompt_text
        );
    } catch (const std::exception& error) {
        return status(false, error.what());
    }

    {
        std::scoped_lock lock(events_mutex_);
        pending_events_.clear();
    }
    cancel_requested_.store(false);
    running_.store(true);

    worker_ = std::thread(
        [this, plan = std::move(plan), directory]() mutable {
            hexloom::agents::AgentEventStream stream(plan.provider);
            const hexloom::agents::ProcessRequest request{
                .executable = plan.executable,
                .arguments = plan.arguments,
                .working_directory = directory,
                .timeout = std::chrono::minutes(15),
                .inherit_standard_input = false,
            };
            const auto result = hexloom::agents::run_process(
                request,
                [this, &stream](
                    const hexloom::agents::ProcessOutput& output
                ) {
                    if (
                        output.stream ==
                        hexloom::agents::ProcessStream::standard_output
                    ) {
                        push_events(stream.feed(output.data));
                    }
                },
                &cancel_requested_
            );
            push_events(stream.finish());

            if (!result.launch_error.empty()) {
                push_events({terminal_event(
                    plan.provider,
                    hexloom::agents::AgentEventType::failed,
                    result.launch_error
                )});
            } else if (result.cancelled) {
                push_events({terminal_event(
                    plan.provider,
                    hexloom::agents::AgentEventType::failed,
                    "Agent run cancelled."
                )});
            } else if (!result.ok()) {
                auto message = result.standard_error;
                if (message.empty()) {
                    message =
                        "Agent exited with code " +
                        std::to_string(result.exit_code) + ".";
                }
                push_events({terminal_event(
                    plan.provider,
                    hexloom::agents::AgentEventType::failed,
                    std::move(message)
                )});
            }
            running_.store(false);
        }
    );
    return status(true);
}

void HexloomAgentBridge::cancel() {
    cancel_requested_.store(true);
}

bool HexloomAgentBridge::is_running() const {
    return running_.load();
}

godot::Array HexloomAgentBridge::poll_events() {
    std::vector<hexloom::agents::AgentEvent> events;
    {
        std::scoped_lock lock(events_mutex_);
        events.swap(pending_events_);
    }

    godot::Array serialized;
    for (const auto& event : events) {
        godot::Dictionary item;
        item["provider"] = godot::String(
            hexloom::agents::to_string(event.provider).data()
        );
        item["type"] = godot::String(
            hexloom::agents::to_string(event.type).data()
        );
        item["session_id"] = godot::String(event.session_id.c_str());
        item["text"] = godot::String(event.text.c_str());
        serialized.push_back(item);
    }
    return serialized;
}

void HexloomAgentBridge::push_events(
    std::vector<hexloom::agents::AgentEvent> events
) {
    if (events.empty()) {
        return;
    }
    std::scoped_lock lock(events_mutex_);
    for (auto& event : events) {
        pending_events_.push_back(std::move(event));
    }
}

void HexloomAgentBridge::stop_and_join() {
    cancel_requested_.store(true);
    if (worker_.joinable()) {
        worker_.join();
    }
    running_.store(false);
}

void HexloomAgentBridge::_bind_methods() {
    godot::ClassDB::bind_method(
        godot::D_METHOD(
            "start",
            "provider",
            "access",
            "working_directory",
            "prompt"
        ),
        &HexloomAgentBridge::start
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("cancel"),
        &HexloomAgentBridge::cancel
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("is_running"),
        &HexloomAgentBridge::is_running
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("poll_events"),
        &HexloomAgentBridge::poll_events
    );
}

}  // namespace hexloom::godot_adapter
