#include "hexloom/generation/texture_generator.hpp"

#include <yaml-cpp/yaml.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>

int run_texture_prompt_tests();

namespace {

class TemporaryDirectory {
public:
    TemporaryDirectory() {
        const auto unique = std::chrono::steady_clock::now()
                                .time_since_epoch()
                                .count();
        path_ = std::filesystem::temp_directory_path() /
            ("hexloom-generation-test-" + std::to_string(unique));
        std::filesystem::create_directory(path_);
    }

    ~TemporaryDirectory() {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }

    [[nodiscard]] const std::filesystem::path& path() const {
        return path_;
    }

private:
    std::filesystem::path path_;
};

[[nodiscard]] hexloom::MaterialRequest test_request() {
    return {
        .id = "test_stone",
        .category = "stone",
        .style_id = "test_style",
        .resolution = 256,
        .physical_size_meters = 2.0F,
        .seamless = true,
        .mobile_optimized = true,
        .maps = {
            hexloom::TextureMap::albedo,
            hexloom::TextureMap::normal,
            hexloom::TextureMap::roughness,
            hexloom::TextureMap::ambient_occlusion,
        },
    };
}

[[nodiscard]] hexloom::ArtStyleProfile test_style() {
    return {
        .id = "test_style",
        .geometry = "low_poly",
        .silhouettes = "exaggerated",
        .edges = "soft_beveled",
        .texture_style = "stylized_pbr",
        .lighting = "atmospheric",
        .realism = 0.25,
        .base_color = {87, 106, 145},
        .secondary_color = {38, 49, 71},
        .accent_color = {73, 214, 255},
        .forbidden = {"photorealism"},
    };
}

}  // namespace

int main() {
    int failures = 0;
    const auto check = [&failures](bool condition, const char* message) {
        if (!condition) {
            std::cerr << "FAIL: " << message << '\n';
            ++failures;
        }
    };

    TemporaryDirectory temporary;
    const hexloom::generation::DeterministicTextureGenerator generator;

    const hexloom::generation::TextureGenerationJob first_job{
        .request = test_request(),
        .style = test_style(),
        .output_directory = temporary.path() / "first",
        .seed = 42,
    };
    const auto first = generator.generate(first_job);
    check(first.ok(), "first deterministic generation should succeed");

    const hexloom::generation::TextureGenerationJob second_job{
        .request = test_request(),
        .style = test_style(),
        .output_directory = temporary.path() / "second",
        .seed = 42,
    };
    const auto second = generator.generate(second_job);
    check(second.ok(), "second deterministic generation should succeed");

    const hexloom::generation::TextureGenerationJob different_seed_job{
        .request = test_request(),
        .style = test_style(),
        .output_directory = temporary.path() / "different-seed",
        .seed = 43,
    };
    const auto different_seed = generator.generate(different_seed_job);
    check(
        different_seed.ok(),
        "generation with a different seed should succeed"
    );

    if (first.artifact.has_value() && second.artifact.has_value()) {
        const auto& first_artifact = *first.artifact;
        const auto& second_artifact = *second.artifact;
        check(
            first_artifact.maps.size() == 4,
            "artifact should contain every requested map"
        );
        check(
            first_artifact.maps.size() == second_artifact.maps.size(),
            "repeated artifacts should have equal map counts"
        );

        const std::uintmax_t expected_size = 256ULL * 256ULL * 4ULL;
        for (std::size_t index = 0;
             index < first_artifact.maps.size();
             ++index) {
            const auto& first_map = first_artifact.maps[index];
            const auto& second_map = second_artifact.maps[index];
            check(
                std::filesystem::file_size(first_map.path) == expected_size,
                "raw RGBA8 map should have exact dimensions"
            );
            check(
                first_map.checksum == second_map.checksum,
                "equal seeds should produce equal checksums"
            );
        }

        check(
            std::filesystem::exists(first_artifact.manifest_path),
            "artifact manifest should exist"
        );
        check(
            std::filesystem::exists(first_artifact.prompt_path),
            "compiled prompt should exist"
        );
        const YAML::Node manifest =
            YAML::LoadFile(first_artifact.manifest_path.string());
        check(
            manifest["provider"].as<std::string>() ==
                "hexloom.deterministic.v1",
            "manifest should record provider id"
        );
        check(
            manifest["style_id"].as<std::string>() == "test_style",
            "manifest should record style id"
        );
        check(
            manifest["maps"].size() == 4,
            "manifest should describe every generated map"
        );

        if (different_seed.artifact.has_value()) {
            check(
                first_artifact.maps.front().checksum !=
                    different_seed.artifact->maps.front().checksum,
                "different seeds should produce different checksums"
            );
        }
    }

    const auto overwrite_attempt = generator.generate(first_job);
    check(
        !overwrite_attempt.ok(),
        "generator should refuse to overwrite an existing artifact"
    );
    if (!overwrite_attempt.issues.empty()) {
        check(
            overwrite_attempt.issues.front().code == "output_exists",
            "overwrite refusal should have a stable issue code"
        );
    }

    auto invalid_request = test_request();
    invalid_request.id = "../unsafe";
    const auto invalid = generator.generate({
        .request = invalid_request,
        .style = test_style(),
        .output_directory = temporary.path() / "unsafe",
        .seed = 0,
    });
    check(!invalid.ok(), "unsafe material id should fail validation");
    check(
        !std::filesystem::exists(temporary.path() / "unsafe"),
        "invalid job should not create output"
    );

    failures += run_texture_prompt_tests();
    if (failures == 0) {
        std::cout << "Hexloom generation tests passed.\n";
    }
    return failures == 0 ? 0 : 1;
}
