#include "hexloom/godot/hexloom_agent_bridge.hpp"

#include "hexloom/agents/agent_cli.hpp"
#include "hexloom/agents/agent_prompt.hpp"
#include "hexloom/agents/process_runner.hpp"
#include "hexloom/core/project_memory.hpp"

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

godot::Dictionary HexloomAgentBridge::compose_prompt(
    const godot::String& memory_path,
    const godot::String& access_name,
    const godot::String& direction
) {
    godot::Dictionary result;
    result["ok"] = false;
    result["prompt"] = godot::String();
    result["total"] = 0;
    result["locked"] = 0;
    result["error"] = godot::String();

    const auto access_text = to_std_string(access_name);
    hexloom::agents::AgentAccess access;
    if (access_text == "read_only") {
        access = hexloom::agents::AgentAccess::read_only;
    } else if (access_text == "workspace_write") {
        access = hexloom::agents::AgentAccess::workspace_write;
    } else {
        result["error"] = godot::String("Unknown agent access mode.");
        return result;
    }

    const auto direction_text = to_std_string(direction);
    if (direction_text.empty()) {
        result["error"] = godot::String("A direction must not be empty.");
        return result;
    }

    const auto loaded =
        hexloom::load_project_memory(to_std_string(memory_path));
    if (!loaded.ok()) {
        std::string message = "Project memory is unusable.";
        if (!loaded.issues.empty()) {
            message = loaded.issues.front().field + ": " +
                loaded.issues.front().message;
        }
        result["error"] = godot::String(message.c_str());
        return result;
    }

    const auto& memory = *loaded.memory;
    std::size_t locked = 0;
    for (const auto& entry : memory.entries) {
        if (entry.locked) {
            ++locked;
        }
    }

    result["ok"] = true;
    result["prompt"] = godot::String(
        hexloom::agents::compile_agent_prompt(memory, access, direction_text)
            .c_str()
    );
    result["total"] = static_cast<int>(memory.entries.size());
    result["locked"] = static_cast<int>(locked);
    for (const auto category : {
             hexloom::MemoryCategory::visual_style,
             hexloom::MemoryCategory::mechanics,
             hexloom::MemoryCategory::constraint,
             hexloom::MemoryCategory::decision,
         }) {
        result[godot::String(hexloom::to_string(category).data())] =
            static_cast<int>(memory.count(category));
    }
    return result;
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
    godot::ClassDB::bind_method(
        godot::D_METHOD(
            "compose_prompt",
            "memory_path",
            "access",
            "direction"
        ),
        &HexloomAgentBridge::compose_prompt
    );
}

}  // namespace hexloom::godot_adapter
