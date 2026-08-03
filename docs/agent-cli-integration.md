# Local agent CLI integration

Hexloom supports user-installed coding agents without requiring a separate API
key. The studio launches each CLI as a child process and the CLI reuses its own
saved user login. Hexloom never reads, copies, or stores provider credentials.

## Architecture

```text
Hexloom terminal UI
        |
        v
Agent process supervisor
        |
        +---- Codex adapter ---- `codex exec --json`
        |
        +---- Claude adapter --- `claude -p --output-format stream-json`
        |
        +---- Gemini adapter --- `gemini -p --output-format stream-json`
        |
        +---- Antigravity ------ `agy --print --output-format stream-json`
        |
        +---- ACP adapter ------ JSON-RPC over stdio (planned)
```

`AgentLaunchPlan` is the first provider-neutral boundary for Codex, Claude,
Gemini, and Antigravity. It produces an
executable and an argv array, never a shell command string. This prevents prompt
text from being interpreted as shell syntax. Read-only access is the default;
workspace writes must be explicitly selected by the orchestrator and visible in
the UI.

For the interactive studio, Codex App Server is the preferred future adapter
because it exposes threads, turns, approvals, account login, and streaming over
JSON-RPC. `codex exec --json` is the smaller first integration and a useful
fallback for one-shot jobs. Claude Code provides non-interactive
`stream-json` input/output and resumable sessions.

Agent Client Protocol (ACP) is the intended open interoperability layer. It
standardizes local agents as subprocesses speaking JSON-RPC over stdio, which
matches Hexloom's terminal model. Provider-specific adapters remain necessary
until each installed CLI exposes a compatible ACP endpoint.

## Distribution and authentication

- Hexloom discovers a CLI installed by the user; vendor binaries are not
  bundled or redistributed.
- Initial login happens in the provider's own CLI or browser flow.
- API keys and access tokens are optional provider concerns, not part of game
  specifications.
- Child processes receive the smallest filesystem permission needed for a job.
- The UI must show command execution, file changes, approvals, and failures as
  normalized events before agents are allowed to edit game workspaces.

## Current milestone

Run this to inspect the exact argv that Hexloom will eventually pass directly
to the operating system:

```sh
hexloom agent-plan codex read "Review the material specification"
hexloom agent-plan claude write "Implement the selected mechanic"
hexloom agent-plan gemini read "Review the material specification"
hexloom agent-plan antigravity read "Review the material specification"
hexloom agent-run codex read . "Review the material specification"
```

`agent-run` is the first end-to-end entry point. It streams the provider's raw
JSONL to stdout and diagnostics to stderr, preserves the CLI exit code, and
applies a ten-minute safety timeout. One-shot jobs receive a closed standard
input so provider CLIs cannot accidentally wait for terminal input.

For Google consumer accounts, Antigravity is the supported free provider.
Google disabled `Login with Google` for consumer Gemini CLI accounts on
2026-06-18. The Gemini adapter remains available for API-key, Standard, and
Enterprise configurations.

The process-supervisor layer launches argv directly on macOS, Linux, and
Windows. It captures and streams stdout and stderr separately, preserves exit
status, supports cancellation, terminates the complete child process tree, and
enforces timeouts. macOS and Linux use `fork`/`execvp` with a process group;
Windows uses `CreateProcessW` with a job object that carries
`JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE`, so a cancelled run takes every process the
agent started with it.

Windows has no `execvp`, so the argv array is joined into one command line and
each entry is quoted using the parsing rules of the Microsoft C runtime. The
prompt therefore still arrives as a single argument regardless of its contents.
`PATH` and `PATHEXT` are searched directly rather than through a shell.

Batch scripts need `cmd.exe`, because a `.cmd` or `.bat` file has no entry
point of its own. This path cannot be avoided: a provider CLI installed through
npm lands as a `.cmd` shim on Windows. It is also where boundary rule 6 is
easiest to lose, so the escaping is deliberate.

`cmd.exe` parses the text **twice**: once for the line that starts the batch
file, and again for the line inside it that forwards `%*` to the real program.
A single caret only survives the first pass, so every metacharacter is written
`^^^&`, which becomes `^&` and then a literal `&`. Carets cannot protect
`%NAME%`, because expansion happens before carets are considered; a doubled
caret placed just after the percent spoils the variable name on both passes
instead. Arguments are argv-quoted first, so after cmd is finished the child's
own C runtime still receives an ordinary quoted command line.

`cmd.exe` is taken from the system directory rather than `%COMSPEC%`, and is
invoked with `/d` so no registry AutoRun command can run, and `/v:off` so `!`
cannot become delayed expansion.

This is the part of the supervisor most worth distrusting — the same class of
bug as Node's CVE-2024-27980, which that project resolved by refusing batch
files outright. The test suite therefore sends adversarial arguments through a
real shim that forwards `%*`, and asserts that quotes, `&`, `|`, `%`, and
`%PATH%` all arrive as literal text and that an injected `echo` never runs.

## Normalized event stream

Provider JSONL is decoded incrementally because operating-system pipe chunks do
not necessarily align with JSON lines. Codex, Claude, and Antigravity events are
mapped to the same event vocabulary:

`session_started`, `progress`, `message`, `tool_started`, `tool_completed`,
`completed`, `failed`, and `protocol_error`.

Raw JSON is retained for diagnostics. A one MiB line limit prevents a malformed
or hostile provider stream from growing the decoder buffer without bounds.

## Primary references

- [Codex non-interactive mode](https://learn.chatgpt.com/docs/non-interactive-mode)
- [Codex App Server](https://learn.chatgpt.com/docs/app-server)
- [Claude Code CLI reference](https://code.claude.com/docs/en/cli-usage)
- [Gemini CLI headless mode](https://google-gemini.github.io/gemini-cli/docs/cli/headless.html)
- [Google consumer migration notice](https://developers.google.com/gemini-code-assist/docs/deprecations/code-assist-individuals)
- [Antigravity CLI reference](https://antigravity.google/docs/cli/reference)
- [Agent Client Protocol introduction](https://agentclientprotocol.com/get-started/introduction)
