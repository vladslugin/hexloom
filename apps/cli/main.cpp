#include "hexloom/core/material_spec_loader.hpp"
#include "hexloom/generation/texture_generator.hpp"

#include <charconv>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string_view>

namespace {

void print_usage() {
    std::cerr
        << "Usage:\n"
        << "  hexloom validate <game.yaml>\n"
        << "  hexloom generate-textures <game.yaml> <output-dir> [seed]\n";
}

void print_specification_issues(
    const std::filesystem::path& path,
    const hexloom::MaterialLoadResult& result
) {
    std::cerr << "Hexloom specification is invalid: " << path << '\n';
    for (const auto& issue : result.issues) {
        std::cerr << "error [" << issue.field << "]: "
                  << issue.message << '\n';
    }
}

[[nodiscard]] bool parse_seed(
    std::string_view value,
    std::uint64_t& seed
) {
    const auto* begin = value.data();
    const auto* end = value.data() + value.size();
    const auto parsed = std::from_chars(begin, end, seed);
    return parsed.ec == std::errc{} && parsed.ptr == end;
}

int validate_command(const std::filesystem::path& specification_path) {
    const auto result = hexloom::load_material_request(specification_path);
    if (!result.ok()) {
        print_specification_issues(specification_path, result);
        return 1;
    }

    const auto& request = *result.request;
    std::cout << "Hexloom material request: " << request.id << '\n'
              << "style: " << request.style_id << '\n'
              << "geometry: " << result.style->geometry << '\n'
              << "status: ready\n"
              << "maps:";
    for (const auto map : request.maps) {
        std::cout << ' ' << hexloom::to_string(map);
    }
    std::cout << '\n';
    return 0;
}

int generate_command(
    const std::filesystem::path& specification_path,
    const std::filesystem::path& output_directory,
    std::uint64_t seed
) {
    const auto loaded = hexloom::load_material_request(specification_path);
    if (!loaded.ok()) {
        print_specification_issues(specification_path, loaded);
        return 1;
    }

    const hexloom::generation::TextureGenerationJob job{
        .request = *loaded.request,
        .output_directory = output_directory,
        .seed = seed,
    };
    const hexloom::generation::DeterministicTextureGenerator generator;
    const auto generated = generator.generate(job);
    if (!generated.ok()) {
        std::cerr << "Texture generation failed:\n";
        for (const auto& issue : generated.issues) {
            std::cerr << "error [" << issue.code << "]: "
                      << issue.message << '\n';
        }
        return 1;
    }

    const auto& artifact = *generated.artifact;
    std::cout << "Texture artifact ready\n"
              << "provider: " << artifact.provider << '\n'
              << "material: " << artifact.material_id << '\n'
              << "seed: " << artifact.seed << '\n'
              << "manifest: " << artifact.manifest_path << '\n'
              << "maps: " << artifact.maps.size() << '\n';
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 3 && std::string_view(argv[1]) == "validate") {
        return validate_command(argv[2]);
    }

    if ((argc == 4 || argc == 5) &&
        std::string_view(argv[1]) == "generate-textures") {
        std::uint64_t seed = 0;
        if (argc == 5 && !parse_seed(argv[4], seed)) {
            std::cerr << "Seed must be an unsigned 64-bit integer.\n";
            return 2;
        }
        return generate_command(argv[2], argv[3], seed);
    }

    print_usage();
    return 2;
}
