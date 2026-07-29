#include "hexloom/core/material_spec_loader.hpp"

#include <filesystem>
#include <iostream>
#include <string_view>

int material_request_tests();

namespace {

[[nodiscard]] bool has_issue(
    const hexloom::MaterialLoadResult& result,
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

int main() {
    int failures = material_request_tests();
    const auto check = [&failures](bool condition, const char* message) {
        if (!condition) {
            std::cerr << "FAIL: " << message << '\n';
            ++failures;
        }
    };

    const std::filesystem::path fixtures = HEXLOOM_TEST_FIXTURE_DIR;

    const auto valid =
        hexloom::load_material_request(fixtures / "valid_game.yaml");
    check(valid.ok(), "valid YAML specification should load");
    check(valid.request.has_value(), "valid result should contain a request");
    check(valid.style.has_value(), "valid result should contain an art style");
    if (valid.request.has_value()) {
        check(
            valid.request->id == "ancient_stone_floor",
            "loader should preserve material id"
        );
        check(
            valid.request->style_id == "soft_neon_scifi",
            "loader should resolve art style id"
        );
        check(
            valid.request->maps.size() == 4,
            "loader should parse all requested maps"
        );
    }
    if (valid.style.has_value()) {
        check(
            valid.style->geometry == "low_poly",
            "loader should parse geometry style"
        );
        check(
            valid.style->accent_color ==
                hexloom::RgbColor{.red = 73, .green = 214, .blue = 255},
            "loader should parse hexadecimal palette colors"
        );
    }

    const auto invalid =
        hexloom::load_material_request(fixtures / "invalid_material.yaml");
    check(!invalid.ok(), "invalid material specification should fail");
    check(
        has_issue(invalid, "material.resolution"),
        "invalid resolution should be diagnosed"
    );
    check(
        has_issue(invalid, "material.maps[1]"),
        "unknown texture map should be diagnosed"
    );
    check(
        has_issue(invalid, "material.maps"),
        "missing albedo map should be diagnosed"
    );
    check(
        has_issue(invalid, "art_style.geometry"),
        "unsupported geometry should be diagnosed"
    );
    check(
        has_issue(invalid, "art_style.realism"),
        "out-of-range realism should be diagnosed"
    );
    check(
        has_issue(invalid, "art_style.palette.base"),
        "invalid palette color should be diagnosed"
    );

    const auto malformed =
        hexloom::load_material_request(fixtures / "malformed.yaml");
    check(!malformed.ok(), "malformed YAML should fail");
    check(
        has_issue(malformed, "$yaml"),
        "malformed YAML should report parser location"
    );

    const auto missing =
        hexloom::load_material_request(fixtures / "does_not_exist.yaml");
    check(!missing.ok(), "missing file should fail");
    check(
        has_issue(missing, "$file"),
        "missing file should report a file issue"
    );

    if (failures == 0) {
        std::cout << "Hexloom core tests passed.\n";
    }
    return failures == 0 ? 0 : 1;
}
