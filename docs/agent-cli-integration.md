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
        +---- ACP adapter ------ JSON-RPC over stdio (planned)
```

`AgentLaunchPlan` is the first provider-neutral boundary. It produces an
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
```

The first process-supervisor layer now launches argv directly on macOS/Linux,
captures and streams stdout and stderr separately, preserves exit status,
supports cancellation, terminates the complete child process group, and
enforces timeouts. Windows has the same public interface; its native backend is
the next platform task. Event normalization and asynchronous UI ownership come
after that backend.

## Primary references

- [Codex non-interactive mode](https://learn.chatgpt.com/docs/non-interactive-mode)
- [Codex App Server](https://learn.chatgpt.com/docs/app-server)
- [Claude Code CLI reference](https://code.claude.com/docs/en/cli-usage)
- [Agent Client Protocol introduction](https://agentclientprotocol.com/get-started/introduction)
