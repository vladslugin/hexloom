# Hexloom

**Where agents weave worlds.**

Hexloom is an AI-first game creation studio. Specialized agents will
design mechanics, generate C++ code and assets, assemble Godot projects, run
tests, and build games for desktop and mobile platforms.

Coding agents are designed as replaceable local providers. A user-installed
Codex or Claude Code CLI can reuse its existing account login, so Hexloom does
not require an API key or handle provider credentials. The first C++ adapter
builds safe structured launch plans; streaming process execution is the next
milestone.

Every change is checked on Linux against the C++ test suite, the CLI, and a
real headless Godot 4.7.1 process loading the native GDExtension.

## First vertical slice

The first milestone intentionally covers one complete workflow:

1. Read a structured material request.
2. Validate it in the engine-independent C++ core.
3. Generate or import a seamless texture set.
4. Create a Godot material and preview scene.
5. Run automated and visual checks.

The repository currently contains the first part of that workflow: the
engine-independent material contract, a CLI smoke test, unit tests, a native
GDExtension bridge, and a Godot Material Lab preview scene.

## Build

Requirements:

- CMake 3.25+
- Ninja
- a C++20 compiler
- Git (CMake fetches a pinned revision of `godot-cpp`)
- Godot 4.7

```sh
cmake --preset desktop-debug
cmake --build --preset desktop-debug
ctest --preset desktop-debug
./build/desktop-debug/apps/cli/hexloom validate games/material-lab/game.yaml
./build/desktop-debug/apps/cli/hexloom generate-textures \
  games/material-lab/game.yaml \
  games/material-lab/generated/stone \
  42
./build/desktop-debug/apps/cli/hexloom agent-plan \
  codex read "Review the material specification"
godot --path godot/material-lab
```

The desktop preset builds a deliberately trimmed `godot-cpp` API profile, so
the native adapter compiles only the Godot classes it actually uses.

`game.yaml` is the source of truth for the Material Lab. Both the CLI and the
Godot GDExtension parse it through the same C++ loader and return structured
validation issues for malformed or unsupported input.

The art style is also structured and validated. The first profile is
`hexloom_stylized_lowpoly`: low-poly geometry, exaggerated silhouettes, soft
bevels, stylized PBR surfaces, atmospheric lighting, and a limited palette.

The first Texture Generator provider is deterministic and offline. Before
generation, a provider-independent prompt compiler turns the material request
and art-style profile into positive and negative prompts. Each artifact stores
that input in `prompt.yaml`, while `artifact.yaml` records the provider, style,
seed, dimensions, filenames, and FNV-1a checksums. The deterministic provider
also derives its albedo from the declared palette. Output is committed
atomically and existing directories are never overwritten.

Material Lab can load these raw maps into a Godot `StandardMaterial3D`. The
automated Godot smoke test generates a temporary artifact through the real CLI,
loads it through the native bridge, applies it to the preview sphere, and
verifies the expected texture-loading signal.

## Repository layout

```text
apps/             User-facing Hexloom applications
engine/core/      Engine-independent C++ contracts and logic
engine/adapters/  Runtime-specific bridges, beginning with Godot
games/            Generated game workspaces
godot/            Godot adapters and preview projects
docs/             Architecture and product decisions
```
