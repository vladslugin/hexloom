#include "hexloom/agents/agent_prompt.hpp"

#include <array>
#include <sstream>
#include <string>

namespace hexloom::agents {
namespace {

[[nodiscard]] std::string_view heading(MemoryCategory category) {
    switch (category) {
        case MemoryCategory::visual_style:
            return "Visual style";
        case MemoryCategory::mechanics:
            return "Mechanics";
        case MemoryCategory::constraint:
            return "Constraints";
        case MemoryCategory::decision:
            return "Decisions already made";
    }
    return "Other";
}

void write_category(
    std::ostringstream& output,
    const ProjectMemory& memory,
    MemoryCategory category
) {
    if (memory.count(category) == 0) {
        return;
    }

    output << heading(category) << ":\n";
    for (const auto& entry : memory.entries) {
        if (entry.category != category) {
            continue;
        }
        output << "- " << entry.text;
        if (entry.locked) {
            output << " [locked]";
        }
        output << '\n';
    }
    output << '\n';
}

}  // namespace

std::string compile_agent_prompt(
    const ProjectMemory& memory,
    AgentAccess access,
    std::string_view direction
) {
    const bool writing = access == AgentAccess::workspace_write;
    std::ostringstream output;

    output << (writing
        ? "You are Hexloom's game engineer.\n\n"
        : "You are Hexloom's game design orchestrator.\n\n");

    if (!memory.empty()) {
        output
            << "Project memory. These are durable decisions about this world, "
               "shared by every agent. Treat them as already agreed rather "
               "than as suggestions.\n\n";

        constexpr std::array categories{
            MemoryCategory::visual_style,
            MemoryCategory::mechanics,
            MemoryCategory::constraint,
            MemoryCategory::decision,
        };
        for (const auto category : categories) {
            write_category(output, memory, category);
        }

        output
            << "An entry marked [locked] may not be traded away to satisfy "
               "the request below. If the direction cannot be met without "
               "breaking one, stop and explain the conflict instead.\n\n";
    }

    output << "Direction:\n\n" << direction << "\n\n";

    if (writing) {
        output
            << "Implement this in the project. Change the smallest set of "
               "files that satisfies it. Add or update tests for what you "
               "change. Finish by reporting every file you touched and why.";
    } else {
        output
            << "Do not edit files. Return ordered steps, affected gameplay "
               "systems, artifacts to produce, risks, and validation checks.";
    }

    return output.str();
}

}  // namespace hexloom::agents
