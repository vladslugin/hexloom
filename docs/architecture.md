# Hexloom architecture

Hexloom separates durable game data from replaceable tools and AI providers.

```text
Game specification
        |
        v
Agent orchestrator ----> Studio memory
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

## First artifact contract

`MaterialRequest` is the first vertical contract. It carries the artistic and
technical constraints needed by a future Texture Agent. The same request can be
validated headlessly, rendered in Godot, or processed by a different adapter.

`HexloomMaterialBridge` is the first GDExtension class. It converts Godot
dictionaries into the same engine-independent contract, runs validation in the
C++ core, and returns a structured result to the Material Lab scene.

## Texture generation providers

Texture generation is represented by a provider-independent
`TextureGenerationJob`. Providers return a versioned artifact manifest and
texture maps. The first deterministic provider exists to test reproducibility,
filesystem safety, and Godot integration before an AI image provider is added.

`compile_texture_prompt` converts the same structured request and art profile
into provider-neutral positive and negative prompts. Prompts are retained beside
every artifact so a later AI provider can be audited, reproduced, or replaced
without changing the game specification.
