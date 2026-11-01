# Buoy Studio

A native Buoy development studio in the classic REALbasic/Xojo arrangement:
a Navigator (filterable project outline and control Library), a central
document with a WYSIWYG Design view and a Source view, a Messages pane, and a
collapsible Agent pane. Navigator, document, Messages and Agent regions use
native `Splitter` panes whose sizes persist. See [docs/DESIGN.md](docs/DESIGN.md), [docs/GLOSSARY.md](docs/GLOSSARY.md)
and docs/ACCEPTANCE.md (private maintainer material).

At compact window widths the tool palette collapses to keep the canvas and
Inspector usable beside Agent. Selecting a Library component or explicitly
showing the palette makes room by collapsing Agent, then Navigator if needed.
Their contents stay available through the View toggles. The palette returns
automatically when the document area grows.

Use **Dark Preview** in the toolbar to switch the design canvas between light
and dark rendering. **View → Canvas Appearance** also offers Light, Dark, and
Follow System. These controls affect the rendered layout and preserve the IDE's
own appearance and the document source; they are enabled in Design mode.

For the native Linux build from the assembled public workspace, see
[the native build guide](docs/NATIVE_BUILD.md):

```sh
mkdir -p dist/studio-builder
buoy tools/build_studio.bui -o dist/studio-builder/build-studio-native
./dist/studio-builder/build-studio-native
```

Use a separate executable directory for each build tool: Buoy deploys runtime
libraries beside executables, and replacing those libraries while another tool
runs can invalidate that process's mappings.

The retained Python builder provides the broader platform and release workflows.
Build the developer-tool packages, then run:

```sh
python3 tools/build_studio.py
python3 tools/build_studio.py --self-test
```

Without `--host`, the command builds on the current desktop. It seals current
library sources into an isolated consumer, checks the native build, verifies the
running executable's embedded identity, runs the Studio self-test and native
multicursor smoke test, and verifies the installed copy before opening it. Use
`--no-open` to verify and install a candidate without opening the installed preview. A fresh
preview gets its own build ID in its filename and window title; existing sessions
are left open. **Help → About Buoy Studio** shows the full source fingerprint,
build time, compiler, packages and rendering mode and can copy those details.
Remote exports omit compiler caches and generated executables, and rebuild native
glue on the destination. Build receipts remain in the printed build directory.

Output is `dist/BuoyStudio/BuoyStudio` on Linux,
`dist/BuoyStudio/BuoyStudio.exe` on Windows, or `dist/BuoyStudio.app` on macOS.

Windows x64 can also be cross-built from Linux with MinGW:

```sh
python3 tools/build_studio.py --target x86_64-windows --output dist/BuoyStudio-Windows/BuoyStudio.exe
```

Set `STUDIO_SHELL_CC` to override the cross C compiler, or `CC` for a native
build. The build creates an app-private Windows terminal DLL and import package,
then lets Buoy deploy the DLL beside the executable. This avoids linking MinGW
CRT objects against Buoy's incomplete Windows import stubs. Required developer
packages must contain fresh native slices for the selected target; semantic
checks alone do not validate those slices. `--self-test` runs only on the native
host target. The standalone shell check cross-links on other hosts and executes
its literal-argv/output/exit and failed-launch cleanup tests when run on Windows.

The preview package is stored without ZIP compression when staged, preserving
its payload bytes so Buoy 0.72's Windows compiler can discover it (BUI-008).
Windows designer self-tests dispatch enabled menu tags through Studio's command
route because nested `MenuItem.PerformAction` is defective (BUI-814); production
menu dispatch is unchanged. Native follow-through and its snapshot limits are in
the Windows acceptance report (not included in this repository).
The builder stages an isolated consumer project to avoid unrelated app resource
manifests. It checks the app on six Buoy targets before host compilation.
The `--self-test` mode drives native actions and requires a distinct success
marker; failure output cannot silently pass the runner.

The native TreeView, SearchField and SegmentedButton come unchanged from
`containers/integrations/`; the builder copies those module files into the
isolated build. It also routes `tools/discover_layout_packages.py` into the
app's Resources with an `app_resources.json` (at link time, so macOS signing is
unaffected) and stages the installed `fluenticons` package for toolbar icons
on Windows/Linux. `--seal-from-source layoutkit` (etc.) seals a library from
`libraries/` into the temporary build only, for development before packages
are resealed; `packages/` is never written by the builder.

Default staging always seals current ChatKit APIs from a private copy of its
manifest, modules and resources, even if the repository ChatKit archive is stale
or absent. `resources/studio-package-sources.json` records those copied input
hashes and the resulting archive hash; both the receipt and archive participate
in build identity. The fingerprint identifies concrete staged inputs, not every
file in the source library or a reproducible-build guarantee. Linux staging also
builds the lifecycle guard privately, without updating the shared library cache.

`--sealed-packages-only` deliberately uses pinned repository archives, including
required optional service archives. It never refreshes them from source. Missing
archives fail explicitly; an incompatible ChatKit archive fails an early Studio
API contract check with instructions to update the pin or use default staging.
It cannot be combined with source sealing or private target package rebuilding.

The required service packages are `secretstore`, `keychain`, `chatkit`, `editorkit`, `projectkit`, `agentkit`,
`agentsession`, `processkit`, `chatgptauth`, and `modelkit`, plus the installed
Buoy `crypto` package for authentication services. No duplicate system crypto archive is committed to this repository.

## Project items

The initial source and notes items are in-memory demo documents. Open item adds
an existing source/document file to the model. User edits immediately update the
registered buffer, its revision and dirty flag; attached agent context reads
that buffer, including unsaved text. Proposed project changes are previews and
remain unapplied until the user chooses Apply changes. Revisions are checked
again when applying; a concurrent user edit makes a proposal stale. Undo/Redo
operate on the project model after either manual or approved agent edits.

File actions include New Source/Text, Save, Save As, Save All, Review Unsaved
Changes, Compare with Saved, Revert to Saved, Rename File, Move File, Close Item,
Close Other Items and Close All Items. Every close/quit path shows one change
review (see below) instead of a prompt per file. Save As refuses to overwrite an
unrelated existing file; for a registered project file it creates and opens a
copy so the original source identity stays intact.
Saves check the accepted disk snapshot before an
AtomicFileKit replacement. This detects external edits but does not lock out
external writers. New destinations are reserved exclusively.

Markdown, JSON and text files have their own document inventory. They are
editable and searchable without entering the compiler/refactor source closure.
Tabs and MRU navigation retain authoritative workspace buffers; the optional
source split shows the same document with independent caret, scroll and fold
state. Both views edit the same revision-checked workspace.

Ordinary saves preserve the document's accepted newline style, including CRLF
and classic Mac CR, and retain its BOM and mixed-ending metadata. Explicit
conversion actions support CR/LF/CRLF/NEL/LS/PS; Unicode separators remain literal
unless an import action is selected. Metadata-only conversions are dirty and
undoable. EditorConfig preparation runs on save. Recovery checkpoints, session
locations and pre-save history are kept per workspace under the application
support directory; recovery and history restoration are reviewed before use.

Language, project search/TODO, navigation/bookmarks/clipboard history, comparison,
external-change review and build/run actions are reachable from native menus.
Navigate > Check Project Spelling scans workspace source and documents, including
unopened files and unsaved buffers. It reuses the Project Results window with
search and TODO/FIXME scans, retaining each mode's results and navigation cursor.
F4 / Shift+F4 move through project search results; F7 / Shift+F7 move through
spelling issues. Previous/Next buttons wrap across files and select the exact
match. Include/exclude globs apply to scans. Spelling runs in cancellable batches;
scan limits are reported. After editing a result's document, rescan before using
its old locations. Correct spelling through the source editor; spelling results
cannot be applied as bulk search replacements.
Language and search edits validate revisions and disk snapshots and enter the
workspace as grouped undoable changes without saving files. External review
supports reload or keeping local text; it does not perform automatic merging.
The Source menu exposes the editor command catalog with enabled-state checks and
custom shortcut routing. Source copy/cut events feed clipboard history. Snippet
Templates offers preview, insertion, and JSON import/export compatible with
Xippets. Imported templates persist in Studio preferences; insertion checks the
workspace and every caret against the picker snapshot, then creates one undoable
linked-placeholder edit. Export requires a new filename.

Local Git loads status, patches and index comparisons; ARM64 Linux/Windows
report it unavailable because the packaged GitKit native slices are missing.

Run Active compiles unsaved source snapshots. Run in Terminal passes an explicit
argument vector to PTY/ConPTY without constructing a shell command. Stop is
asynchronous: project switching and closing retain jobs and hosts until they
finish cleanup. Check All Targets reports compiler diagnostics for each target.
Buoy language services start automatically when Studio opens a project. The server
is discovered from `BUOY_LSP` (an explicit executable path), `BUOY_ROOT`, standard
Buoy installations, or next to a compiler on `PATH`. If discovery fails, set
`BUOY_LSP` before launching Studio. **Language > Server Status** shows the selected
executable, state and latest failure; **Restart Language Server** reconnects and
resynchronizes source buffers.

Completion while typing is on by default for Buoy source, including both split
views: identifier prefixes of two or more characters and `.` request completion;
`(` and `,` request signature help. **View > Editor > Language Completion While
Typing** toggles and saves this preference. Explicit **Language > Complete** and
**Signature Help** remain available when automatic completion is off. Results
require selection and acceptance; edits are grouped for undo.

Language requests depend on the configured server's capabilities and responses;
unavailable results are reported without fabricated completion or rename edits.

The shell runner checks six targets and exercises real document/workspace edits,
conflicts, persistence, language/search transactions, comparison, literal terminal
arguments and asynchronous run cancellation on Linux. Its default Linux runtime
uses the repository GUI container; `--host` uses installed native libraries.
Six-target compiler checks are not macOS/Windows native runtime coverage.

## Change review, file rename/move and undo

**Change review** (`StudioChangeReview.bui`). One modal window lists a batch of
file changes with a checkbox each and shows the unified diff of the selected one.
It is used for:

- *Unsaved changes* (Close, Close Others, Close All, Quit, Save All, File → Review
  Unsaved Changes): checked items are saved and unchecked items are discarded.
  Saves run first, and nothing is discarded if a save (e.g. a cancelled Save As)
  fails. A file whose text matches disk but whose line endings, byte-order mark or
  final newline differ is labelled "line endings / encoding only".
- *Rename/Move File*: each affected file is one entry you can approve or exclude.

**Rename/Move File** (`StudioFileRefactor.bui`, File menu and Navigator context
menu). The planner is pure: it proposes per-file rewrites and the host applies the
approved subset.

- Checked by default: Buoy `Import Name` / `Import "rel/path"` (incl. `Public
  Import`, `As`, `.{ }`), resolved like the compiler (next to the importer, then
  the entry file's folder). A move that breaks name lookup turns the import into a
  relative quoted path. Markdown inline and reference-style links, HTML `href`/`src`,
  CSS `url()`, and quoted relative paths in JSON/YAML/TOML. The moved file's own
  imports and links are re-pointed too.
- Also: moved config files (JSON/YAML/TOML) re-point their own relative paths;
  unquoted YAML values that name a moved file are rewritten; TOML multi-line
  strings are left alone.
- Unchecked by default: `OldModule.Name` qualifiers (a class can share the module's
  name) and plain-text mentions of the old path or file name.
- Refused: renaming `main.bui` (the entry file), a destination that exists, a
  module name already used by another source or an installed package, and changing
  a `.bui` extension. Unsaved edits in affected files go through the unsaved review
  first. Nothing runs while a program, the agent or unscanned/unsaved documents
  would make the project state unprovable (same rules as Rescan Project Sources).
- Applying is all-or-nothing on disk (written files are restored if a later write
  or the move fails). The open project is updated in place: the moved item keeps
  its id, tab and text undo history (`ProjectWorkspace.RenameItem`,
  `StudioProjectSession.RenameEntry`); agent chat, terminal, debugger and preview
  are untouched. If the in-place update cannot be validated, Studio falls back to
  a full rescan.
  Files whose imports or links were rewritten take the new text without a history
  frame and start a fresh text history (older frames would bring back the old
  references); moved files that were not rewritten keep theirs.

**Undo/redo.** Text edits use ProjectKit's per-document history (multi-file
refactors are one transaction). File renames/moves record the exact disk bytes of
every rewritten file. Edit → Undo (Cmd/Ctrl+Z) acts on whichever happened last for
the current document: its own text edit, or a later file rename/move; Redo mirrors
it. Edit → Undo/Redo File Rename/Move address the file history directly. File undo
refuses (changing nothing) if any affected file was edited since, unsaved, or
changed on disk.

**Folders.** File → Rename Folder… / Move Folder… act on the selected file's
folder: the folder moves as a whole (non-project files included) and every
reference into, out of and inside it is planned in one pass (`StudioPlanFolderMove`).
Files that move together keep their mutual imports; a folder holding `main.bui`
is refused.

**Persistence and tooling.** The last 20 file operations each way are saved with
the workspace session and restored on reopen (each is re-verified against disk
before use). The language server is told about renames (old document closed, new
one opened).

**Tests.** `tests/studio_file_refactor_test.bui` (planner), `tests/projectkit_rename_test.bui`
(ProjectKit RenameItem/ReloadItem), `tests/studio_review_native_test.bui` (the review
window's native list/checkbox cells and a real folder move with undo/redo on disk;
verified on Linux, macOS and Windows), and the Studio `--self-test` IDE step (browse
stays clean, rename/folder rename/undo/redo in place, unsaved review).

### Not done yet

- **Framework API**: per-control native snapshots for the designer canvas belong in
  Buoy itself (draft BUI-709, `upstream-issues/BUI-709-control-snapshot-api`);
  Studio currently ships its own per-platform adapters.

## Native layout preview and GUI checks

Choose **Preview in Native Runtime** to compile and run the active Window/View
from its current buffer, including unsaved changes. Studio reports source-only
GUI lint findings in Messages before launch, then compares the native bounds
with the designer's literal frames. Unverified accessibility and interaction
checks remain explicitly unverified; a source report or matching frame does not
prove screen-reader associations or keyboard behaviour.

The generated harness now uses the same `PreviewSession` as the standalone
[native preview tool](https://github.com/BuoyBay/devtools/blob/main/tools/preview/README.md). Studio bundles its freshly
sealed PreviewKit archive as an app resource and stages it into each private
consumer, so previewing does not require a separately installed PreviewKit.
Qualified source paths identify observations; typed access follows Buoy's
root-level lowering of nested controls. Interactive Studio preview does not
capture screenshots or send permission-helper commands.

It checks the Studio preview module and generated harness on all six targets,
then verifies the shared native snapshot under private X11 on Linux.

## Agent connections

The default provider is explicitly **Demo agent · local**. It reads the attached
registered item and proposes a small source change without any model request.
Configure actual local providers before launching Studio:

| Environment variable prefix | Protocol | Default argv |
| --- | --- | --- |
| `BUOY_STUDIO_CODEX` | Codex app-server | `app-server --listen stdio://` |
| `BUOY_STUDIO_CLAUDE` | Claude stream-json | `-p --input-format stream-json --output-format stream-json --verbose` |
| `BUOY_STUDIO_COPILOT` | GitHub Copilot CLI · ACP | `--acp --stdio` |
| `BUOY_STUDIO_GEMINI` | Gemini CLI · ACP | `--acp` |
| `BUOY_STUDIO_OPENCODE` | OpenCode · ACP | `acp` |
| `BUOY_STUDIO_QWEN` | Qwen Code · ACP | `--acp` |
| `BUOY_STUDIO_GOOSE` | Goose · ACP | `acp` |
| `BUOY_STUDIO_VIBE` | Mistral Vibe · ACP (`vibe-acp`) | No arguments |
| `BUOY_STUDIO_ACP` | Agent Client Protocol | No arguments |
| `BUOY_STUDIO_T3` | MCP endpoint | No arguments |

For each prefix, set `_EXECUTABLE` to an absolute executable path and optionally
`_ARGS` to a JSON array of exact argument strings. No shell parsing is involved.
`BUOY_STUDIO_CWD` supplies the session working directory, otherwise the app uses
`PWD` or `.`. The ordinary Codex and Claude endpoints retain their own authentication
configuration. The separate ChatGPT plan provider uses the subscription flow
below and sends its access token only through the child environment.

T3 Code is reached through a configured MCP endpoint. Studio discovers that
endpoint's tools; it does not invent a private T3 protocol or tool name. Set
`BUOY_STUDIO_T3_PROMPT_TOOL` to an actually discovered tool and optionally
`BUOY_STUDIO_T3_PROMPT_FIELD` / `_CONTEXT_FIELD` to its schema's field names
(default `prompt` / `context`). `BUOY_STUDIO_T3_TOOL_ARGUMENTS` supplies a JSON
object of fixed tool arguments, such as an existing thread ID required by the
selected tool; the prompt and context are overlaid on that object. Fixed arguments
are limited to 64 KiB. Select an actually exposed tool and its schema; Studio does
not create threads implicitly. A raw MCP endpoint without an explicit prompt
tool is a tool connection, not a chat model.

Subprocess sessions advance through a main-thread timer and display text,
tool activity, proposal and permission events. Allow once/Deny are translated
through the selected protocol's offered decisions. The session layer can reject
unsupported unsafe provider capabilities rather than silently allowing them.
Stop requests cancellation. Closing the window closes its process session.

## AI autocomplete

Open **Settings / Preferences** to select an autocomplete provider independently
of the agent in Chat. Autocomplete starts **off**; enable automatic suggestions
explicitly or press **Alt+\\** to request a suggestion.
**Alt+Shift+\\** toggles automatic suggestions.
Ghost text stays a preview until **Tab** accepts it; **Escape**, typing, navigation,
selection changes and document changes invalidate pending suggestions. An accepted
suggestion uses the editor's ordinary undo transaction. The existing chat-agent
completion path remains available as **Selected chat agent (compatibility)**.

- **GitHub Copilot:** install the official [Copilot language server](https://github.com/github/copilot-language-server-release)
  and choose its absolute native executable path in Preferences, or set
  `BUOY_STUDIO_COPILOT_LSP_EXECUTABLE`. Studio runs it with `--stdio`, keeps the
  process alive, synchronizes the authoritative unsaved document and uses
  `textDocument/inlineCompletion`. Click **Sign in to Copilot**: Studio copies
  the device code and asks you to continue to GitHub. Server status appears in
  Preferences. This is separate from the Copilot CLI agent connection. Suggestions
  that repeat an identical prefix are converted to insertions; other
  replacement ranges are shown through the editor’s revision-checked inline edit
  preview. This integration requests ordinary completions; Copilot’s separate
  next-edit prediction method is not enabled.
- **Codestral:** set `MISTRAL_API_KEY` (or `BUOY_STUDIO_CODESTRAL_TOKEN`). The default
  model is `codestral-latest`; choose another FIM model in Preferences or set
  `BUOY_STUDIO_CODESTRAL_MODEL`. Requests use `/v1/fim/completions` with both
  prefix and suffix.
- **Ollama:** run Ollama and install a model that supports fill-in-the-middle.
  Choose its exact installed name in Preferences or `BUOY_STUDIO_OLLAMA_MODEL`.
  No model is downloaded automatically. Requests use `/api/generate` with prefix,
  suffix, a bounded output and five-minute model keep-alive.
- **Tabby:** run a Tabby completion server and set its URL in Preferences.
  Set `BUOY_STUDIO_TABBY_TOKEN` if the server requires authentication. Requests
  use `/v1/completions` with language, relative file path, prefix and suffix.

Server URL overrides are `BUOY_STUDIO_CODESTRAL_BASE`, `BUOY_STUDIO_OLLAMA_BASE`
and `BUOY_STUDIO_TABBY_BASE`; these are server roots (Studio appends the route).
Defaults are `https://api.mistral.ai`, `http://localhost:11434` and
`http://localhost:8080`. HTTPS is required except on `localhost`/`127.0.0.1`.
API tokens are read from the environment, never saved in Preferences. HTTP requests
run off the UI thread with redirects disabled and a ten-second timeout. Cancelled
HTTP replies are ignored; the request finishes before another starts, bounding
background work. Copilot cancellation uses LSP `$/cancelRequest`.

HTTP providers receive up to 6,000 characters before and 2,000 after the cursor;
Copilot receives the active unsaved document. Choose local Ollama/Tabby when code
must stay on a server you control. No automatic fallback sends code to another
provider. These integrations require the user's configured service/model and
account; offline fixtures validate protocol behavior without a paid account.

The named agent presets use the existing ACP session layer, including its proposal
review and permission handling. Sign in using each CLI's own supported login flow
before connecting. `_EXECUTABLE` and `_ARGS` overrides work as above; for Node-based
CLIs on Windows, point `_EXECUTABLE` at `node.exe` and include the CLI script path
before its ACP arguments in the JSON `_ARGS` array. Mistral Vibe uses `vibe-acp`.
The presets follow the [ACP registry](https://github.com/agentclientprotocol/registry);
custom ACP remains available for other agents.

This saves the built app, per-step logs and `validation.json` under
`dist/studio-container-validation/`, with the repository mounted read-only and no
network or real model credentials during testing. See
container setup (not included in this repository)
for bootstrapping a new machine and the separately tracked Linux BuoyScript SDK
availability limitation.

Maintainers run the private validation suite before release. See the repository README for public build instructions.

## Continue with ChatGPT

The native Continue with ChatGPT action starts an actual PKCE sign-in flow. Studio
first binds an ephemeral HTTP callback listener to **127.0.0.1 only**, then opens
the exact authorization URL in the default browser. The listener accepts a small
ASCII HTTP GET with the matching Host header; the authentication package verifies
callback state and the issued client registration before exchanging its code.
Discovery, JWKS/RS256 identity verification and token exchange run in a background
task; a timer publishes results to the UI. Select **ChatGPT** in the backend
picker to show its account controls. The button displays the signed-in state
and prevents duplicate sign-in attempts. Sign out forgets the stored session
before attempting remote revocation, and reports remote revocation failures.

Only the nonsecret installation host ID is stored in plain `Prefs`
(`com.buoy.studio`, key `chatgpt-host-id`). Renewable credentials use the reusable
`secretstore` package backed by macOS Keychain, Windows Credential Manager, or
Linux Secret Service. The stable service/account identity survives builds and
bundle path changes. Startup validates the saved identity and renews its grant
on a worker. Cross-process locking protects rotating refresh tokens. Storage
failures remain visible; there is no plaintext fallback. An account's identity
token is never treated as a legacy Codex account-token handoff.

Studio discovers native Codex installations on each platform, including npm's
native payload so GUI launches do not depend on finding Node. An explicit
`BUOY_STUDIO_CODEX_EXECUTABLE` still overrides discovery. Successful sign-in or
restore selects ChatGPT when the local Demo backend was selected. The host uses
OpenAI's documented custom Responses provider with
`ACCESS_TOKEN` as `env_key`; credentials are absent from argv and provider config.
There is no separate Codex login. After a refresh, Studio restarts the subscription
child and resumes its existing thread on the next send when a thread ID exists.
Refresh/sign-out are serialized and unavailable during a running chat turn. An
HTTP operation already in progress cannot be aborted. Explicit sign-in cancellation
forgets a completed exchange; a completed refresh keeps its valid credentials.
Closing the window preserves remembered sign-in. No browser or real sign-in is started by the
native self-test.

Reference: [OpenAI's Codex app-server subscription integration](https://developers.openai.com/siwc/token-sharing-open-source/codex-app-server).

## Shared agent tools and Buoy resources

The selected backend works against the registered `ProjectWorkspace`, including
unsaved buffers. Tool dispatch follows the actual host registry. Codex uses its
dynamic tools, Claude uses a configured SDK MCP server with built-in tools
disabled, and direct model APIs receive the same definitions as function tools.
ACP file access is translated into registered-buffer reads and reviewed proposals;
a write requires an unchanged revision previously read by that session. Unsupported
native execution requests are decline-only. The host does not claim that a
provider's read-only sandbox is an OS boundary around all possible disk reads.

The native permission selector above the agent chat is shared by WebKit and native
transcripts: **Read only** blocks proposals, scripts and renders; **Review edits**
keeps buffer changes pending until Apply; **Auto-approve edits** applies allowed
changes immediately with Undo support. These modes govern Studio project edits;
they do not enable provider-native commands or add per-call approval to scripts.

**Tools…** opens individual tool switches plus independent switches for MCP project
buffer resources, attached project-item previews/file references, and compiler
guidance in context. Disabling a tool removes it from discovery and rejects direct
calls, including project-tool callbacks from scripts and ACP file access. Scripts
still run with OS privileges, so these switches are not a script sandbox. External
MCP servers and provider-owned capabilities retain their own settings. Access is
saved locally; changing it requires an idle agent with resolved proposals and
starts a fresh session. Read-only restrictions override enabled tool checkboxes.

| Tool | Behavior |
| --- | --- |
| `project_workspace` | Registered-item counts and explicit buffer/proposal policy. |
| `project_list_items` | Registered item identities, paths, revisions and dirty state. |
| `project_read_item` | Current unsaved text; optional bounded line ranges with UTF-8 byte positions. |
| `project_search` | Bounded literal search over current buffers, with revisions and byte positions. |
| `project_propose_edits` | Revision-checked undoable proposal; applies immediately only with user-enabled auto-allow. |
| `project_proposal_status` | Pending, stale, applied or rejected status and expected/current revisions. |
| `buoy_search_docs` | Bounded title/path/summary index search in the selected compiler's documentation. |
| `buoy_read_doc_page` | Bounded selected-version documentation reads, confined to its docs root. |
| `project_check` | Start check-only compilation of a frozen unsaved-buffer snapshot; optionally all six targets. |
| `project_check_result` | Nonblocking progress, bounded diagnostics, captured revisions and stale/clean state. |
| `project_script_help` | BuoyScript primer, examples, setup, limits and error recovery. |
| `project_script` | Start a native script in the configured optional helper; chain project tools. |
| `project_script_result` / `project_script_cancel` | Read or cancel the current script; prior edits remain undoable. |

A compiler check never links, runs the program or applies an unapplied proposal.
Structural file tools and agent-controlled approval are not exposed. Proposed
edits use review or user-enabled auto-allow. Configuring the optional BuoyScript
helper enables native execution with user privileges; it is not a sandbox.
A successful buffer application is distinct from saving to disk. Checks and
symbols/test execution are separate capabilities: this release does not expose
language-server symbols or configured build/test execution as agent tools.

Before sending a non-demo turn, Studio runs the selected compiler's `--version`
and `ai init --agnostic` in a private temporary project. It imports that version's
syntax/verification guidance and the current project's `AGENTS.md`, and exposes
its documentation through the tools above. It never initializes the user's live
project. Wrapper/compiler metadata, the resolved compiler, syntax kit and index
invalidate the binding on upgrades; project instructions are reread for each
turn. MCP configuration in the resource context is a reference, not a claim that
an external Node.js MCP server has been connected. Attached-item previews are
bounded and explicitly marked as potentially stale; tools provide current text.

For a disposable app-level account/backend regression without designer tests,
run the preview with `--agent-self-test`. This never signs into a real account.
The wider `--self-test` remains a separate check and can expose unrelated failures.

## Grok and Meta Muse

These are working Responses API adapters, not subprocess placeholders. Configure
an explicit model plus a token before opening Studio:

| Provider | Token environment | Model environment | Optional API base |
| --- | --- | --- | --- |
| Grok | `XAI_API_KEY` or `BUOY_STUDIO_GROK_TOKEN` | `BUOY_STUDIO_GROK_MODEL` | `BUOY_STUDIO_GROK_BASE` |
| Meta Muse | `META_API_KEY` or `BUOY_STUDIO_MUSE_TOKEN` | `BUOY_STUDIO_MUSE_MODEL` | `BUOY_STUDIO_MUSE_BASE` |

Defaults are the modelkit xAI/Meta API roots. Availability is configuration-based;
Studio does not test credentials or make an inference request until Send is
pressed. Model responses use background HTTP tasks. Main-thread polling executes
the declared project-list/read/propose tools against current unsaved buffers, so
editing remains responsive and project state is never accessed by HTTP workers.
The loop is bounded to eight model turns and 32 function calls per turn. Responses
are buffered rather than token-streamed. Stop immediately ignores late results;
it does not claim to abort an in-flight HTTP request.

Project edit proposals remain previews until Apply changes, and stale revisions
are rejected. Multiple remote proposals are queued for review. No remote-model
tool can automatically approve or save project edits.

## Designer, rename/delete and package discovery

The Contact sample opens in Design. Canvas gestures and Inspector edits are
revision-checked `ProjectWorkspace` transactions with undo/redo. Select a Library
control to open the palette, choose a unique name, then Insert or Place it on the
canvas. The Inspector also supports structured constraint authoring. Computed
and unsupported regions remain read-only as LayoutCanvas defines.

Studio implements `LayoutProjectRefactorHost` for LayoutCanvas's rename and
delete actions. It plans over every registered `.bui` buffer (unsaved text
included, identified by relative source path), re-checks every buffer's
revision after planning, and applies the plan as one atomic proposal. It is
enabled for the complete in-memory sample or a project opened with **File > Open
Project** / `--project FOLDER`. Studio recursively registers unopened `.bui`
sources, including code-behind; unsaved buffers override their disk text. It
refuses symlinks, ambiguous module basenames, unresolved imports and lossy input.
Inventory and exact disk baselines are checked before planning and again before
applying a refactor. **File > Rescan Project Sources** includes added files and
preserves unsaved buffers, refusing conflicts. Standalone opened files still
leave project refactors disabled until their complete project is scanned.

Start Studio with `--project FOLDER` (or `BUOY_STUDIO_PROJECT`) to discover
package controls for that folder. Studio runs only its bundled helper (or
`BUOY_STUDIO_DISCOVERY_HELPER`) with Python 3 (`BUOY_STUDIO_PYTHON` or a
standard location), asynchronously and without a shell. A fresh catalog
replaces the Library only after the helper exits 0 and its new index parses;
otherwise Messages explains the failure and the previous catalog stays.

### Masked text fields

Add the `formatcontrols` package and its dependencies to the project, then open
that project in Studio. Package discovery adds **Masked text field** to the
Library's **Text input** category. Insert it like other controls; Studio adds
`Import formatcontrols.FormatDesigner` to the source.

Select the control's **Input mask > MaskPattern** property in the Inspector.
The built-in pattern editor previews raw sample characters before **Apply**
commits one undoable source edit. Invalid patterns cannot be applied. Existing
computed expressions remain read-only in this editor and are preserved.
Changing a mask also checks the control's existing raw `Value`; clear or update
that value first if it does not fit. The **Value (raw characters)** editor checks
the current pattern and rejects formatted punctuation. Computed sibling values
are preserved and must be updated together in Source.

An empty pattern disables masking. MaskKit patterns use `#` for a digit, `L`/`l`
for upper/lowercase letters, `A` for a letter, `*` for a letter or digit, `.` for
any character and `\` to escape a literal. Lowercase `a` is a literal. For
example, `(###) ###-####` formats raw `2125550199` as `(212) 555-0199`.

`RawInput` chooses raw characters or already formatted text during entry;
`AllowsEmpty` controls whether an empty value can commit. `Value` contains the
canonical raw characters. Validation runs during editing; formatting happens
when editing commits. Invalid drafts retain the previous committed value. This
control does not insert punctuation on every keystroke or implement Xojo's
different `ValidationMask` grammar.
Literal-only masks display their constant with an empty raw value. Application
code can use `ReplaceValue` to explicitly load a new record over a rejected draft,
or `CancelEdit` to restore the old value.

Maintainers run the private validation suite before release. See the repository README for public build instructions.

## Designer menus, toolbar and shortcuts

Every LayoutEditor command is on the menubar (`StudioDesignerMenus.bui`). An item's
tag is `design:<command>` or `edit:<command>`; `StudioWindow.Action` sends it to
`LayoutEditor.PerformCommand` in Design mode. Enabled and check-mark state come from
`CanPerformCommand` and `CommandChecked`. Studio implements
`LayoutEditorCommandObserver`. Each change notification marks the menus dirty, and
the next 30 ms poller tick refreshes them. `MenuNeedsUpdate` also refreshes them just
before a menu opens. In Source mode Duplicate (Primary+D) adds the next occurrence of
the selection as another cursor (VS Code; Copy Line Down is Shift+Alt+Down), and
Indent and Outdent act on source lines; visual arrangement commands remain disabled.

- **Edit**: Undo and Redo (project-wide), Cut, Copy, Paste, Duplicate, Delete,
  Select All Controls, Deselect All, Select Parent.
- **View**: Design/Source, **Preview in Native Runtime**, Stop Preview, Zoom
  In/Out, Actual Size, Zoom to Fit, plus toggles for Show Grid, Snap to Grid,
  Smart Guides, Show Bounds (outline every control) and Show Spacing (distance
  markers). It also has the panel toggles (Navigator, Agent, Messages, Inspector,
  Tool Palette).
- **Arrange**: these items are grouped into submenus:
  - Align (left, centre and right edges; top, middle and bottom edges)
  - Align To, with three radio choices: Selection, Key Object or Container
  - Center in Container
  - Distribute (edges or centres)
  - Spacing (make equal, increase, decrease or remove, horizontally and vertically)
  - Size (same width, height or size, or the default size)
  - Order (front, forward, backward, back)
  - Embed in View / Unembed
  - Nudge

  The key object is the first selected control.
- **Toolbar**: align left/centre/right/top/middle/bottom, distribute horizontal and
  vertical centres, and **Preview**. Each button is disabled when its command cannot
  run.

| Command | macOS | Windows | Linux |
|---|---|---|---|
| Design / Source | Cmd+1 / Cmd+2 | Ctrl+1 / Ctrl+2 | Ctrl+1 / Ctrl+2 |
| Preview in Native Runtime | Cmd+R | Ctrl+R | Ctrl+R |
| Stop Preview | Cmd+Alt+R | Ctrl+Alt+R | Ctrl+Alt+R |
| Duplicate | Cmd+D | Ctrl+D | Ctrl+D |
| Bring Forward / Send Backward | Cmd+] / Cmd+[ | Ctrl+] / Ctrl+[ | Ctrl+] / Ctrl+[ |
| Bring to Front / Send to Back | Cmd+Shift+] / Cmd+Shift+[ | Ctrl+Shift+] / Ctrl+Shift+[ | Ctrl+Alt+] / Ctrl+Alt+[ |
| Zoom In / Out / Actual Size | Cmd+= / Cmd+- / Cmd+0 | Ctrl+= / Ctrl+- / Ctrl+0 | same as Windows |
| Zoom to Fit | Cmd+9 | Ctrl+9 | Ctrl+9 |
| Show Grid / Show Bounds | Cmd+Alt+G / Cmd+Alt+B | Ctrl+Alt+G / Ctrl+Alt+B | same as Windows |
| Embed in View | Cmd+Alt+E | Ctrl+Alt+E | Ctrl+Alt+E |
| Navigator / Agent / Messages | Cmd+Alt+1/2/3 | Ctrl+Alt+1/2/3 | Ctrl+Alt+1/2/3 |
| Inspector / Tool Palette | Cmd+Alt+4 / Cmd+Alt+5 | Ctrl+Alt+4 / Ctrl+Alt+5 | same as Windows |
| Cut, Copy, Paste, Select All, Delete, arrows | handled by the focused canvas | same | same |

These shortcut choices follow behaviour checked on GTK under Xvfb with synthetic
X key events:

- A menubar shortcut takes priority over the focused native text field.
- Setting an item's `Enabled` to `False` greys it out, but its shortcut still fires.

Studio therefore re-checks `CanPerformCommand` before every dispatch. Text-editing
chords stay unbound in the menubar so they keep working in the Inspector, the filter,
the chat composer and the source editor. The design canvas handles them itself when
it has focus. The menu items still work by mouse.

In Buoy 0.72, `Primary+Shift+<key>` menubar shortcuts never fire on Linux. That
includes the existing Redo (Ctrl+Shift+Z), Show Tests and Documentation shortcuts.
Linux therefore uses Ctrl+Alt for the front/back z-order chords. Linux also does not
draw check marks for menu items. The canvas and the toolbar show the toggle states
there instead. The navigator's TreeView component has no per-row images, so Library
entries stay text-only. Control icons appear in the tool palette.

## Native-runtime preview

The designer paints a literal-frame model. Only the compiler and the native
runtime show what users will actually see: native control metrics, constraints,
custom controls and package controls. Several approaches were considered:

1. Rendering individual native widgets into cached canvas images. LayoutCanvas now
   supports this on macOS, Windows and GTK4 through local OS snapshot adapters.
   These previews preserve immediate rearrangement and source-constructor isolation,
   but use literal geometry and representative data; they do not run the runtime
   constraint solver. Buoy still has no general public UIControl snapshot API.
2. Calling `tools/buoy_preview.py` or the PreviewKit MCP server. These give exact
   pixels, but they need Python, ImageMagick and X11 tools (or the macOS Test Controls
   app, or the Windows VM), and they belong to CI and agent workflows.
3. Building and running a generated harness in-process with ProcessKit.
   **Chosen.**

**View > Preview in Native Runtime** (Cmd/Ctrl+R, or the **Preview** toolbar button)
uses `StudioPreview.bui`:

- **Stage.** It creates a private folder under the temporary directory and copies the
  `--project` folder's top-level `.bui` files and its `packages/*.buipkg` into it. If
  Studio has no `--project` folder, it uses the opened file's folder instead. Every
  registered `.bui` buffer is then written over those files, including unsaved text.
- **Generate.** It writes `_studio_preview_main.bui`. For a Window root, the harness
  subclasses the window and quits when it closes. For a View root, a generated
  window hosts the view at its designed size.
- **Check and build.** It runs `buoy <harness> --check --diagnostic-format json`, then
  `buoy <harness> --app-type desktop -o preview`. Studio finds the compiler from
  `BUOY_STUDIO_COMPILER`, then `BUOY`, then `PATH`, then the standard install
  locations. Compiler errors appear in Messages as `file:line:col: message`.
- **Run and stop.** The preview runs as a child process in its own process tree. Stop
  Preview, a new preview, or closing Studio kills the tree. The staging folder is
  deleted when the preview ends.
- **Progress.** Each preview has a ProgressKit tree (`StudioPreviewJob.Progress`) with
  four leaves: checking (including staging), building, launching and inspecting. Every
  leaf ends succeeded, failed or cancelled, including phases that were skipped, so a
  finished snapshot means that the pipeline has settled. Inspecting succeeds once a usable
  bounds report has been read (root size plus one observation for each expected control),
  even if the preview window stays open. Stop Preview only *requests*
  a stop: the job is `stopping` until the child has been reaped and staging removed,
  then reports `stopped`. If the staging folder cannot be removed, the job ends `failed`
  with `CleanupError` set, and the folder is kept in `RetainedFolders` across later
  previews. A preview that already succeeded stays succeeded. On Linux the
  build links Studio's bundled `studio-gtk-lifecycle.o`; if it is missing, the preview
  fails and asks you to rebuild Studio.
- **Compare geometry.** About 0.7 s after the window opens, the harness writes the
  runtime bounds of every named control. Studio compares them with the designer's
  literal frames. Messages then reports either that they match, or each difference,
  e.g. `nameField: designer (112, 54, 280, 24) · native (112, 54, 280, 28)`. This is
  where constraint-driven layout and native minimum sizes show up.

Every step is an explicit argv with no shell, polled from a UI timer, so Studio never
blocks. The previewed root must be `Public`, because the harness imports it. Roots
need a parameterless constructor. Code-behind subclasses are not used: the preview
shows the layout class itself. For pixel captures with typed fixture data, use
`tools/buoy_preview.py` (see `tools/preview/README.md`).

## Verification


Maintainers run the private validation suite before release. See the repository README for public build instructions.

## Code, Problems, debugger and terminal

Source mode uses a native-input canvas editor with lexical highlighting, line
numbers, Unicode-aware caret movement, smart block indentation, bracket matching
and workspace undo/redo. **Edit > Find and Replace** searches or replaces the
current source; Replace All is one undo step. Selections and scroll positions
persist while switching documents. **Build > Check Project**
checks a private snapshot of every registered source (including unsaved text)
without running it. **Show Problems** lists compiler errors and warnings; selecting
a row opens its file and positions the editor. Matching buffer revisions also
show inline markers; edits invalidate old results. Compiler exit status alone never
counts as a successful check.

The source editor supports VS Code-style **multiple cursors**: Alt+click (Option+click
on macOS) adds or removes a cursor, Primary+D adds the next occurrence of the word or
selection (Primary+K Primary+D skips one), Primary+Shift+L selects every occurrence,
Ctrl+Alt+Up/Down (Option+Cmd+Up/Down) add a cursor above/below, Ctrl+Shift+Alt+arrows
(Shift+Option+Cmd+arrows) or Shift+Alt+drag (Option+drag; a middle-button drag on
Linux) make a column selection, Shift+Alt+I puts a cursor at the end of every selected
line and Escape returns to one cursor. Typing, deletion, Enter, Tab, paste (one
clipboard line per cursor when the counts match), comments and line moves apply at
every cursor as one undo step. Shift+Alt+Up/Down copy lines up/down. Dragging the
selection moves it (hold Option on macOS or Ctrl elsewhere to copy); text and files
dropped from other applications are inserted, and the selection can be dragged out.
Dragging near an edge of the editor auto-scrolls, and Escape cancels an in-editor
drag. During an in-editor drag, changing the copy modifier refreshes the drop
indicator and cursor without moving the mouse (verified on GTK/X11; native GTK
requires pointer motion). Primary+] / Primary+[ always indent or outdent the lines
under every cursor and selection, including empty lines with a bare caret; empty
lines inside a multiline selection stay empty. Tab at a bare caret still inserts
spaces.

**View > Editor > Cursor** picks the caret style (Line, Thin Line, Block, Block
Outline, Underline, Thin Underline) and Blink. Carets span the full line height; a
block caret redraws its character in the background colour and becomes an outline
when the editor loses focus. Blinking repaints only the caret cells over a cached
frame, restarts on every caret move or edit, and stops (caret shown) after about 10 s
without changes. Both settings persist (`editor.cursorStyle`, `editor.cursorBlink`).
The full key list is in
[containers/integrations/textinput/README.md](https://github.com/BuoyBay/native-integrations/blob/main/containers/integrations/textinput/README.md#multi-cursor-and-drag-and-drop-studiocodeeditor).

**Build > Show Terminal** opens a genuine PTY shell on macOS/Linux or ConPTY on
Windows in the project directory. Input, Ctrl-C, resizing and ANSI output are
interactive. Closing the window hides it; Studio shutdown cleans up the shell
and its jobs. The compact terminal currently has no scrollback or mouse reporting
and assumes one codepoint per cell.

**Build > Show Debugger** opens the DAP debugger for explicit launch or attach,
source breakpoints, Continue/Pause, Step In/Over/Out, stack/frame inspection,
locals and output. Compile a target with `buoy FILE -g -O0 -o EXECUTABLE` to provide
DWARF; choose that executable in the debugger. It requires Python 3 and an installed
`buoy-dap`/`lldb-dap` (or `BUOY_STUDIO_DEBUG_ADAPTER`); missing tools are reported.
Stopped stack frames and Inspect navigate registered project sources and show an
execution arrow. Continue, disconnect or editing that source clears the arrow.
Unknown adapter paths are reported without opening files. Breakpoint entries are
applied on Launch/Attach. See [debugger details](docs/DEBUGGER.md).

## Tests


Maintainers run the private validation suite before release. See the repository README for public build instructions.

## Documentation library

Open **Help > Documentation** to browse the bundled upstream Buoy reference,
tutorials, course, and examples together with documentation from installed project
packages. On macOS the Help menu includes the system Help search field
(Command-Shift-/), registered through `desktopmenus` the same way Calls and
Permissions do. The reader owns appearance and navigation: package content uses the
same light/dark theme, breadcrumb hierarchy, heading outline, search, code styling,
and copy controls as upstream content. Local documentation images remain available
offline. Opening the window reindexes installed package docs; use the reader's
status-bar refresh control or its Help menu after installing, updating, or
removing packages.

Packages author Markdown and semantic front matter under
`resources/buoy-docs/`. Package names and versions come from the sealed archive's
manifest. See the [authoring and static website export guide](https://github.com/BuoyBay/devtools/blob/main/tools/documentation/README.md)
for the format and commands. Studio ships its upstream snapshot and discovery
helper; package indexing needs Python 3, as package control discovery already does.
The bundled upstream library can be read without package indexing.

## Repository tickets

**Repository > GitHub / Forgejo Tickets** opens a native issue browser. Configure
`BUOY_STUDIO_TICKETS_FORGE` (`github` or `forgejo`), `_API`, `_OWNER`, `_REPO`, and
`_TOKEN`. Opening the window makes no request: Refresh, pagination, create,
update and comment are explicit actions. HTTP work runs in the background and
credentials are excluded from displayed errors and attached context.

**Attach to Agent** registers the selected issue as an in-memory project item
alongside the current source context. Agents can read that snapshot with the
normal project tools. Editing its local buffer does not publish changes to the
forge; repository writes remain explicit Tickets actions.


Studio’s Find and Replace panel supports PCRE2 Grep in the current document:
lookaround, named and numbered captures, multiline anchors, dot-all and optional
wrapping. Next/Previous and the editor’s find shortcuts reuse the regex options;
matches highlight across lines. Preview lists up to 50 substitutions without
changing source. Replace & Find changes the selected match; Replace All is one
undoable edit through the authoritative source buffer.

In Grep replacement text, use `$1` or `\1` for numbered captures, `${name}` for
named captures, `$0` for the whole match, `$$` for a dollar, `\n`/`\r`/`\t` for
whitespace, `\U…\E`/`\L…\E` for case conversion, and `\u`/`\l` for the next
character. Outside Grep mode, replacement text stays literal. Invalid patterns,
unknown captures and excessive regex backtracking report errors before editing.
Regex matching is capped at 100,000 results; byte-splitting `\C` is disabled to
preserve Unicode editor positions. The toolchain’s `regex-1.0.0.buipkg` must be
available when building Studio (the build helper stages it automatically).

The project navigator provides a context menu for the clicked file or layout control.
Files offer reference-aware Rename/Move, Duplicate, Save, Save As, and Close;
layout controls offer Rename, Duplicate, Cut, Copy, Paste, and Delete with the
same availability rules as the designer. Duplicate File also appears in File
and the command palette. It copies unsaved text to a uniquely named sibling;
untitled copies stay unsaved. Rescan the project after adding a source copy.

### Source diagnostic gutter

The diagnostic lane uses shapes as well as colours: an error is a red octagon
with an X, a warning an amber triangle, information a blue circled i, and a hint
an outlined light bulb. A line with several problems shows its most severe badge;
a collapsed fold collects its hidden problems at the header. Click the badge to
unfold and select the highest-severity problem and inspect every full message in
a read-only window. Next/Previous Problem commands remain available.

The breakpoint lane is separate from diagnostics and folding. Existing breakpoint
toggles and the execution arrow remain available to debugger hosts; clicking a
problem badge never toggles a breakpoint. LSP information and hint messages retain
their own severities instead of being reported as warnings.


### BuoyScript library, playground and build events

**Scripts → Library / Playground / Console** provides a dedicated BuoyScript
workspace using Studio's source editor, editor settings and native controls.
Save UTF-8 function bodies in the Studio-wide library or the project's committed
`studio-scripts/*.buoyscript` directory. Open, save a copy under a new name,
import/export, delete, and bind/unbind project tasks from the same window.
Project task references live in `studio-scripts/tasks.buoytasks`, so moving a
project preserves its own automation. The Scripts menu gives quick access to
saved scripts and named tasks.

The playground runs the current buffer or a one-line expression in a fresh
helper process. Its console receives Print output, return values and errors;
Previous recalls the last 30 submissions with their evaluation modes. Stop and
the 30-second timeout terminate the helper. Hiding the window preserves drafts
and console text. Body-mode language assistance reuses the installed Buoy LSP
for supported diagnostics, completion, signature help, type information,
symbols, definitions and range formatting in an isolated virtual document.

Bind the reserved tasks `before-build` and `after-build`, then explicitly enable
build events for the Studio session. Before-build completes before the compiler
snapshot; failure or pending edit proposals abort the build. After-build runs
after a successful check/compile and before program launch; failure or pending
proposals prevent launch. Events start disabled, reset on project switch, and
never execute on open or import. Named tasks can also be run explicitly.

**Scripts → BuoyScript Help & Examples** opens the shared
[BuoyScript reference](docs/BUOYSCRIPT.md), including runner setup. **Verify
Runner** distinguishes a present helper from one that has passed a native probe.
The editor and library remain usable while the runner is unavailable.

### Agent scripting and undo

Agent edit proposals use the editor's grouped undo/redo history, including
multi-file changes. Enable **View → Auto-Allow Agent Edits (Undoable)** to apply
new proposals without review in the current workspace. Turn it off to resume
review. Revision checks still apply; Undo and Redo remain available. The setting
does not save files or approve older pending proposals.

The optional BuoyScript runner lets agents chain project tools and transform
unsaved text. The [BuoyScript primer](docs/BUOYSCRIPT.md) is included in agent
context, available through `project_script_help`, and searchable in Studio Help.
BuoyScript is enabled by default. **File → Settings / Preferences…** saves the
scripting toggle and an optional runner path. Builds bundle the helper when the
selected SDK provides a native host runtime; Studio discovers it automatically.
For a separately built helper (`tools/build_studio_script_runner.py`), select its
absolute path in preferences or set `BUOY_STUDIO_SCRIPT_RUNNER`. The runner needs a
compatible native BuoyScript runtime; Studio itself does not link LLVM.

Navigator file management also supports New Folder, Copy Path, Reveal in File
Manager, Delete File, and Restore Last Deleted File from the File menu and the
appropriate row menus. Folder rows include empty folders. Delete first resolves
unsaved edits and asks for confirmation, then keeps exact saved contents in the
project's private `.buoy/deleted` store. Restore Last Deleted File is available
for deletions made during the current session. A deletion that prevents the
project from reopening (such as removing an imported module) restores the file
and reports the problem. Unrelated unsaved text and independent undo/redo history survive deletion and restore.

With focus in the navigator, F2 renames a file, folder, or control (Return also
renames on macOS), and Primary+D duplicates a file or control. Shift+F10 or the
Menu key opens the selected row's context menu. Source editor shortcuts keep
their editor behavior when the editor has focus. File rename/move undo and redo
check saved bytes for every moved file and refuse external changes.

## Agent inline completion

Choose an autocomplete provider in Settings / Preferences. With the Agent provider,
Studio uses Chat's selected configured local agent or signed-in ChatGPT provider.
In a source editor, **Alt+\** (Option+\ on macOS) requests ghost text.
**Alt+Shift+\** toggles automatic requests after a roughly 600 ms typing pause.
Automatic requests default to disabled and the preference is saved. Use a single
caret anywhere in a line, including inside arguments and expressions. Generated text
that overlaps the existing suffix is trimmed, preserving closing brackets and quotes.
**Tab** accepts the previewed edit as one undoable change; **Escape** dismisses it,
including a request still in flight. Typing, selection changes, focus loss and IME
composition invalidate pending replies. Standard language completion stays available.

Agent-provider requests use a separate session with up to 6,000 preceding and 2,000 following
characters of the current unsaved buffer. This context goes to the selected provider.
Chat history stays separate. Each request has a roughly nine-second polling deadline;
failed, oversized or Markdown-wrapped responses are discarded. Multiline suggestions
and replacements show an explicit before/after preview. Long rows wrap; PageUp and
PageDown review the complete preview without moving the source caret.

The Agent provider can return up to 16 ordered, disjoint edits in the active file.
Each replacement must match the exact old source. Tab accepts only the current edit;
when the next edit is elsewhere, Tab first jumps there, then another Tab accepts it.
Later positions move with accepted edits. Each acceptance has its own undo step;
ordinary typing, navigation, undo or Escape clears the remaining sequence. Copilot
replacement ranges are preserved and previewed, including UTF-16 positions. HTTP
fill-in-the-middle providers continue to supply insertions.
The completion bridge has an empty project workspace and cancels permission requests.
It does not grant agent tool permissions. Grok/Muse HTTP providers and unconfigured
MCP prompt contracts are not wired to this completion path.
