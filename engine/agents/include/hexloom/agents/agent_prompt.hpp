#pragma once

#include "hexloom/agents/agent_cli.hpp"
#include "hexloom/core/project_memory.hpp"

#include <string>
#include <string_view>

namespace hexloom::agents {

// Builds the prompt an agent receives for one direction. Project memory is
// prepended so style, mechanics, and constraints do not have to be restated
// every session, and the access mode decides whether the agent is asked to
// plan or to change the project.
//
// This lives in C++ rather than in the Godot layer so the wording is shared by
// every front end and can be tested without a running editor.
[[nodiscard]] std::string compile_agent_prompt(
    const ProjectMemory& memory,
    AgentAccess access,
    std::string_view direction
);

}  // namespace hexloom::agents
