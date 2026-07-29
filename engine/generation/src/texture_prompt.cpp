#include "hexloom/generation/texture_prompt.hpp"

#include <iomanip>
#include <sstream>
#include <string>

namespace hexloom::generation {
namespace {

[[nodiscard]] std::string color_string(const RgbColor& color) {
    std::ostringstream output;
    output << '#'
           << std::hex << std::uppercase << std::setfill('0')
           << std::setw(2) << static_cast<int>(color.red)
           << std::setw(2) << static_cast<int>(color.green)
           << std::setw(2) << static_cast<int>(color.blue);
    return output.str();
}

[[nodiscard]] std::string readable(std::string value) {
    for (auto& character : value) {
        if (character == '_') {
            character = ' ';
        }
    }
    return value;
}

}  // namespace

TexturePrompt compile_texture_prompt(
    const MaterialRequest& request,
    const ArtStyleProfile& style
) {
    std::ostringstream positive;
    positive << "Create a seamless " << readable(style.texture_style)
             << " texture set for a " << readable(request.category)
             << " material named " << request.id << ". "
             << "Art direction: " << readable(style.geometry)
             << " forms, " << readable(style.silhouettes)
             << " silhouettes, " << readable(style.edges)
             << " edges, and " << readable(style.lighting)
             << " lighting. Realism strength: " << style.realism << ". "
             << "Use this limited palette: base "
             << color_string(style.base_color) << ", secondary "
             << color_string(style.secondary_color) << ", accent "
             << color_string(style.accent_color) << ". "
             << "The texture must tile without visible seams at "
             << request.physical_size_meters << " meters and remain readable "
             << "on mobile screens. Required maps:";
    for (const auto map : request.maps) {
        positive << ' ' << to_string(map);
    }
    positive << ". Preserve broad shapes and avoid accidental lettering.";

    std::ostringstream negative;
    for (std::size_t index = 0; index < style.forbidden.size(); ++index) {
        if (index != 0) {
            negative << ", ";
        }
        negative << readable(style.forbidden[index]);
    }

    return {
        .positive = positive.str(),
        .negative = negative.str(),
    };
}

}  // namespace hexloom::generation
