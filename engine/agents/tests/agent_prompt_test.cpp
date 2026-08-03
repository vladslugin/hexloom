#include "hexloom/agents/agent_prompt.hpp"

#include <iostream>
#include <string>

namespace {

[[nodiscard]] hexloom::ProjectMemory sample_memory() {
    hexloom::ProjectMemory memory;
    memory.entries.push_back({
        .id = "cold_stone_palette",
        .category = hexloom::MemoryCategory::visual_style,
        .text = "Stone reads cold and desaturated.",
        .locked = true,
    });
    memory.entries.push_back({
        .id = "shrine_traversable",
        .category = hexloom::MemoryCategory::mechanics,
        .text = "Every shrine stays traversable.",
        .locked = false,
    });
    return memory;
}

}  // namespace

int run_agent_prompt_tests() {
    using hexloom::agents::AgentAccess;
    using hexloom::agents::compile_agent_prompt;

    int failures = 0;
    const auto check = [&failures](bool condition, const char* message) {
        if (!condition) {
            std::cerr << "FAIL: " << message << '\n';
            ++failures;
        }
    };

    const auto memory = sample_memory();
    const auto planning = compile_agent_prompt(
        memory,
        AgentAccess::read_only,
        "Give the shrine a colder silhouette"
    );
    check(
        planning.find("Stone reads cold and desaturated.") != std::string::npos,
        "remembered decisions should reach the agent"
    );
    check(
        planning.find("Every shrine stays traversable.") != std::string::npos,
        "every category should reach the agent"
    );
    check(
        planning.find("Give the shrine a colder silhouette") !=
            std::string::npos,
        "the direction should reach the agent"
    );
    check(
        planning.find("Do not edit files") != std::string::npos,
        "read-only access should forbid edits"
    );
    check(
        planning.find("Implement this in the project") == std::string::npos,
        "read-only access should not ask for an implementation"
    );

    // A locked entry only means something if the agent is told what locked
    // costs, so the marker and its explanation travel together.
    check(
        planning.find("Stone reads cold and desaturated. [locked]") !=
            std::string::npos,
        "a locked entry should be marked"
    );
    check(
        planning.find("Every shrine stays traversable. [locked]") ==
            std::string::npos,
        "an unlocked entry should not be marked"
    );
    check(
        planning.find("stop and explain the conflict") != std::string::npos,
        "the agent should be told what a locked entry costs"
    );

    const auto writing = compile_agent_prompt(
        memory,
        AgentAccess::workspace_write,
        "Add a respawning collectible"
    );
    check(
        writing.find("Implement this in the project") != std::string::npos,
        "write access should ask for an implementation"
    );
    check(
        writing.find("Do not edit files") == std::string::npos,
        "write access should not forbid edits"
    );
    check(
        writing.find("Stone reads cold and desaturated.") != std::string::npos,
        "memory should reach a writing agent too"
    );

    const auto without_memory = compile_agent_prompt(
        {},
        AgentAccess::read_only,
        "Start a new world"
    );
    check(
        without_memory.find("Project memory") == std::string::npos,
        "an empty memory should not add an empty section"
    );
    check(
        without_memory.find("Start a new world") != std::string::npos,
        "the direction should survive an empty memory"
    );

    return failures;
}
