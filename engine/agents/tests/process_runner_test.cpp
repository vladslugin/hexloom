#include "hexloom/agents/process_runner.hpp"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>

int run_process_child(int argc, char** argv) {
    const std::string_view mode(argv[1]);
    if (mode == "--process-child-print-args") {
        for (int index = 2; index < argc; ++index) {
            std::cout << "ARG[" << argv[index] << "]\n";
        }
        std::cout << std::flush;
        return 0;
    }
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
    // Provider CLIs installed through npm are .cmd shims, so a batch file has
    // to work -- and a prompt sent through one must survive cmd.exe as data.
    // The shim forwards %* the way an npm shim forwards to node, so this
    // exercises argv quoting, cmd escaping, and the child's own parsing.
    const auto batch_path =
        std::filesystem::temp_directory_path() / "hexloom-shim.cmd";
    {
        std::ofstream batch(batch_path);
        batch << "@echo off\r\n"
              << "\"" << executable << "\" --process-child-print-args %*\r\n";
    }

    const std::string injection = "a\" & echo INJECTED & \"b";
    const auto batch_result = hexloom::agents::run_process({
        .executable = batch_path.string(),
        .arguments = {
            "plain",
            injection,
            "trailing|pipe",
            "100% sure",
            "%PATH% stays literal",
        },
        .working_directory = std::filesystem::current_path(),
        .timeout = 20s,
    });
    const auto& batch_output = batch_result.standard_output;
    if (std::getenv("HEXLOOM_DEBUG_BATCH") != nullptr) {
        std::cerr << "----- batch stdout -----\n"
                  << batch_output << "----- batch stderr -----\n"
                  << batch_result.standard_error << "------------------------\n";
    }
    check(
        batch_result.launch_error.empty(),
        "a batch shim should launch through the command processor"
    );
    check(
        batch_output.find("ARG[plain]") != std::string::npos,
        "a plain argument should survive a batch shim"
    );
    check(
        batch_output.find("ARG[" + injection + "]") != std::string::npos,
        "quotes and ampersands should reach the child as literal text"
    );
    check(
        batch_output.find("ARG[trailing|pipe]") != std::string::npos,
        "a pipe should reach the child as literal text"
    );
    check(
        batch_output.find("ARG[100% sure]") != std::string::npos,
        "a percent sign should reach the child as literal text"
    );
    // An expanded %PATH% would splice environment content -- and any
    // metacharacter inside it -- into the command line unescaped.
    check(
        batch_output.find("ARG[%PATH% stays literal]") != std::string::npos,
        "an environment reference must not be expanded"
    );
    // If cmd had parsed the payload, 'echo INJECTED' would have run as its own
    // command and printed a line of its own.
    check(
        batch_output.find("\nINJECTED") == std::string::npos &&
            batch_output.rfind("INJECTED", 0) != 0,
        "an injected command must not execute"
    );
    std::error_code remove_error;
    std::filesystem::remove(batch_path, remove_error);
#endif

    return failures;
}
