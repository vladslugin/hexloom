#pragma once

#include "hexloom/core/art_style.hpp"
#include "hexloom/core/material_request.hpp"

#include <string>

namespace hexloom::generation {

struct TexturePrompt {
    std::string positive;
    std::string negative;
};

[[nodiscard]] TexturePrompt compile_texture_prompt(
    const MaterialRequest& request,
    const ArtStyleProfile& style
);

}  // namespace hexloom::generation
