#include "hexloom/agents/process_runner.hpp"

namespace hexloom::agents {

ProcessResult run_process(
    const ProcessRequest&,
    ProcessOutputHandler,
    const std::atomic_bool*
) {
    ProcessResult result;
    result.launch_error =
        "The Windows process backend is not implemented yet.";
    return result;
}

}  // namespace hexloom::agents
