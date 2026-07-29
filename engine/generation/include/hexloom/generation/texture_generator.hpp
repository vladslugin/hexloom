#pragma once

#include "hexloom/core/material_request.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace hexloom::generation {

struct TextureGenerationJob {
    MaterialRequest request;
    std::filesystem::path output_directory;
    std::uint64_t seed = 0;
};

struct GeneratedTextureMap {
    TextureMap type;
    std::filesystem::path path;
    std::uint32_t width;
    std::uint32_t height;
    std::uint64_t checksum;
};

struct TextureArtifact {
    std::string provider;
    std::string material_id;
    std::uint64_t seed;
    std::filesystem::path manifest_path;
    std::vector<GeneratedTextureMap> maps;
};

struct TextureGenerationIssue {
    std::string code;
    std::string message;
};

struct TextureGenerationResult {
    std::optional<TextureArtifact> artifact;
    std::vector<TextureGenerationIssue> issues;

    [[nodiscard]] bool ok() const {
        return artifact.has_value() && issues.empty();
    }
};

class TextureGenerator {
public:
    virtual ~TextureGenerator() = default;

    [[nodiscard]] virtual std::string provider_id() const = 0;
    [[nodiscard]] virtual TextureGenerationResult generate(
        const TextureGenerationJob& job
    ) const = 0;
};

class DeterministicTextureGenerator final : public TextureGenerator {
public:
    [[nodiscard]] std::string provider_id() const override;
    [[nodiscard]] TextureGenerationResult generate(
        const TextureGenerationJob& job
    ) const override;
};

}  // namespace hexloom::generation
