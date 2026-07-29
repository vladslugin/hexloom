#include "hexloom/agents/process_runner.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>

int run_process_child(std::string_view mode) {
    if (mode == "--process-child-success") {
        std::cout << "event-one\n" << std::flush;
        std::cerr << "diagnostic-one\n" << std::flush;
        return 7;
    }
    if (mode == "--process-child-wait") {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return 0;
    }
    return -1;
}

int run_process_runner_tests(const std::string& executable) {
#if defined(__unix__) || defined(__APPLE__)
    using namespace std::chrono_literals;
    int failures = 0;
    const auto check = [&failures](bool condition, const char* message) {
        if (!condition) {
            std::cerr << "FAIL: " << message << '\n';
            ++failures;
        }
    };

    int output_events = 0;
    const auto completed = hexloom::agents::run_process(
        {
            .executable = executable,
            .arguments = {"--process-child-success"},
            .working_directory = std::filesystem::current_path(),
            .timeout = 2s,
        },
        [&output_events](const hexloom::agents::ProcessOutput&) {
            ++output_events;
        }
    );
    check(completed.exit_code == 7, "child exit code should be preserved");
    check(
        completed.standard_output == "event-one\n",
        "stdout should be captured"
    );
    check(
        completed.standard_error == "diagnostic-one\n",
        "stderr should be captured separately"
    );
    check(output_events >= 2, "output should be streamed to the callback");

    const auto timed_out = hexloom::agents::run_process({
        .executable = executable,
        .arguments = {"--process-child-wait"},
        .working_directory = std::filesystem::current_path(),
        .timeout = 50ms,
    });
    check(timed_out.timed_out, "long-running child should time out");
    check(!timed_out.ok(), "timed-out child should not succeed");

    const auto missing = hexloom::agents::run_process({
        .executable = "hexloom-command-that-does-not-exist",
        .arguments = {},
        .working_directory = std::filesystem::current_path(),
        .timeout = 1s,
    });
    check(!missing.launch_error.empty(), "exec failure should be reported");

    return failures;
#else
    static_cast<void>(executable);
    return 0;
#endif
}
