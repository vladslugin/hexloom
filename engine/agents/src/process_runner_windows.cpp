#include "hexloom/agents/process_runner.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <chrono>
#include <cstddef>
#include <future>
#include <mutex>
#include <string>
#include <utility>
#include <vector>
#include <wctype.h>

namespace hexloom::agents {
namespace {

class Handle {
public:
    explicit Handle(HANDLE value = INVALID_HANDLE_VALUE) : value_(value) {}

    ~Handle() {
        reset();
    }

    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;

    Handle(Handle&& other) noexcept : value_(other.value_) {
        other.value_ = INVALID_HANDLE_VALUE;
    }

    Handle& operator=(Handle&& other) noexcept {
        if (this != &other) {
            reset();
            value_ = other.value_;
            other.value_ = INVALID_HANDLE_VALUE;
        }
        return *this;
    }

    [[nodiscard]] HANDLE get() const {
        return value_;
    }

    [[nodiscard]] bool valid() const {
        return value_ != INVALID_HANDLE_VALUE && value_ != nullptr;
    }

    void reset(HANDLE value = INVALID_HANDLE_VALUE) {
        if (valid()) {
            CloseHandle(value_);
        }
        value_ = value;
    }

    [[nodiscard]] HANDLE* address() {
        return &value_;
    }

private:
    HANDLE value_;
};

[[nodiscard]] std::wstring widen(const std::string& value) {
    if (value.empty()) {
        return {};
    }
    const int length = MultiByteToWideChar(
        CP_UTF8,
        0,
        value.data(),
        static_cast<int>(value.size()),
        nullptr,
        0
    );
    if (length <= 0) {
        return {};
    }
    std::wstring wide(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(
        CP_UTF8,
        0,
        value.data(),
        static_cast<int>(value.size()),
        wide.data(),
        length
    );
    return wide;
}

[[nodiscard]] std::string narrow(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }
    const int length = WideCharToMultiByte(
        CP_UTF8,
        0,
        value.data(),
        static_cast<int>(value.size()),
        nullptr,
        0,
        nullptr,
        nullptr
    );
    if (length <= 0) {
        return {};
    }
    std::string narrowed(static_cast<std::size_t>(length), '\0');
    WideCharToMultiByte(
        CP_UTF8,
        0,
        value.data(),
        static_cast<int>(value.size()),
        narrowed.data(),
        length,
        nullptr,
        nullptr
    );
    return narrowed;
}

[[nodiscard]] std::string describe(DWORD error_code) {
    LPWSTR buffer = nullptr;
    const DWORD length = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error_code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&buffer),
        0,
        nullptr
    );
    if (length == 0 || buffer == nullptr) {
        return "Windows error " + std::to_string(error_code);
    }
    std::wstring message(buffer, length);
    LocalFree(buffer);
    while (!message.empty() &&
           (message.back() == L'\r' || message.back() == L'\n' ||
            message.back() == L' ')) {
        message.pop_back();
    }
    return narrow(message);
}

// Quotes one argument using the parsing rules that the Microsoft C runtime
// applies to a command line. The prompt therefore reaches the provider as a
// single argv entry no matter which characters it contains.
void append_quoted(std::wstring& command_line, const std::wstring& argument) {
    if (!argument.empty() &&
        argument.find_first_of(L" \t\n\v\"") == std::wstring::npos) {
        command_line += argument;
        return;
    }

    command_line.push_back(L'"');
    for (auto character = argument.begin();; ++character) {
        std::size_t backslashes = 0;
        while (character != argument.end() && *character == L'\\') {
            ++character;
            ++backslashes;
        }

        if (character == argument.end()) {
            command_line.append(backslashes * 2, L'\\');
            break;
        }
        if (*character == L'"') {
            command_line.append(backslashes * 2 + 1, L'\\');
        } else {
            command_line.append(backslashes, L'\\');
        }
        command_line.push_back(*character);
    }
    command_line.push_back(L'"');
}

[[nodiscard]] bool has_separator(const std::wstring& value) {
    return value.find_first_of(L"\\/") != std::wstring::npos;
}

[[nodiscard]] std::wstring search_path(const std::wstring& name) {
    wchar_t buffer[MAX_PATH];
    const DWORD length =
        SearchPathW(nullptr, name.c_str(), nullptr, MAX_PATH, buffer, nullptr);
    if (length == 0 || length >= MAX_PATH) {
        return {};
    }
    return std::wstring(buffer, length);
}

// Resolves an executable the way a shell would, but without one: the PATH is
// searched directly and PATHEXT is applied by hand.
[[nodiscard]] std::wstring resolve_executable(const std::wstring& name) {
    const bool has_extension =
        name.find_last_of(L'.') != std::wstring::npos &&
        name.find_last_of(L'.') > name.find_last_of(L"\\/") + 1;

    if (has_extension) {
        if (has_separator(name)) {
            return GetFileAttributesW(name.c_str()) != INVALID_FILE_ATTRIBUTES
                ? name
                : std::wstring();
        }
        return search_path(name);
    }

    std::wstring extensions(L".COM;.EXE;.BAT;.CMD");
    wchar_t path_extension[1024];
    const DWORD length = GetEnvironmentVariableW(
        L"PATHEXT",
        path_extension,
        static_cast<DWORD>(std::size(path_extension))
    );
    if (length > 0 && length < std::size(path_extension)) {
        extensions.assign(path_extension, length);
    }

    std::size_t start = 0;
    while (start <= extensions.size()) {
        const std::size_t separator = extensions.find(L';', start);
        const std::wstring extension = extensions.substr(
            start,
            separator == std::wstring::npos ? std::wstring::npos
                                            : separator - start
        );
        if (!extension.empty()) {
            const std::wstring candidate = name + extension;
            if (has_separator(name)) {
                if (GetFileAttributesW(candidate.c_str()) !=
                    INVALID_FILE_ATTRIBUTES) {
                    return candidate;
                }
            } else {
                auto found = search_path(candidate);
                if (!found.empty()) {
                    return found;
                }
            }
        }
        if (separator == std::wstring::npos) {
            break;
        }
        start = separator + 1;
    }
    return {};
}

[[nodiscard]] std::wstring lowercase_extension(const std::wstring& path) {
    const std::size_t dot = path.find_last_of(L'.');
    if (dot == std::wstring::npos || dot < path.find_last_of(L"\\/") + 1) {
        return {};
    }
    std::wstring extension = path.substr(dot);
    for (auto& character : extension) {
        character = static_cast<wchar_t>(towlower(character));
    }
    return extension;
}

void read_stream(
    HANDLE pipe,
    ProcessStream stream,
    std::string& destination,
    const ProcessOutputHandler& handler,
    std::mutex& handler_mutex
) {
    char buffer[4096];
    DWORD count = 0;
    while (ReadFile(pipe, buffer, sizeof(buffer), &count, nullptr) &&
           count > 0) {
        std::string chunk(buffer, static_cast<std::size_t>(count));
        std::scoped_lock lock(handler_mutex);
        destination += chunk;
        if (handler) {
            handler({stream, std::move(chunk)});
        }
    }
}

[[nodiscard]] bool create_output_pipe(Handle& read_end, Handle& write_end) {
    SECURITY_ATTRIBUTES attributes{};
    attributes.nLength = sizeof(attributes);
    attributes.bInheritHandle = TRUE;

    HANDLE read_handle = INVALID_HANDLE_VALUE;
    HANDLE write_handle = INVALID_HANDLE_VALUE;
    if (!CreatePipe(&read_handle, &write_handle, &attributes, 0)) {
        return false;
    }
    // Only the child may inherit the writing end.
    SetHandleInformation(read_handle, HANDLE_FLAG_INHERIT, 0);
    read_end.reset(read_handle);
    write_end.reset(write_handle);
    return true;
}

}  // namespace

ProcessResult run_process(
    const ProcessRequest& request,
    ProcessOutputHandler on_output,
    const std::atomic_bool* cancel_requested
) {
    ProcessResult result;
    if (request.executable.empty()) {
        result.launch_error = "Executable must not be empty.";
        return result;
    }
    if (!request.working_directory.empty() &&
        !std::filesystem::is_directory(request.working_directory)) {
        result.launch_error = "Working directory does not exist.";
        return result;
    }

    const std::wstring resolved = resolve_executable(widen(request.executable));
    if (resolved.empty()) {
        result.launch_error =
            "Could not find '" + request.executable + "' on PATH.";
        return result;
    }

    // A batch file cannot be started without cmd.exe, which would re-parse the
    // command line and defeat the argv-only guarantee that keeps prompt text
    // from being read as shell syntax.
    const std::wstring extension = lowercase_extension(resolved);
    if (extension == L".bat" || extension == L".cmd") {
        result.launch_error =
            "'" + request.executable +
            "' resolves to a batch script (" + narrow(extension) +
            "), which Hexloom refuses to launch because it would require a "
            "shell to interpret the prompt. Install a native executable, or "
            "point Hexloom at the interpreter directly.";
        return result;
    }

    std::wstring command_line;
    append_quoted(command_line, resolved);
    for (const auto& argument : request.arguments) {
        command_line.push_back(L' ');
        append_quoted(command_line, widen(argument));
    }

    Handle job(CreateJobObjectW(nullptr, nullptr));
    if (!job.valid()) {
        result.launch_error =
            "Could not create a process job: " + describe(GetLastError());
        return result;
    }
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags =
        JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(
            job.get(),
            JobObjectExtendedLimitInformation,
            &limits,
            sizeof(limits)
        )) {
        result.launch_error =
            "Could not configure the process job: " + describe(GetLastError());
        return result;
    }

    Handle stdout_read;
    Handle stdout_write;
    Handle stderr_read;
    Handle stderr_write;
    if (!create_output_pipe(stdout_read, stdout_write) ||
        !create_output_pipe(stderr_read, stderr_write)) {
        result.launch_error =
            "Could not create process pipes: " + describe(GetLastError());
        return result;
    }

    Handle null_input;
    if (!request.inherit_standard_input) {
        SECURITY_ATTRIBUTES attributes{};
        attributes.nLength = sizeof(attributes);
        attributes.bInheritHandle = TRUE;
        null_input.reset(CreateFileW(
            L"NUL",
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            &attributes,
            OPEN_EXISTING,
            0,
            nullptr
        ));
        if (!null_input.valid()) {
            result.launch_error =
                "Could not open the null device: " + describe(GetLastError());
            return result;
        }
    }

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = request.inherit_standard_input
        ? GetStdHandle(STD_INPUT_HANDLE)
        : null_input.get();
    startup.hStdOutput = stdout_write.get();
    startup.hStdError = stderr_write.get();

    const std::wstring directory =
        request.working_directory.empty()
            ? std::wstring()
            : request.working_directory.wstring();

    PROCESS_INFORMATION process{};
    std::vector<wchar_t> mutable_command_line(
        command_line.begin(),
        command_line.end()
    );
    mutable_command_line.push_back(L'\0');

    // Suspended so the job assignment lands before the child can spawn
    // anything of its own.
    const BOOL started = CreateProcessW(
        resolved.c_str(),
        mutable_command_line.data(),
        nullptr,
        nullptr,
        TRUE,
        CREATE_SUSPENDED | CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP,
        nullptr,
        directory.empty() ? nullptr : directory.c_str(),
        &startup,
        &process
    );
    if (!started) {
        result.launch_error =
            "Could not start process: " + describe(GetLastError());
        return result;
    }

    Handle child_process(process.hProcess);
    Handle child_thread(process.hThread);

    if (!AssignProcessToJobObject(job.get(), child_process.get())) {
        const auto error = GetLastError();
        TerminateProcess(child_process.get(), 1);
        ResumeThread(child_thread.get());
        result.launch_error =
            "Could not place the process in its job: " + describe(error);
        return result;
    }
    ResumeThread(child_thread.get());

    // The parent must drop its copies or the readers never observe EOF.
    stdout_write.reset();
    stderr_write.reset();

    std::mutex handler_mutex;
    auto stdout_reader = std::async(
        std::launch::async,
        [&]() {
            read_stream(
                stdout_read.get(),
                ProcessStream::standard_output,
                result.standard_output,
                on_output,
                handler_mutex
            );
        }
    );
    auto stderr_reader = std::async(
        std::launch::async,
        [&]() {
            read_stream(
                stderr_read.get(),
                ProcessStream::standard_error,
                result.standard_error,
                on_output,
                handler_mutex
            );
        }
    );

    const auto started_at = std::chrono::steady_clock::now();
    bool terminated = false;
    while (true) {
        if (WaitForSingleObject(child_process.get(), 20) == WAIT_OBJECT_0) {
            break;
        }

        const auto now = std::chrono::steady_clock::now();
        const bool cancelled =
            cancel_requested != nullptr && cancel_requested->load();
        const bool timed_out = request.timeout.count() > 0 &&
            now - started_at >= request.timeout;
        if (!terminated && (cancelled || timed_out)) {
            result.cancelled = cancelled;
            result.timed_out = timed_out;
            // Kills the child and everything it started.
            TerminateJobObject(job.get(), 1);
            terminated = true;
        }
    }

    // A process the child started can outlive it while still holding the pipe.
    // Give the readers a moment to drain, then close the job so they always
    // reach end-of-file instead of blocking forever.
    const auto grace = std::chrono::seconds(2);
    if (stdout_reader.wait_for(grace) != std::future_status::ready ||
        stderr_reader.wait_for(grace) != std::future_status::ready) {
        TerminateJobObject(job.get(), 1);
    }
    stdout_reader.wait();
    stderr_reader.wait();

    DWORD exit_code = 0;
    if (GetExitCodeProcess(child_process.get(), &exit_code)) {
        result.exit_code = static_cast<int>(exit_code);
    }
    return result;
}

}  // namespace hexloom::agents
