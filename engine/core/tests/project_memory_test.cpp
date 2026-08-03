#include "hexloom/core/project_memory.hpp"

#include <filesystem>
#include <iostream>
#include <string_view>

namespace {

[[nodiscard]] bool has_issue(
    const hexloom::MemoryLoadResult& result,
    std::string_view field
) {
    for (const auto& issue : result.issues) {
        if (issue.field == field) {
            return true;
        }
    }
    return false;
}

}  // namespace

int project_memory_tests() {
    int failures = 0;
    const auto check = [&failures](bool condition, const char* message) {
        if (!condition) {
            std::cerr << "FAIL: " << message << '\n';
            ++failures;
        }
    };

    const std::filesystem::path fixtures = HEXLOOM_TEST_FIXTURE_DIR;

    const auto valid =
        hexloom::load_project_memory(fixtures / "valid_memory.yaml");
    check(valid.ok(), "valid project memory should load");
    if (valid.memory.has_value()) {
        const auto& memory = valid.memory.value();
        check(memory.entries.size() == 5, "loader should read every entry");
        check(
            memory.count(hexloom::MemoryCategory::visual_style) == 2,
            "entries should be counted per category"
        );
        check(
            memory.count(hexloom::MemoryCategory::constraint) == 1,
            "constraints should be counted separately"
        );
        check(
            memory.entries.front().locked,
            "loader should preserve the locked flag"
        );
        check(
            !memory.entries[1].locked,
            "an entry without a locked flag should default to unlocked"
        );
    }

    const auto invalid =
        hexloom::load_project_memory(fixtures / "invalid_memory.yaml");
    check(!invalid.ok(), "invalid project memory should fail");
    check(
        has_issue(invalid, "memory[0].id"),
        "a non-portable identifier should be diagnosed"
    );
    check(
        has_issue(invalid, "memory[2].id"),
        "a duplicated identifier should be diagnosed"
    );
    check(
        has_issue(invalid, "memory[3].category"),
        "an unknown category should be diagnosed"
    );

    const auto missing =
        hexloom::load_project_memory(fixtures / "does_not_exist.yaml");
    check(!missing.ok(), "missing memory file should fail");
    check(
        has_issue(missing, "$file"),
        "missing memory file should report a file issue"
    );

    const auto malformed =
        hexloom::load_project_memory(fixtures / "malformed.yaml");
    check(!malformed.ok(), "malformed YAML should fail");

    // A text long enough to stop being usable shared context must be caught
    // before it reaches an agent.
    hexloom::ProjectMemory oversized;
    oversized.entries.push_back({
        .id = "too_long",
        .category = hexloom::MemoryCategory::decision,
        .text = std::string(401, 'x'),
        .locked = false,
    });
    check(
        !hexloom::validate(oversized).empty(),
        "an overlong memory text should be rejected"
    );

    return failures;
}
