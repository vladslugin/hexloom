#include "hexloom/agents/process_runner.hpp"

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace hexloom::agents {
namespace {

class FileDescriptor {
public:
    explicit FileDescriptor(int value = -1) : value_(value) {}
    ~FileDescriptor() {
        if (value_ >= 0) {
            close(value_);
        }
    }

    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;

    FileDescriptor(FileDescriptor&& other) noexcept
        : value_(other.value_) {
        other.value_ = -1;
    }

    FileDescriptor& operator=(FileDescriptor&& other) noexcept {
        if (this != &other) {
            reset();
            value_ = other.value_;
            other.value_ = -1;
        }
        return *this;
    }

    [[nodiscard]] int get() const {
        return value_;
    }

    [[nodiscard]] int release() {
        const int value = value_;
        value_ = -1;
        return value;
    }

    void reset() {
        if (value_ >= 0) {
            close(value_);
            value_ = -1;
        }
    }

private:
    int value_;
};

[[nodiscard]] bool create_pipe(
    FileDescriptor& read_end,
    FileDescriptor& write_end
) {
    int descriptors[2];
    if (pipe(descriptors) != 0) {
        return false;
    }
    read_end = FileDescriptor(descriptors[0]);
    write_end = FileDescriptor(descriptors[1]);
    return true;
}

void make_non_blocking(int descriptor) {
    const int flags = fcntl(descriptor, F_GETFL);
    if (flags >= 0) {
        static_cast<void>(fcntl(descriptor, F_SETFL, flags | O_NONBLOCK));
    }
}

void append_output(
    int descriptor,
    ProcessStream stream,
    std::string& destination,
    const ProcessOutputHandler& handler,
    bool& open
) {
    char buffer[4096];
    while (true) {
        const ssize_t count = read(descriptor, buffer, sizeof(buffer));
        if (count > 0) {
            std::string chunk(buffer, static_cast<std::size_t>(count));
            destination += chunk;
            if (handler) {
                handler({stream, std::move(chunk)});
            }
            continue;
        }
        if (count == 0) {
            open = false;
        } else if (errno != EAGAIN && errno != EWOULDBLOCK &&
                   errno != EINTR) {
            open = false;
        }
        return;
    }
}

void report_child_error(int descriptor, int error_number) {
    const auto* bytes = reinterpret_cast<const char*>(&error_number);
    std::size_t written = 0;
    while (written < sizeof(error_number)) {
        const ssize_t count = write(
            descriptor,
            bytes + written,
            sizeof(error_number) - written
        );
        if (count > 0) {
            written += static_cast<std::size_t>(count);
        } else if (errno != EINTR) {
            break;
        }
    }
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

    std::vector<char*> arguments;
    arguments.reserve(request.arguments.size() + 2);
    arguments.push_back(const_cast<char*>(request.executable.c_str()));
    for (const auto& argument : request.arguments) {
        arguments.push_back(const_cast<char*>(argument.c_str()));
    }
    arguments.push_back(nullptr);

    FileDescriptor stdout_read;
    FileDescriptor stdout_write;
    FileDescriptor stderr_read;
    FileDescriptor stderr_write;
    FileDescriptor error_read;
    FileDescriptor error_write;
    if (!create_pipe(stdout_read, stdout_write) ||
        !create_pipe(stderr_read, stderr_write) ||
        !create_pipe(error_read, error_write)) {
        result.launch_error =
            "Could not create process pipes: " + std::string(strerror(errno));
        return result;
    }

    const int descriptor_flags = fcntl(error_write.get(), F_GETFD);
    if (descriptor_flags < 0 ||
        fcntl(
            error_write.get(),
            F_SETFD,
            descriptor_flags | FD_CLOEXEC
        ) < 0) {
        result.launch_error = "Could not configure the process error pipe.";
        return result;
    }

    const pid_t process_id = fork();
    if (process_id < 0) {
        result.launch_error =
            "Could not start process: " + std::string(strerror(errno));
        return result;
    }

    if (process_id == 0) {
        stdout_read.reset();
        stderr_read.reset();
        error_read.reset();
        static_cast<void>(setpgid(0, 0));

        if (dup2(stdout_write.get(), STDOUT_FILENO) < 0 ||
            dup2(stderr_write.get(), STDERR_FILENO) < 0) {
            report_child_error(error_write.get(), errno);
            _exit(127);
        }
        stdout_write.reset();
        stderr_write.reset();

        if (!request.working_directory.empty() &&
            chdir(request.working_directory.c_str()) != 0) {
            report_child_error(error_write.get(), errno);
            _exit(127);
        }

        execvp(request.executable.c_str(), arguments.data());
        report_child_error(error_write.get(), errno);
        _exit(127);
    }

    stdout_write.reset();
    stderr_write.reset();
    error_write.reset();
    static_cast<void>(setpgid(process_id, process_id));
    make_non_blocking(stdout_read.get());
    make_non_blocking(stderr_read.get());
    make_non_blocking(error_read.get());

    const auto started_at = std::chrono::steady_clock::now();
    bool stdout_open = true;
    bool stderr_open = true;
    bool error_open = true;
    bool process_exited = false;
    bool termination_sent = false;
    auto force_kill_at = started_at;
    int status = 0;
    int child_error = 0;
    std::size_t child_error_bytes = 0;

    while (!process_exited || stdout_open || stderr_open || error_open) {
        const auto now = std::chrono::steady_clock::now();
        const bool cancelled =
            cancel_requested != nullptr && cancel_requested->load();
        const bool timed_out =
            request.timeout.count() > 0 &&
            now - started_at >= request.timeout;

        if (!termination_sent && !process_exited &&
            (cancelled || timed_out)) {
            result.cancelled = cancelled;
            result.timed_out = timed_out;
            static_cast<void>(kill(-process_id, SIGTERM));
            termination_sent = true;
            force_kill_at = now + std::chrono::milliseconds(250);
        } else if (termination_sent && !process_exited &&
                   now >= force_kill_at) {
            static_cast<void>(kill(-process_id, SIGKILL));
        }

        pollfd descriptors[3]{
            {
                stdout_read.get(),
                static_cast<short>(stdout_open ? POLLIN : 0),
                0,
            },
            {
                stderr_read.get(),
                static_cast<short>(stderr_open ? POLLIN : 0),
                0,
            },
            {
                error_read.get(),
                static_cast<short>(error_open ? POLLIN : 0),
                0,
            },
        };
        static_cast<void>(poll(descriptors, 3, 20));

        if (stdout_open &&
            (descriptors[0].revents & (POLLIN | POLLHUP | POLLERR))) {
            append_output(
                stdout_read.get(),
                ProcessStream::standard_output,
                result.standard_output,
                on_output,
                stdout_open
            );
        }
        if (stderr_open &&
            (descriptors[1].revents & (POLLIN | POLLHUP | POLLERR))) {
            append_output(
                stderr_read.get(),
                ProcessStream::standard_error,
                result.standard_error,
                on_output,
                stderr_open
            );
        }
        if (error_open &&
            (descriptors[2].revents & (POLLIN | POLLHUP | POLLERR))) {
            const ssize_t count = read(
                error_read.get(),
                reinterpret_cast<char*>(&child_error) + child_error_bytes,
                sizeof(child_error) - child_error_bytes
            );
            if (count > 0) {
                child_error_bytes += static_cast<std::size_t>(count);
            } else if (count == 0 ||
                       (errno != EAGAIN && errno != EWOULDBLOCK &&
                        errno != EINTR)) {
                error_open = false;
            }
        }

        if (!process_exited) {
            const pid_t wait_result = waitpid(process_id, &status, WNOHANG);
            process_exited = wait_result == process_id;
        }
    }

    if (child_error_bytes == sizeof(child_error)) {
        result.launch_error =
            "Could not launch process: " + std::string(strerror(child_error));
    }
    if (WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result.exit_code = 128 + WTERMSIG(status);
    }
    return result;
}

}  // namespace hexloom::agents
