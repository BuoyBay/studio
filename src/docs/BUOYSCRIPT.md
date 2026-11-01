# BuoyScript — Studio scripts, playground and automation

Open **Scripts → Library / Playground / Console** to write and run BuoyScript
in the IDE. Keep automation in a Studio-wide library or in the project itself,
reference saved project scripts as tasks, and attach tasks to build events.
**Scripts → BuoyScript Help & Examples** opens this reference in Studio's
documentation viewer. Editing, saving and importing remain available without a
configured runner.

## Agent access and shared project tools

BuoyScript is **enabled by default**. Open **File → Settings / Preferences…**
to turn it off or choose a matching runner executable. The preference applies to
both agents and the playground and is saved between launches. Disabling scripting
stops the active script; existing project proposals and edits remain reviewable
and undoable.

Use `project_script_help {}` first. It reports live enablement and runner status
and works even when the runner is unavailable. The script tools remain advertised
so an agent can diagnose setup rather than confusing BuoyScript with `.bui`
application files or substituting compiler checks.
`project_script` runs a **function body returning String** in a fresh native JIT
helper. Do not include `Import`, `Sub Main`, `Export`, or an entry declaration.
BuoyScript v1 has no imports. Use the selected toolchain's grammar and stdlib.

```buoy
Dim raw As String = ProjectTool("project_list_items", "{}")
Dim reply As JSONItem = Json.Parse(raw)
If reply.HasKey("error") Then Return raw
Return reply.Value("result").Value("content").ValueAt(0).Value("text").AsString().Or("")
```

Send the body as the JSON `source` string in `project_script`. It returns a
`scriptId` immediately. Poll `project_script_result {"scriptId":"..."}` until
`complete` is true; inspect `ok`, `value`, and `error`. Use
`project_script_cancel` with the same ID to stop. Only one script helper may run
at a time across the playground, build events and agent tools.
A reported result remains final even if helper teardown fails; `cleanupWarning`
describes cleanup problems without changing `ok` or the returned value. Continue
polling until `complete` so owned handles have been released. Closing an agent
session cancels its script and detaches bounded cleanup, allowing a new session.

`ProjectTool(name As String, argumentsJson As String) As String` returns the
**full JSON-RPC response**, containing either `error`, or an MCP `result` with
`isError` and `content[0].text`. The latter text is itself JSON for the tool's
payload. Check both error layers before using data. Each call waits for its
response, so later calls can use earlier results. Supported commands:

- `project_workspace`, `project_list_items`, `project_read_item`
- `project_search`, `project_propose_edits`, `project_proposal_status`

Call the ordinary tools directly to discover their schemas. Scripts cannot
recursively start scripts or call arbitrary extension tools. Run `project_check`
after applied edits, outside the script; a pending proposal is not compiler input.

## Edit an authoritative unsaved buffer

Use the exact `version` from `project_read_item`. All edit offsets are UTF-8
**bytes**, zero-based, with an exclusive end. `String.LenBytes` gives byte length.
Pass ordinary JSON objects to `Json.Stringify` rather than hand-escaping text.
This example replaces text in a small item whose ID is `main` (use list to find
IDs). For large buffers, use `startLine`/`maxLines` ranges and the returned absolute
byte offsets; a response over 1 MiB fails the script. U+0000 is not supported in
project text. `project_workspace.execution = "unavailable"` describes the core
project tools; it does not disable a separately configured script runner.

```buoy
Dim raw As String = ProjectTool("project_read_item", "{\"itemId\":\"main\"}")
Dim response As JSONItem = Json.Parse(raw)
If response.HasKey("error") Then Return raw
Dim toolResult As JSONItem = response.Value("result")
If toolResult.Value("isError").AsBoolean().Or(False) Then Return raw
Dim item As JSONItem = Json.Parse(toolResult.Value("content").ValueAt(0).Value("text").AsString().Or("{}"))
Dim oldText As String = item.Value("text").AsString().Or("")
Dim edit As JSONItem = JSONItem.NewObject()
edit.Value("startByte") = 0
edit.Value("endByte") = String.LenBytes(oldText)
edit.Value("text") = String.Replace(oldText, "oldName", "newName", True)
Dim edits As JSONItem = JSONItem.NewArray()
edits.Add(edit)
Dim change As JSONItem = JSONItem.NewObject()
change.Value("itemId") = "main"
change.Value("expectedVersion") = item.Value("version")
change.Value("edits") = edits
Dim changes As JSONItem = JSONItem.NewArray()
changes.Add(change)
Dim input As JSONItem = JSONItem.NewObject()
input.Value("changes") = changes
Return ProjectTool("project_propose_edits", input.ToString())
```

Edits enter the **same grouped undo/redo history** as user-approved changes.
A multi-item proposal is one undo group. Default policy leaves proposals pending.
The user can enable **View → Auto-Allow Agent Edits (Undoable)** for the current
workspace; `project_workspace.autoAllowAgentEdits` reports the policy. The agent
cannot change it. Auto-allow applies new proposals using the same revision gates
and approval function. Undo/redo advances revisions: re-read before another edit.
No automatic disk save occurs. Disabling auto-allow affects future proposals;
pending proposals still need review. Turning it on does not approve old proposals.

## Errors and limits

- Compile error: source is a function body, must return String, uses `+` for
  concatenation, and `\"` for quotes. Reported source lines include the four-line
  generated bridge/function prefix. Read current toolchain docs for stdlib details.
- Stale revision: re-read the item and recalculate edits. Never retry old offsets.
- Pending proposal: return its ID and wait for user review; do not busy-loop.
- Runner exited/crashed: the generated entry catches ordinary script exceptions; native
  faults can still abort the helper. Studio reports the failed run.
- In the legacy agent protocol, avoid `Print`/`StdOut`: stdout carries the
  bridge protocol. The IDE console mode supports Print and reserves stderr for
  protocol messages; avoid writing `StdErr` there.
- Source ≤64 KiB, 128 callbacks, 1 MiB per protocol line/response, 2 MiB total
  helper output, timeout 30 seconds by default (`timeoutMs`: 100–60000).
  The agent API deliberately caps each run at 60 seconds to bound unattended
  use of the single shared helper. Split longer agent work into separate runs.
  The user-controlled playground and build events allow up to one hour; this
  does not raise the agent API limit. The helper is not a security sandbox.
- Cancellation, timeout, or failure does **not** roll back earlier proposals or
  applied edits. Inspect state; Undo restores applied changes.

## Setup and trust

Studio builds bundle the native helper automatically when the selected Buoy SDK
includes a runtime for the build host. Studio discovers that bundled helper at
launch, without an environment variable. Unsupported SDK targets report the
missing runner in settings and `project_script_help`.

To build a helper separately, use
`python3 tools/build_studio_script_runner.py --output /absolute/path/studio-script-runner`.
Use the same Buoy SDK and libbuoyscript runtime version. Select the absolute path
in **Settings / Preferences**, or set `BUOY_STUDIO_SCRIPT_RUNNER` before starting
Studio. A saved runner path takes precedence over the environment override. The helper
and its native libraries must remain together as deployed by the compiler.
The base Studio binary does not link LLVM. The builder checks target runtime
availability and reports missing platforms before linking. Rebuild the helper
when updating this integration so it understands IDE console mode and the
current result callbacks. In the Scripts window, **Verify Runner** performs an
explicit native probe. The status distinguishes a missing configuration, an
unavailable path, a present but unverified helper, and a successful probe. A
configured path or compiler check alone does not establish JIT execution.

BuoyScript compiles native code with the user's OS privileges. The helper contains
crashes and can be terminated; it is **not a security sandbox**. Scripts should
use `ProjectTool` for all project text changes so edits remain visible and undoable.
Direct native filesystem writes bypass the editor and its history. Enabling
scripting is separate from enabling auto-allow edits.

The Buoy 0.72.0 archive installed on the development Linux machine contains only
`runtime/arm64-apple-macos/libbuoyscript.dylib`. Native Linux JIT execution cannot
be verified with that archive. Studio's bridge and history can be tested using a
protocol fixture; this does not establish native JIT/FFI correctness. Native
Apple Silicon macOS validation has exercised Unicode return values, Print
output, ProjectTool callbacks, compile errors and caught/uncaught script
exceptions. These results do not establish native support on other targets.

## BuoyStudio Scripts menu and playground

Open **Scripts → Library / Playground / Console** in the IDE. This uses the same
StudioCodeEditor and editor settings as project source: syntax colors, native
text input, selection, folding, editor commands and completion. The language server supplies supported
completion, signature, hover/type info, symbols, definition and formatting
features when offered by the installed Buoy language server. A virtual document
in an isolated temporary language workspace uses the runner's four-line prefix
and entry wrapper; diagnostics and edits map back to the body. It does not add a
source document to the application project. Unsupported server features report
through the status line. Language analysis uses Buoy's compiler, not a JIT session.
No virtual source file is saved. **Format** requests range formatting for the
body, keeping generated declarations outside the editable range. Expression
mode is intended for quick evaluation; language analysis is available in body
mode. Language edits validate the buffer revision and reject ranges outside the
body. Script Undo restores an applied edit group.

Choose **Project scripts** or **Studio scripts** as the save destination, enter a
portable name (letters, digits, hyphens, underscores), then **Save**. Changing a
saved script's name or destination saves a new copy, leaving the original intact.
**Open** loads a library entry; **New** creates a draft. Unsaved changes must be
saved or explicitly discarded before another draft replaces them. Closing the
Scripts window hides it and retains the draft and console. **Undo/Redo** operate
on script edits separately from project edits. **Import** opens a `.buoyscript`
file as a draft for review; **Save** adds it to the chosen library. **Export**
writes a standalone file, including an overwrite confirmed in the save dialog;
atomic conflict checks preserve externally changed files. Neither importing nor opening runs code.

Project scripts live in `studio-scripts/<name>.buoyscript`; Studio scripts live
in `BuoyStudio/scripts` under the platform application-data folder. These are
UTF-8 function bodies, separate from `.bui` application sources and Studio's
`.buoy` cache. Commit `studio-scripts/`, including `tasks.buoytasks`, with the
project. Copying a project preserves its script references without user-specific
paths. Libraries allow up to 512 scripts; project task references allow up to
512 entries within a 64 KiB manifest. Names are portable and case-insensitively
unique. Studio scripts run against the current workspace, including when no
project directory has been opened.
The Scripts menu lists project scripts, Studio scripts and project tasks (first
128 entries); use the library for the full list. Refresh Script Menu updates it.

**Run** executes the current unsaved buffer; **Run Task** executes the saved task
reference. In function-body mode the body must return String. In expression mode,
the IDE evaluates `String.From(<expression>)`; enter one expression on one line.
Each run is a fresh native process: variables do not persist across submissions.
**Previous** cycles backward through the last 30 submissions, restoring the
original source and evaluation mode. Untouched New and Previous buffers do not
require a discard confirmation. **Stop** terminates the shared active script,
including an agent-held script. Run timeout defaults to 30 seconds and build-event
timeout to 60 seconds. The two timeout fields accept 100..3600000 milliseconds
(up to one hour); agent tool calls retain their separate 60-second maximum. The console shows `Print` output, returned values and errors. Console
retention is bounded (32 KiB per run, 32K characters transcript), with an explicit
truncation marker at a valid UTF-8 boundary. Runner crash text is retained,
including a final stderr fragment without a newline. Compile errors in the console
use body line numbers; the underlying helper response retains compiler locations. The new helper
sends protocol messages on stderr in IDE console mode, leaving stdout for Print;
avoid writing StdErr in that mode. Rebuild an older helper before using this UI.
The legacy agent protocol continues to use stdout and still forbids Print.

Try this function body in the playground:

```buoy
Print "Hello from BuoyStudio"
Dim answer As Integer = 6 * 7
Print "Answer: " + String.From(answer)
Return "Finished"
```

## Project tasks and build events

Save a project script, enter a task name, and select **Bind Task**. Bindings are
project-owned relative references in `studio-scripts/tasks.buoytasks`. This file
contains a JSON object, for example:

```json
{"before-build":"prepare-assets","after-build":"report-build","release":"prepare-release"}
```

The values name scripts in the same directory, without their `.buoyscript`
extension. **Run Task** and the Scripts menu run named tasks explicitly.
Change bindings by binding a different script to the same task name. **Unbind
Task** removes the named reference; deletion refuses scripts that tasks still
use. A task always resolves inside its own project library, including when a
Studio-wide script has the same name.
Missing or invalid references fail visibly instead of falling back to user scripts.

The reserved tasks **before-build** and **after-build** run for Check Project,
Check All Targets and Compile and Run Active (including terminal runs) when
**Events On** is enabled in the Scripts window. Events are disabled initially,
require explicit session enablement, and reset when switching projects. They
never run on import, project open, background language analysis, tests or preview.
Before-build completes before Studio captures compiler input; failure or edits
awaiting review abort the build. Approve those proposals before building again. After-build runs only after successful compilation/checking and completes
before a compiled program is launched; failure or pending edit proposals prevent
launch. A failed after-build event or pending proposal after a check is reported
in Studio messages. Project revision
checks still apply: an after-build edit invalidates the compiled snapshot, so run
again. Events use saved task source. If an open bound hook has unsaved edits, save
it before building or disable build events; Studio aborts instead of silently using
the older file. Stop Run / Check
also cancels build scripts. Cancellation leaves previous project proposals or
applied edits available for review/Undo.

A build script can use the same ProjectTool bridge described above, plus Buoy
stdlib/native functionality offered by the installed BuoyScript runtime. Keep
project text edits in the bridge so they remain reviewable and undoable. A
pending proposal does not become compiler input until approved. Scripts cannot
recursively invoke build/check/run tools through ProjectTool. There is no
implicit shell expansion, shared JIT state or automatic saving of project text.
