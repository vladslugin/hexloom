#pragma once

#include "hexloom/agents/agent_event.hpp"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

namespace hexloom::godot_adapter {

class HexloomAgentBridge : public godot::RefCounted {
    GDCLASS(HexloomAgentBridge, godot::RefCounted)

public:
    HexloomAgentBridge() = default;
    ~HexloomAgentBridge() override;

    [[nodiscard]] godot::Dictionary start(
        const godot::String& provider,
        const godot::String& access,
        const godot::String& working_directory,
        const godot::String& prompt
    );
    void cancel();
    [[nodiscard]] bool is_running() const;
    [[nodiscard]] godot::Array poll_events();

protected:
    static void _bind_methods();

private:
    void push_events(std::vector<hexloom::agents::AgentEvent> events);
    void stop_and_join();

    std::atomic_bool cancel_requested_{false};
    std::atomic_bool running_{false};
    std::mutex events_mutex_;
    std::vector<hexloom::agents::AgentEvent> pending_events_;
    std::thread worker_;
};

}  // namespace hexloom::godot_adapter
