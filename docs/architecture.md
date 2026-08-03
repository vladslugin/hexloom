# Hexloom architecture

Hexloom separates durable game data from replaceable tools and AI providers.

```text
Game specification
        |
        v
Agent orchestrator ----> Studio memory
        |
        +----> Local agent CLI adapters (Codex / Claude / ACP)
        |
        v
Engine-independent C++ core
        |
        +----> Headless validation
        |
        +----> Godot adapter ----> Desktop / Android / iOS
```

## Boundary rules

1. The C++ core must not include Godot headers.
2. Agents communicate through versioned requests and artifacts.
3. Generated assets retain their prompt, seed, model, and validation report.
4. Godot is the first runtime adapter, not the source of game rules.
5. Every generated artifact must be reproducible or explicitly marked manual.
6. Agent prompts are passed as argv values, never through a shell command.
7. Provider credentials remain owned by the installed provider CLI.

## First artifact contract

`MaterialRequest` is the first vertical contract. It carries the artistic and
technical constraints needed by a future Texture Agent. The same request can be
validated headlessly, rendered in Godot, or processed by a different adapter.

`HexloomMaterialBridge` is the first GDExtension class. It converts Godot
dictionaries into the same engine-independent contract, runs validation in the
C++ core, and returns a structured result to the Material Lab scene.

`HexloomAgentBridge` keeps provider processes outside Godot's main thread. It
accepts only structured provider, access, working-directory, and prompt values;
launches an argv vector without a shell; and exposes normalized events through
a polled, mutex-protected queue. Godot objects never cross the worker-thread
boundary. Cancellation is forwarded to the process supervisor, which
terminates the complete child process group.

## Project memory

`ProjectMemory` is the second durable contract. It carries what a world has
already decided — visual style, mechanics, constraints, and prior choices — as
a validated list rather than prose, so every agent receives the same context
without the creator restating it. Entries may be `locked`, which an agent must
treat as non-negotiable.

`compile_agent_prompt` turns memory, an access mode, and one direction into the
text a provider receives. It lives in the agent layer, not in the Godot script,
so the same wording reaches every front end and can be tested and inspected
without a running editor or an installed provider.

## Texture generation providers

Texture generation is represented by a provider-independent
`TextureGenerationJob`. Providers return a versioned artifact manifest and
texture maps. The first deterministic provider exists to test reproducibility,
filesystem safety, and Godot integration before an AI image provider is added.

`compile_texture_prompt` converts the same structured request and art profile
into provider-neutral positive and negative prompts. Prompts are retained beside
every artifact so a later AI provider can be audited, reproduced, or replaced
without changing the game specification.
