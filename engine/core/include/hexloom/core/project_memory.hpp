#pragma once

#include "hexloom/core/material_request.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hexloom {

// What a remembered decision is about. The category drives how an entry is
// grouped for an agent and for the creator, so it is a closed set rather than
// free text.
enum class MemoryCategory {
    visual_style,
    mechanics,
    constraint,
    decision,
};

struct MemoryEntry {
    std::string id;
    MemoryCategory category = MemoryCategory::decision;
    std::string text;
    // A locked entry may not be traded away to satisfy a new request; an agent
    // must stop and explain instead of quietly breaking it.
    bool locked = false;
};

struct ProjectMemory {
    std::vector<MemoryEntry> entries;

    [[nodiscard]] std::size_t count(MemoryCategory category) const;
    [[nodiscard]] bool empty() const {
        return entries.empty();
    }
};

struct MemoryLoadResult {
    std::optional<ProjectMemory> memory;
    std::vector<ValidationIssue> issues;

    [[nodiscard]] bool ok() const {
        return memory.has_value() && issues.empty();
    }
};

[[nodiscard]] std::vector<ValidationIssue> validate(const ProjectMemory& memory);

[[nodiscard]] MemoryLoadResult load_project_memory(
    const std::filesystem::path& memory_path
);

[[nodiscard]] std::string_view to_string(MemoryCategory category);

[[nodiscard]] std::optional<MemoryCategory> parse_memory_category(
    std::string_view value
);

}  // namespace hexloom
