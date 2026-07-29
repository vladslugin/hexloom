#include "hexloom/core/material_spec_loader.hpp"

#include <filesystem>
#include <iostream>
#include <string_view>

namespace {

void print_usage() {
    std::cerr << "Usage: hexloom validate <game.yaml>\n";
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 3 || std::string_view(argv[1]) != "validate") {
        print_usage();
        return 2;
    }

    const std::filesystem::path specification_path = argv[2];
    const auto result = hexloom::load_material_request(specification_path);

    if (!result.ok()) {
        std::cerr << "Hexloom specification is invalid: "
                  << specification_path << '\n';
        for (const auto& issue : result.issues) {
            std::cerr << "error [" << issue.field << "]: "
                      << issue.message << '\n';
        }
        return 1;
    }

    const auto& request = *result.request;
    std::cout << "Hexloom material request: " << request.id << '\n'
              << "style: " << request.style_id << '\n'
              << "status: ready\n"
              << "maps:";
    for (const auto map : request.maps) {
        std::cout << ' ' << hexloom::to_string(map);
    }
    std::cout << '\n';

    return 0;
}
