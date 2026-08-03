#include "hexloom/agents/process_runner.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
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
    if (mode == "--process-child-check-stdin") {
        return std::cin.get() == std::char_traits<char>::eof() ? 0 : 8;
    }
    return -1;
}

int run_process_runner_tests(const std::string& executable) {
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
    // The runner passes bytes through untouched, so the child's own newline
    // translation is visible here. AgentEventStream strips the carriage
    // return when it splits provider output into lines.
#if defined(_WIN32)
    const std::string line_break = "\r\n";
#else
    const std::string line_break = "\n";
#endif
    check(completed.exit_code == 7, "child exit code should be preserved");
    check(
        completed.standard_output == "event-one" + line_break,
        "stdout should be captured"
    );
    check(
        completed.standard_error == "diagnostic-one" + line_break,
        "stderr should be captured separately"
    );
    check(output_events >= 2, "output should be streamed to the callback");

    const auto closed_input = hexloom::agents::run_process({
        .executable = executable,
        .arguments = {"--process-child-check-stdin"},
        .working_directory = std::filesystem::current_path(),
        .timeout = 2s,
    });
    check(
        closed_input.ok(),
        "one-shot child should receive end-of-file on standard input"
    );

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

    std::atomic_bool cancel_requested{true};
    const auto cancelled = hexloom::agents::run_process(
        {
            .executable = executable,
            .arguments = {"--process-child-wait"},
            .working_directory = std::filesystem::current_path(),
            .timeout = 5s,
        },
        {},
        &cancel_requested
    );
    check(cancelled.cancelled, "a cancelled child should report cancellation");
    check(!cancelled.ok(), "a cancelled child should not succeed");

#if defined(_WIN32)
    // A batch file cannot start without cmd.exe re-parsing the command line,
    // so Hexloom must refuse it rather than let a prompt become shell syntax.
    const auto batch_path =
        std::filesystem::temp_directory_path() / "hexloom-refused.cmd";
    {
        std::ofstream batch(batch_path);
        batch << "@echo off\r\n";
    }
    const auto batch_result = hexloom::agents::run_process({
        .executable = batch_path.string(),
        .arguments = {},
        .working_directory = std::filesystem::current_path(),
        .timeout = 5s,
    });
    check(
        batch_result.launch_error.find("batch script") != std::string::npos,
        "a batch script should be refused with an explanatory error"
    );
    std::error_code remove_error;
    std::filesystem::remove(batch_path, remove_error);
#endif

    return failures;
}
