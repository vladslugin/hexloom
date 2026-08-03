#include "hexloom/agents/agent_cli.hpp"
#include "hexloom/agents/agent_prompt.hpp"
#include "hexloom/agents/process_runner.hpp"
#include "hexloom/core/material_spec_loader.hpp"
#include "hexloom/core/project_memory.hpp"
#include "hexloom/generation/texture_generator.hpp"

#include <charconv>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

void print_usage() {
    std::cerr
        << "Usage:\n"
        << "  hexloom validate <game.yaml>\n"
        << "  hexloom generate-textures <game.yaml> <output-dir> [seed]\n"
        << "  hexloom memory <memory.yaml>\n"
        << "  hexloom compose-prompt <memory.yaml> <read|write> <direction>\n"
        << "  hexloom agent-plan <codex|claude|gemini|antigravity> "
           "<read|write> <prompt>\n"
        << "  hexloom agent-run <codex|claude|gemini|antigravity> "
           "<read|write> "
           "<workspace> <prompt>\n";
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
        .style = *loaded.style,
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

void print_memory_issues(
    const std::filesystem::path& path,
    const hexloom::MemoryLoadResult& result
) {
    std::cerr << "Hexloom project memory is invalid: " << path << '\n';
    for (const auto& issue : result.issues) {
        std::cerr << "error [" << issue.field << "]: " << issue.message << '\n';
    }
}

int memory_command(const std::filesystem::path& memory_path) {
    const auto loaded = hexloom::load_project_memory(memory_path);
    if (!loaded.ok()) {
        print_memory_issues(memory_path, loaded);
        return 1;
    }

    const auto& memory = *loaded.memory;
    std::cout << "Hexloom project memory: " << memory.entries.size()
              << " durable decisions\n";
    for (const auto category : {
             hexloom::MemoryCategory::visual_style,
             hexloom::MemoryCategory::mechanics,
             hexloom::MemoryCategory::constraint,
             hexloom::MemoryCategory::decision,
         }) {
        std::cout << hexloom::to_string(category) << ": "
                  << memory.count(category) << '\n';
    }
    for (const auto& entry : memory.entries) {
        std::cout << "  - [" << hexloom::to_string(entry.category) << "] "
                  << entry.id << (entry.locked ? " (locked)" : "") << '\n';
    }
    return 0;
}

[[nodiscard]] std::optional<hexloom::agents::AgentAccess>
parse_agent_access(std::string_view access_name) {
    if (access_name == "read") {
        return hexloom::agents::AgentAccess::read_only;
    }
    if (access_name == "write") {
        return hexloom::agents::AgentAccess::workspace_write;
    }
    return std::nullopt;
}

int compose_prompt_command(
    const std::filesystem::path& memory_path,
    std::string_view access_name,
    std::string_view direction
) {
    const auto access = parse_agent_access(access_name);
    if (!access.has_value()) {
        std::cerr << "Agent access must be 'read' or 'write'.\n";
        return 2;
    }
    if (direction.empty()) {
        std::cerr << "A direction must not be empty.\n";
        return 2;
    }

    const auto loaded = hexloom::load_project_memory(memory_path);
    if (!loaded.ok()) {
        print_memory_issues(memory_path, loaded);
        return 1;
    }

    std::cout << hexloom::agents::compile_agent_prompt(
        *loaded.memory,
        *access,
        direction
    ) << '\n';
    return 0;
}

int agent_plan_command(
    std::string_view provider_name,
    std::string_view access_name,
    std::string prompt
) {
    const auto provider =
        hexloom::agents::parse_agent_provider(provider_name);
    if (!provider.has_value()) {
        std::cerr
            << "Agent provider must be 'codex', 'claude', 'gemini', "
               "or 'antigravity'.\n";
        return 2;
    }

    const auto access = parse_agent_access(access_name);
    if (!access.has_value()) {
        std::cerr << "Agent access must be 'read' or 'write'.\n";
        return 2;
    }

    try {
        const auto plan = hexloom::agents::make_cli_launch_plan(
            *provider,
            *access,
            std::move(prompt)
        );
        std::cout << hexloom::agents::format_launch_plan(plan);
        return 0;
    } catch (const std::invalid_argument& error) {
        std::cerr << error.what() << '\n';
        return 2;
    }
}

int agent_run_command(
    std::string_view provider_name,
    std::string_view access_name,
    const std::filesystem::path& workspace,
    std::string prompt
) {
    using namespace std::chrono_literals;
    const auto provider =
        hexloom::agents::parse_agent_provider(provider_name);
    if (!provider.has_value()) {
        std::cerr
            << "Agent provider must be 'codex', 'claude', 'gemini', "
               "or 'antigravity'.\n";
        return 2;
    }
    const auto access = parse_agent_access(access_name);
    if (!access.has_value()) {
        std::cerr << "Agent access must be 'read' or 'write'.\n";
        return 2;
    }

    try {
        const auto plan = hexloom::agents::make_cli_launch_plan(
            *provider,
            *access,
            std::move(prompt)
        );
        const auto result = hexloom::agents::run_process(
            {
                .executable = plan.executable,
                .arguments = plan.arguments,
                .working_directory = workspace,
                .timeout = 10min,
            },
            [](const hexloom::agents::ProcessOutput& output) {
                auto& stream =
                    output.stream ==
                            hexloom::agents::ProcessStream::standard_output
                        ? std::cout
                        : std::cerr;
                stream << output.data << std::flush;
            }
        );

        if (!result.launch_error.empty()) {
            std::cerr << "Agent launch failed: "
                      << result.launch_error << '\n';
            return 1;
        }
        if (result.timed_out) {
            std::cerr << "Agent exceeded the 10 minute timeout.\n";
            return 124;
        }
        return result.exit_code;
    } catch (const std::invalid_argument& error) {
        std::cerr << error.what() << '\n';
        return 2;
    }
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

    if (argc == 3 && std::string_view(argv[1]) == "memory") {
        return memory_command(argv[2]);
    }

    if (argc == 5 && std::string_view(argv[1]) == "compose-prompt") {
        return compose_prompt_command(argv[2], argv[3], argv[4]);
    }

    if (argc == 5 && std::string_view(argv[1]) == "agent-plan") {
        return agent_plan_command(argv[2], argv[3], argv[4]);
    }

    if (argc == 6 && std::string_view(argv[1]) == "agent-run") {
        return agent_run_command(argv[2], argv[3], argv[4], argv[5]);
    }

    print_usage();
    return 2;
}
