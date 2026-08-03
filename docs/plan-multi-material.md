# Multi-material game specifications

## Context

`games/material-lab/game.yaml` declares a `game:` block and exactly one `material:`
block. The loader in `engine/core/src/material_spec_loader.cpp` reads the
`material:` map and the `art_style:` map, and never looks at `game:` at all — a
typo anywhere under `game:` is silently accepted today.

A game is more than one material. To move Material Lab from a single-texture
demo toward a real world, the specification has to carry a list of materials,
and the C++ core has to stay the source of truth for that list (Godot is an
adapter, not the schema owner).

The intended outcome: `game.yaml` accepts `materials:` as a sequence, `game:` is
validated instead of ignored, and every existing consumer — the `hexloom` CLI,
`HexloomMaterialBridge`, the Material Lab scene, and all 7 registered tests —
keeps working with no change in observable behaviour for single-material specs.

### Project-memory constraint that binds the design

The 18k-triangle scene budget is locked. `material_lab.gd` builds its preview
sphere at `radial_segments = 96, rings = 48` — roughly 9.2k triangles. Two such
spheres already exceed the budget. A multi-material Material Lab must therefore
swap one preview mesh, or drop per-preview tessellation, rather than instance a
sphere per material. This plan keeps the scene single-preview and defers the
gallery; Phase B below records what it would cost.

No locked entry has to be traded away to do this work.

## Scope decision

Split into two phases. **Phase A is the deliverable**; Phase B is recorded so
the sequencing is explicit, not to be built now.

- **Phase A** — schema, core loader, `validate` command, Godot bridge. Fully
  backward compatible; no artifact-layout change; no GDScript change required.
- **Phase B (follow-up)** — batch texture generation for N materials and a
  multi-material preview. Deferred because it forces a change to the on-disk
  artifact layout that `material_lab.gd`, `studio.gd`, and both Godot smoke
  tests all read.

The split works because of a load-bearing fact about the test wiring:
`games/material-lab/game.yaml` is used **only** by `hexloom_cli_validate`
(`apps/cli/CMakeLists.txt:10-16`). Every generation path — `hexloom_cli_generate`,
`hexloom_godot_smoke`, `hexloom_studio_smoke` — runs against
`engine/core/tests/fixtures/valid_game.yaml`, which stays single-material. So
Phase A can add a second material to the real game spec without any generation
code seeing a list.

## Ordered steps

### 1. Schema: accept `materials:`, keep `material:`

Target file: `engine/core/src/material_spec_loader.cpp`, plus a new
`engine/core/tests/fixtures/valid_multi_material.yaml`.

- `materials:` — a non-empty sequence of material maps. Each entry uses the
  same field set the current `material:` map uses.
- `material:` — a single map, retained as an alias for a one-entry list.
- Both keys present → one issue at field `$schema`, and stop. Neither present →
  the existing `material` "Required section must be a map." issue, unchanged.
- **Issue-path prefixes mirror the source key.** A spec written with `material:`
  reports `material.resolution`; a spec written with `materials:` reports
  `materials[0].resolution`. This is what keeps
  `engine/core/tests/material_spec_loader_test.cpp:72-82` passing untouched —
  its fixture `invalid_material.yaml` uses the singular key.

Factor the existing per-material body (`material_spec_loader.cpp:293-357`) into
a `read_material(const YAML::Node&, std::string_view field_prefix, ...)` helper
and call it once per entry. `read_maps` (`material_spec_loader.cpp:225-259`)
takes the same prefix parameter; its hardcoded `"material.maps"` strings become
`prefix + ".maps"`.

### 2. Core: `MaterialLoadResult` carries a list

Target: `engine/core/include/hexloom/core/material_spec_loader.hpp`.

Replace `std::optional<MaterialRequest> request` with
`std::vector<MaterialRequest> requests`, and add a
`[[nodiscard]] const MaterialRequest* primary() const` accessor returning
`requests.empty() ? nullptr : &requests.front()`.

Do **not** keep both `request` and `requests` as members. Two fields holding the
same material is a divergence bug waiting to happen, and the internal C++ API is
ours to change — the contracts that must not break are the CLI's argv/stdout,
the bridge's Dictionary keys, and the YAML files, all of which are preserved
below. `ok()` becomes `!requests.empty() && style.has_value() && issues.empty()`.

Three call sites update mechanically: `apps/cli/main.cpp:62,87`,
`engine/adapters/godot/src/hexloom_material_bridge.cpp:128-130`, and the
assertions in `engine/core/tests/material_spec_loader_test.cpp:41-55`.

### 3. Core: cross-material validation

New checks in the loader, after the per-material `validate()` loop:

- **Duplicate ids** → issue at `materials.ids`. This is not cosmetic: the
  generator names files `<material_id>_<map>.rgba8`
  (`deterministic_texture_generator.cpp:388-389`) in one flat directory, so two
  materials sharing an id would silently overwrite each other's maps.
- **Empty sequence** → issue at `materials`, "At least one material is required."

Per-material rules stay in `validate(const MaterialRequest&)`
(`engine/core/src/material_request.cpp:31`) and are reused unchanged.

### 4. Core: validate the `game:` section

New `GameManifest { std::string id, title, type; }` in
`engine/core/include/hexloom/core/material_request.hpp` (or a new
`game_manifest.hpp` alongside it), with a `validate(const GameManifest&)`
following the shape of the existing `validate` overloads.

`game:` stays **optional** — `valid_game.yaml` has no `game:` block and must
keep loading. When present, `id`/`title`/`type` are required and `id` reuses the
existing `is_portable_identifier` rule (`material_request.cpp:14`). Issues are
prefixed `game.`. Add `std::optional<GameManifest> game` to `MaterialLoadResult`.

### 5. CLI: print every material

Target: `apps/cli/main.cpp:55-73` (`validate_command`).

Keep the current line order and wording for the first material so nothing that
greps this output shifts. Emit one `material:`/`maps:` block per additional
entry, and a `materials: <n>` count line. `hexloom_cli_validate` asserts only on
exit code, so this is safe; the constraint is self-imposed for readability.

### 6. CLI: material selection for `generate-textures`

Target: `apps/cli/main.cpp:75-111,290-298`.

`generate-textures <spec> <out> [seed]` keeps its exact current behaviour when
the spec resolves to one material. When the spec declares more than one and no
material is named, fail with exit 2 and an error listing the available ids —
generating an arbitrary one would be worse than refusing.

Add an optional trailing `<material-id>` argument for the multi-material case.
The current dispatch is positional on `argc` (4 or 5); extend to 6 and
disambiguate seed-vs-id by trying `parse_seed` first
(`apps/cli/main.cpp:45-53`). Update `print_usage` (`main.cpp:20-32`).

### 7. Godot bridge: expose the list, preserve the existing keys

Target: `engine/adapters/godot/src/hexloom_material_bridge.cpp:122-152`.

Add a `materials` Array to the returned Dictionary, one entry per request, each
with the same shape `serialize_result` already produces. **Keep the top-level
`material_id`, `style_id`, `resolution`, `maps`, `valid`, `issues`, and
`art_style` keys, mirroring `materials[0]`.** That is what lets
`godot/material-lab/scripts/material_lab.gd:85-86,165` and its `validation_result`
lookups run unmodified. `validate_material` (the Dictionary-in overload) is
untouched — it validates one request by design.

`godot/studio/scripts/studio.gd` needs no change at all: it never calls the
bridge, only parses `artifact.yaml` (`studio.gd:802-844`).

### 8. Spec + fixtures

- `games/material-lab/game.yaml` → convert `material:` to a `materials:` list.
  Adding a second material here is safe (only `hexloom_cli_validate` reads it)
  and is the proof the feature works end to end.
- `engine/core/tests/fixtures/valid_game.yaml` → **leave singular and
  single-material.** It is the input for the CLI generation test and both Godot
  smoke tests; it is the backward-compatibility fixture.
- New `valid_multi_material.yaml` (two materials, distinct ids) and
  `invalid_multi_material.yaml` (duplicate ids, plus one bad field in the
  second entry to prove `materials[1].*` paths are reported).

### 9. Docs

`docs/architecture.md:31-39` describes `MaterialRequest` as "the first vertical
contract" in the singular. Update to say a specification carries a validated
list of material requests plus an optional game manifest.

## Affected gameplay systems

| System | Effect |
|---|---|
| Specification schema (`games/*/game.yaml`) | Gains `materials:`; `game:` becomes validated |
| Core loader / validation (`engine/core`) | List-aware; new cross-material and game-manifest rules |
| `hexloom validate` | Prints N materials; exit codes unchanged |
| `hexloom generate-textures` | Unchanged for 1 material; explicit selection required for N |
| Texture generation (`engine/generation`) | **Untouched in Phase A** — still one job per material |
| `HexloomMaterialBridge` | Adds `materials`; all existing keys preserved |
| Material Lab scene (`material_lab.gd`) | **No change** — reads the preserved top-level keys |
| Studio (`studio.gd`) | **No change** — reads artifact manifests, not specs |
| Project memory / agents | Not touched |

## Artifacts to produce

- Extended loader with `read_material(node, prefix, ...)` and cross-material checks
- `MaterialLoadResult { requests, game, style, issues }` + `primary()`
- `GameManifest` type and its `validate` overload
- CLI: multi-material `validate` output, material selector for `generate-textures`
- Bridge: `materials` array alongside the preserved single-material keys
- Fixtures: `valid_multi_material.yaml`, `invalid_multi_material.yaml`
- Converted `games/material-lab/game.yaml`
- New assertions in `engine/core/tests/material_spec_loader_test.cpp`
- Updated `docs/architecture.md`

## Risks

1. **Silent overwrite on duplicate ids.** The flat `<id>_<map>.rgba8` naming
   means duplicate material ids destroy each other's output. Mitigated by the
   step-3 check; this is the single most important new validation.
2. **Issue-path churn breaking the existing test.** `material_spec_loader_test.cpp`
   asserts on exact field strings including `material.maps[1]`. The
   prefix-mirrors-source-key rule in step 1 is what prevents this; if that rule
   is dropped, the test must be rewritten.
3. **Dual `request`/`requests` state.** Explicitly rejected in step 2 for this
   reason — flag it in review if it reappears.
4. **Bridge key removal.** Dropping the top-level `material_id`/`resolution`
   keys would break `material_lab.gd:85-87`, which fails the Godot smoke test
   only at runtime, not at compile time. Keep them mirrored.
5. **`game:` made mandatory.** Would immediately break `valid_game.yaml` and
   with it three tests. It must stay optional.
6. **Seed/id argv ambiguity.** A material id that parses as an integer would be
   read as a seed. `is_portable_identifier` allows all-digit ids. Try seed
   parsing first and document the precedence; a purely numeric material id is
   pathological but should be rejected explicitly rather than misread.
7. **Triangle budget (locked).** Any later multi-material preview must respect
   18k triangles — see Phase B.

## Phase B — deferred, recorded for sequencing

Not part of this change. Batch generation for N materials forces a decision
about artifact layout: `<id>_<map>.rgba8` files already carry the id and would
coexist flat, but `artifact.yaml` and `prompt.yaml` collide. Moving to
`<out>/<id>/artifact.yaml` requires coordinated edits to `material_lab.gd`,
`studio.gd:733-762,802-844`, `cmake/run-godot-smoke-test.cmake`, and
`cmake/run-studio-smoke-test.cmake` in one commit. A multi-material preview must
also swap a single mesh or reduce `radial_segments`/`rings` from 96/48 to stay
inside the locked 18k budget.

## Verification

Build per the project's Windows setup (vcvars64 shell; pass Godot's path
explicitly for the adapter):

```
cmake --build build/desktop-debug
ctest --test-dir build/desktop-debug --output-on-failure
```

Expect **7 tests** with the Godot adapter enabled: `hexloom_core_tests`,
`hexloom_generation_tests`, `hexloom_agents_tests`, `hexloom_cli_validate`,
`hexloom_cli_generate`, `hexloom_godot_smoke`, `hexloom_studio_smoke`. Any drop
below 7 means the adapter or `godot`/`godot4` on PATH was not picked up
(`CMakeLists.txt:81`), not that tests were removed.

Targeted checks:

1. **Backward compatibility** — `hexloom validate engine/core/tests/fixtures/valid_game.yaml`
   produces byte-identical stdout to the pre-change binary. Capture the current
   output *before* editing anything and diff.
2. **Multi-material load** — new assertions: `valid_multi_material.yaml` yields
   `requests.size() == 2` with the expected ids in document order.
3. **Path prefixes** — `invalid_multi_material.yaml` reports `materials[1].*`
   and `materials.ids`; `invalid_material.yaml` still reports `material.maps[1]`.
4. **Optional `game:`** — `valid_game.yaml` (no `game:`) still `ok()`;
   `games/material-lab/game.yaml` populates `result.game`; a `game:` block with
   a missing `title` reports `game.title`.
5. **Mutual exclusion** — a fixture with both `material:` and `materials:`
   reports `$schema` and does not crash.
6. **CLI selection** — `hexloom generate-textures games/material-lab/game.yaml <tmp> 42`
   exits 2 and lists ids once the spec has two materials; adding the id argument
   generates successfully. Confirm `<tmp>` is left absent on the failure path —
   the generator refuses pre-existing output directories
   (`deterministic_texture_generator.cpp:315-322`), so a stray directory from a
   failed run poisons the next attempt.
7. **Godot bridge** — `hexloom_godot_smoke` must still print
   "Hexloom texture artifact loaded"; that string is the assertion
   (`run-godot-smoke-test.cmake:60`) and it only passes if the preserved
   top-level `material_id`/`resolution` keys still reach `material_lab.gd`.
8. **Determinism** — generate the same single material twice with seed 42 and
   compare `checksum_fnv1a64` values in `artifact.yaml`. The deterministic
   provider stays the only texture source; nothing here introduces an image
   provider.
