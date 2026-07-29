#include "hexloom/generation/texture_generator.hpp"
#include "hexloom/generation/texture_prompt.hpp"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace hexloom::generation {
namespace {

constexpr std::uint64_t fnv_offset_basis = 14695981039346656037ULL;
constexpr std::uint64_t fnv_prime = 1099511628211ULL;
constexpr double tau = 6.28318530717958647692;

class StagingDirectory {
public:
    explicit StagingDirectory(std::filesystem::path path)
        : path_(std::move(path)) {}

    ~StagingDirectory() {
        if (!committed_) {
            std::error_code error;
            std::filesystem::remove_all(path_, error);
        }
    }

    void commit() {
        committed_ = true;
    }

private:
    std::filesystem::path path_;
    bool committed_ = false;
};

void add_issue(
    TextureGenerationResult& result,
    std::string code,
    std::string message
) {
    result.issues.push_back({std::move(code), std::move(message)});
}

[[nodiscard]] std::uint8_t clamp_byte(double value) {
    return static_cast<std::uint8_t>(
        std::clamp(value, 0.0, 255.0)
    );
}

[[nodiscard]] double periodic_pattern(
    std::uint32_t x,
    std::uint32_t y,
    std::uint32_t size,
    std::uint64_t seed
) {
    const double phase =
        static_cast<double>(seed % 4096ULL) / 4096.0 * tau;
    const double normalized_x =
        static_cast<double>(x) / static_cast<double>(size);
    const double normalized_y =
        static_cast<double>(y) / static_cast<double>(size);

    return
        std::sin(tau * normalized_x * 4.0 + phase) * 0.46 +
        std::cos(tau * normalized_y * 6.0 - phase) * 0.34 +
        std::sin(
            tau * (normalized_x + normalized_y) * 3.0 + phase * 0.5
        ) * 0.20;
}

[[nodiscard]] std::array<std::uint8_t, 4> make_pixel(
    TextureMap map,
    const ArtStyleProfile& style,
    std::uint32_t x,
    std::uint32_t y,
    std::uint32_t size,
    std::uint64_t seed
) {
    const double pattern = periodic_pattern(x, y, size, seed);

    switch (map) {
        case TextureMap::albedo: {
            const double blend = std::clamp(
                (pattern + 1.0) * 0.5,
                0.0,
                1.0
            );
            const auto channel = [blend](
                std::uint8_t base,
                std::uint8_t secondary
            ) {
                return clamp_byte(
                    static_cast<double>(secondary) * (1.0 - blend) +
                    static_cast<double>(base) * blend
                );
            };
            return {
                channel(style.base_color.red, style.secondary_color.red),
                channel(style.base_color.green, style.secondary_color.green),
                channel(style.base_color.blue, style.secondary_color.blue),
                255,
            };
        }
        case TextureMap::normal: {
            const double next_x =
                periodic_pattern((x + 1) % size, y, size, seed);
            const double next_y =
                periodic_pattern(x, (y + 1) % size, size, seed);
            return {
                clamp_byte(128.0 + (pattern - next_x) * 180.0),
                clamp_byte(128.0 + (pattern - next_y) * 180.0),
                255,
                255,
            };
        }
        case TextureMap::roughness: {
            const auto value = clamp_byte(188.0 + pattern * 28.0);
            return {value, value, value, 255};
        }
        case TextureMap::metallic: {
            const auto value = clamp_byte(24.0 + pattern * 8.0);
            return {value, value, value, 255};
        }
        case TextureMap::ambient_occlusion: {
            const auto value = clamp_byte(225.0 + pattern * 20.0);
            return {value, value, value, 255};
        }
        case TextureMap::height: {
            const auto value = clamp_byte(128.0 + pattern * 70.0);
            return {value, value, value, 255};
        }
    }

    return {255, 0, 255, 255};
}

[[nodiscard]] std::uint64_t update_checksum(
    std::uint64_t checksum,
    const std::vector<std::uint8_t>& bytes
) {
    for (const auto byte : bytes) {
        checksum ^= byte;
        checksum *= fnv_prime;
    }
    return checksum;
}

[[nodiscard]] std::string checksum_string(std::uint64_t checksum) {
    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::setw(16) << checksum;
    return stream.str();
}

[[nodiscard]] bool write_map(
    const std::filesystem::path& path,
    TextureMap map,
    const ArtStyleProfile& style,
    std::uint32_t size,
    std::uint64_t seed,
    std::uint64_t& checksum
) {
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        return false;
    }

    checksum = fnv_offset_basis;
    std::vector<std::uint8_t> row(
        static_cast<std::size_t>(size) * 4
    );

    for (std::uint32_t y = 0; y < size; ++y) {
        for (std::uint32_t x = 0; x < size; ++x) {
            const auto pixel = make_pixel(
                map,
                style,
                x,
                y,
                size,
                seed
            );
            const auto offset = static_cast<std::size_t>(x) * 4;
            std::copy(pixel.begin(), pixel.end(), row.begin() + offset);
        }

        output.write(
            reinterpret_cast<const char*>(row.data()),
            static_cast<std::streamsize>(row.size())
        );
        if (!output) {
            return false;
        }
        checksum = update_checksum(checksum, row);
    }

    return true;
}

[[nodiscard]] bool write_manifest(
    const std::filesystem::path& path,
    const TextureArtifact& artifact
) {
    YAML::Emitter yaml;
    yaml << YAML::BeginMap;
    yaml << YAML::Key << "schema_version" << YAML::Value << 1;
    yaml << YAML::Key << "provider" << YAML::Value << artifact.provider;
    yaml << YAML::Key << "material_id" << YAML::Value << artifact.material_id;
    yaml << YAML::Key << "style_id" << YAML::Value << artifact.style_id;
    yaml << YAML::Key << "seed" << YAML::Value << artifact.seed;
    yaml << YAML::Key << "prompt" << YAML::Value
         << artifact.prompt_path.filename().string();
    yaml << YAML::Key << "format" << YAML::Value << "rgba8";
    yaml << YAML::Key << "maps" << YAML::Value << YAML::BeginSeq;
    for (const auto& map : artifact.maps) {
        yaml << YAML::BeginMap;
        yaml << YAML::Key << "type" << YAML::Value
             << std::string(to_string(map.type));
        yaml << YAML::Key << "file" << YAML::Value
             << map.path.filename().string();
        yaml << YAML::Key << "width" << YAML::Value << map.width;
        yaml << YAML::Key << "height" << YAML::Value << map.height;
        yaml << YAML::Key << "checksum_fnv1a64" << YAML::Value
             << checksum_string(map.checksum);
        yaml << YAML::EndMap;
    }
    yaml << YAML::EndSeq;
    yaml << YAML::EndMap;

    std::ofstream output(path);
    output << yaml.c_str() << '\n';
    return static_cast<bool>(output);
}

[[nodiscard]] bool write_prompt(
    const std::filesystem::path& path,
    const TexturePrompt& prompt
) {
    YAML::Emitter yaml;
    yaml << YAML::BeginMap;
    yaml << YAML::Key << "schema_version" << YAML::Value << 1;
    yaml << YAML::Key << "positive" << YAML::Value << prompt.positive;
    yaml << YAML::Key << "negative" << YAML::Value << prompt.negative;
    yaml << YAML::EndMap;

    std::ofstream output(path);
    output << yaml.c_str() << '\n';
    return static_cast<bool>(output);
}

}  // namespace

std::string DeterministicTextureGenerator::provider_id() const {
    return "hexloom.deterministic.v1";
}

TextureGenerationResult DeterministicTextureGenerator::generate(
    const TextureGenerationJob& job
) const {
    TextureGenerationResult result;

    const auto request_issues = validate(job.request);
    if (!request_issues.empty()) {
        for (const auto& issue : request_issues) {
            add_issue(
                result,
                "invalid_request." + issue.field,
                issue.message
            );
        }
        return result;
    }

    const auto style_issues = validate(job.style);
    if (!style_issues.empty()) {
        for (const auto& issue : style_issues) {
            add_issue(
                result,
                "invalid_style." + issue.field,
                issue.message
            );
        }
        return result;
    }
    if (job.request.style_id != job.style.id) {
        add_issue(
            result,
            "style_mismatch",
            "Material style id does not match the supplied art profile."
        );
        return result;
    }

    if (job.output_directory.empty() ||
        job.output_directory == job.output_directory.root_path()) {
        add_issue(
            result,
            "unsafe_output",
            "Output directory must be a specific non-root path."
        );
        return result;
    }

    std::error_code error;
    if (std::filesystem::exists(job.output_directory, error)) {
        add_issue(
            result,
            "output_exists",
            "Output directory already exists; refusing to overwrite it."
        );
        return result;
    }
    if (error) {
        add_issue(
            result,
            "output_check_failed",
            "Could not inspect the output directory."
        );
        return result;
    }

    const auto parent = job.output_directory.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, error);
        if (error) {
            add_issue(
                result,
                "output_parent_failed",
                "Could not create the output parent directory."
            );
            return result;
        }
    }

    auto staging_path = job.output_directory;
    staging_path += ".hexloom-staging";
    if (std::filesystem::exists(staging_path, error)) {
        add_issue(
            result,
            "staging_exists",
            "A staging directory already exists; refusing to overwrite it."
        );
        return result;
    }

    std::filesystem::create_directory(staging_path, error);
    if (error) {
        add_issue(
            result,
            "staging_create_failed",
            "Could not create the staging directory."
        );
        return result;
    }
    StagingDirectory staging(staging_path);

    TextureArtifact staging_artifact{
        .provider = provider_id(),
        .material_id = job.request.id,
        .style_id = job.style.id,
        .seed = job.seed,
        .manifest_path = staging_path / "artifact.yaml",
        .prompt_path = staging_path / "prompt.yaml",
        .maps = {},
    };

    const auto prompt = compile_texture_prompt(job.request, job.style);
    if (!write_prompt(staging_artifact.prompt_path, prompt)) {
        add_issue(
            result,
            "prompt_write_failed",
            "Could not write the compiled texture prompt."
        );
        return result;
    }

    for (const auto map : job.request.maps) {
        const auto filename =
            job.request.id + "_" + std::string(to_string(map)) + ".rgba8";
        const auto path = staging_path / filename;
        std::uint64_t checksum = 0;
        if (!write_map(
                path,
                map,
                job.style,
                job.request.resolution,
                job.seed,
                checksum
            )) {
            add_issue(
                result,
                "map_write_failed",
                "Could not write texture map '" +
                    std::string(to_string(map)) + "'."
            );
            return result;
        }

        staging_artifact.maps.push_back({
            .type = map,
            .path = path,
            .width = job.request.resolution,
            .height = job.request.resolution,
            .checksum = checksum,
        });
    }

    if (!write_manifest(staging_artifact.manifest_path, staging_artifact)) {
        add_issue(
            result,
            "manifest_write_failed",
            "Could not write the artifact manifest."
        );
        return result;
    }

    std::filesystem::rename(staging_path, job.output_directory, error);
    if (error) {
        add_issue(
            result,
            "commit_failed",
            "Could not atomically commit generated textures."
        );
        return result;
    }
    staging.commit();

    TextureArtifact artifact = std::move(staging_artifact);
    artifact.manifest_path = job.output_directory / "artifact.yaml";
    artifact.prompt_path = job.output_directory / "prompt.yaml";
    for (auto& map : artifact.maps) {
        map.path = job.output_directory / map.path.filename();
    }
    result.artifact = std::move(artifact);
    return result;
}

}  // namespace hexloom::generation
