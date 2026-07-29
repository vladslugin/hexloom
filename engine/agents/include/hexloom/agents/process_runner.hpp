#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace hexloom::agents {

enum class ProcessStream {
    standard_output,
    standard_error,
};

struct ProcessOutput {
    ProcessStream stream;
    std::string data;
};

struct ProcessRequest {
    std::string executable;
    std::vector<std::string> arguments;
    std::filesystem::path working_directory;
    std::chrono::milliseconds timeout{0};
};

struct ProcessResult {
    int exit_code = -1;
    bool cancelled = false;
    bool timed_out = false;
    std::string launch_error;
    std::string standard_output;
    std::string standard_error;

    [[nodiscard]] bool ok() const {
        return exit_code == 0 && !cancelled && !timed_out &&
            launch_error.empty();
    }
};

using ProcessOutputHandler = std::function<void(const ProcessOutput&)>;

// Runs without a shell. Cancellation terminates the complete child process
// group so agent-spawned commands do not remain in the background.
[[nodiscard]] ProcessResult run_process(
    const ProcessRequest& request,
    ProcessOutputHandler on_output = {},
    const std::atomic_bool* cancel_requested = nullptr
);

}  // namespace hexloom::agents
