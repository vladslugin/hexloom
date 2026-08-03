#include "hexloom/core/project_memory.hpp"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cctype>
#include <set>
#include <string>

namespace hexloom {
namespace {

constexpr std::size_t maximum_text_length = 400;
constexpr std::size_t maximum_entries = 500;

void add_issue(
    std::vector<ValidationIssue>& issues,
    std::string field,
    std::string message
) {
    issues.push_back({std::move(field), std::move(message)});
}

[[nodiscard]] bool is_portable_identifier(std::string_view value) {
    if (value.empty() || value.size() > 64) {
        return false;
    }
    return std::ranges::all_of(value, [](unsigned char character) {
        return std::islower(character) || std::isdigit(character) ||
            character == '_' || character == '-';
    });
}

}  // namespace

std::size_t ProjectMemory::count(MemoryCategory category) const {
    return static_cast<std::size_t>(std::ranges::count_if(
        entries,
        [category](const MemoryEntry& entry) {
            return entry.category == category;
        }
    ));
}

std::string_view to_string(MemoryCategory category) {
    switch (category) {
        case MemoryCategory::visual_style:
            return "visual_style";
        case MemoryCategory::mechanics:
            return "mechanics";
        case MemoryCategory::constraint:
            return "constraint";
        case MemoryCategory::decision:
            return "decision";
    }
    return "unknown";
}

std::optional<MemoryCategory> parse_memory_category(std::string_view value) {
    if (value == "visual_style") {
        return MemoryCategory::visual_style;
    }
    if (value == "mechanics") {
        return MemoryCategory::mechanics;
    }
    if (value == "constraint") {
        return MemoryCategory::constraint;
    }
    if (value == "decision") {
        return MemoryCategory::decision;
    }
    return std::nullopt;
}

std::vector<ValidationIssue> validate(const ProjectMemory& memory) {
    std::vector<ValidationIssue> issues;

    if (memory.entries.size() > maximum_entries) {
        add_issue(
            issues,
            "memory",
            "A project may remember at most " +
                std::to_string(maximum_entries) + " entries."
        );
    }

    std::set<std::string> seen;
    for (std::size_t index = 0; index < memory.entries.size(); ++index) {
        const auto& entry = memory.entries[index];
        const std::string field = "memory[" + std::to_string(index) + "]";

        if (!is_portable_identifier(entry.id)) {
            add_issue(
                issues,
                field + ".id",
                "Memory id may contain only lowercase letters, digits, "
                "underscores, and hyphens."
            );
        } else if (!seen.insert(entry.id).second) {
            add_issue(
                issues,
                field + ".id",
                "Memory id '" + entry.id + "' is used more than once."
            );
        }

        if (entry.text.empty()) {
            add_issue(issues, field + ".text", "Memory text must not be empty.");
        } else if (entry.text.size() > maximum_text_length) {
            add_issue(
                issues,
                field + ".text",
                "Memory text must stay under " +
                    std::to_string(maximum_text_length) + " characters so it "
                    "remains usable as shared context."
            );
        }
    }

    return issues;
}

namespace {

[[nodiscard]] MemoryLoadResult parse_root(const YAML::Node& root) {
    MemoryLoadResult result;
    ProjectMemory memory;

    if (!root.IsMap()) {
        add_issue(result.issues, "$", "Memory root must be a map.");
        return result;
    }

    const YAML::Node entries = root["memory"];
    if (!entries || !entries.IsSequence()) {
        add_issue(
            result.issues,
            "memory",
            "Required field must be a sequence."
        );
        return result;
    }

    for (std::size_t index = 0; index < entries.size(); ++index) {
        const std::string field = "memory[" + std::to_string(index) + "]";
        const YAML::Node node = entries[index];
        if (!node.IsMap()) {
            add_issue(result.issues, field, "Memory entry must be a map.");
            continue;
        }

        MemoryEntry entry;
        try {
            entry.id = node["id"] ? node["id"].as<std::string>() : "";
            entry.text = node["text"] ? node["text"].as<std::string>() : "";
            if (node["locked"]) {
                entry.locked = node["locked"].as<bool>();
            }
        } catch (const YAML::Exception&) {
            add_issue(result.issues, field, "Memory entry has an invalid type.");
            continue;
        }

        const YAML::Node category = node["category"];
        if (!category || !category.IsScalar()) {
            add_issue(
                result.issues,
                field + ".category",
                "Required field is missing."
            );
            continue;
        }
        const auto parsed = parse_memory_category(category.as<std::string>());
        if (!parsed.has_value()) {
            add_issue(
                result.issues,
                field + ".category",
                "Unknown category '" + category.as<std::string>() +
                    "'. Use visual_style, mechanics, constraint, or decision."
            );
            continue;
        }
        entry.category = *parsed;
        memory.entries.push_back(std::move(entry));
    }

    for (const auto& issue : validate(memory)) {
        result.issues.push_back(issue);
    }
    result.memory = std::move(memory);
    return result;
}

}  // namespace

MemoryLoadResult load_project_memory(
    const std::filesystem::path& memory_path
) {
    try {
        return parse_root(YAML::LoadFile(memory_path.string()));
    } catch (const YAML::BadFile&) {
        MemoryLoadResult result;
        add_issue(
            result.issues,
            "$file",
            "Could not open project memory: " + memory_path.string()
        );
        return result;
    } catch (const YAML::ParserException& error) {
        MemoryLoadResult result;
        add_issue(
            result.issues,
            "$yaml",
            "Invalid YAML near line " + std::to_string(error.mark.line + 1) +
                ", column " + std::to_string(error.mark.column + 1) + "."
        );
        return result;
    } catch (const YAML::Exception& error) {
        MemoryLoadResult result;
        add_issue(
            result.issues,
            "$yaml",
            "Could not read project memory: " + std::string(error.what())
        );
        return result;
    }
}

}  // namespace hexloom
