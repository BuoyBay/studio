# Native debugger

Open Studio's Debugger window, enter the path of a **built executable**, (or macOS `.app` bundle), optional
arguments as a JSON string array, and source breakpoints as absolute `file:line`
entries (one per line). Build Buoy sources with `buoy main.bui -g -O0 -o app`.
`-g` emits DWARF; Buoy's separate `--debug` enables its ARC leak detector and
`Break` statements. Merely opening a project never builds or launches it.

**Launch** starts that executable, initially stopped. **Continue**, **Pause**,
**Step Over**, **Step In** and **Step Out** send native DAP operations. The stack
shows frame IDs, source paths and lines. Enter a frame ID and choose **Inspect**
to view that frame's non-expensive scopes/locals/registers. Source breakpoints
are configured at session startup; unresolved ones are explicitly reported in
Output. Reconfigure them by stopping and starting a new session. A stop or
Inspect selection reveals its registered project source in Studio with an
execution arrow. Unknown source paths are reported without opening files; resume,
disconnect, or changes to that source clear the arrow.

**Attach** uses the entered process ID. OS debugger permissions still apply.
**Stop** disconnects and terminates a launched target; it detaches from an
attached target without requesting its termination. Closing the view hides it;
Studio shutdown closes the session. An adapter that does not acknowledge Stop
is closed after a bounded timeout, with a visible error.

The UI starts Studio's bundled `studio_debug_adapter.py` with Python 3, then that
helper starts an installed native DAP adapter with explicit argv (no shell).
Adapter responses are bounded length-framed DAP; UI traffic is bounded JSONL.
Program arguments remain individual argv strings. Reverse requests such as
`runInTerminal` are refused. The helper is app-owned; project helper scripts are
never selected implicitly.

Configuration:

- `BUOY_STUDIO_DEBUG_PROGRAM`: initial executable field.
- `BUOY_STUDIO_DEBUG_ADAPTER`: trusted path to `buoy-dap` or `lldb-dap`.
- `BUOY_STUDIO_DEBUG_HELPER`: explicit trusted override for Studio's bundled helper.
- `BUOY_STUDIO_PYTHON`: explicit Python interpreter override.

Without an explicit adapter, the helper checks installed Buoy/LLDB locations,
Homebrew LLVM, macOS `xcrun --find lldb-dap`, and normal executable discovery.
Missing Python, missing helper, missing adapter, target errors and OS attach
permission failures are shown; no mock debugging session is substituted.

Native LLDB integration follows [LLVM's lldb-dap configuration](https://lldb.llvm.org/use/lldbdap.html).
The implementation supports launch/attach, source breakpoints, stepping, pause,
stack frames, scopes/locals and output. Variable mutation, expression evaluation,
watchpoints, reverse debugging and a source-gutter breakpoint editor are outside
this implementation. Adapter availability and capabilities vary by host.

Maintainers run the private validation suite before release. See the repository README for public build instructions.

## macOS debugger authorization

Apple LLDB may report `this is a non-interactive debug session, cannot get
permission to debug processes` when macOS has not authorized debugging for the
launch context. This also occurs for an ordinary clang-built native executable;
it does not by itself indicate a Buoy compiler defect. The bridge preserves
Apple's structured error details and keeps the session failed, even if the
adapter subsequently emits a stopped event without a valid process.

Launch Studio normally in the Aqua desktop session. Any required system
permission must be granted by the user through the normal macOS authorization
flow or its developer-tool settings. Studio never changes Developer Mode,
DevToolsSecurity, group membership or privacy permissions. SSH and desktop launch
contexts must be validated separately; compiler checks do not establish native
OS debugger authorization. Native tests fail explicitly when the adapter is
installed but the host refuses debugger launch.

A launch blocked in macOS authorization can leave Apple's `debugserver` waiting
in a separate process session after its adapter exits. The bridge records only
its own adapter's descendant debugservers before teardown, then rechecks PID,
start time and executable identity before cleaning them up. It does not select
external attach targets or change any OS authorization setting.

## Source path spelling

Source breakpoints retain the absolute path spelling used when the source was
compiled. Resolving a symlink can break DWARF matching: on macOS `/tmp/...` and
`/private/tmp/...` can refer to the same file while only the compiler's original
spelling resolves a source breakpoint. The bridge therefore does not resolve
source-directory symlinks. The native regression compiles through such a symlink
and exercises actual verified source breakpoints, stepping and locals. macOS
also tests a real `.app` bundle and its compiler-produced adjacent dSYM.

## Source navigation and session revisions

Save the source and build the chosen executable before Launch. Studio snapshots
clean, saved registered-buffer revisions at the optional
`StudioDebuggerSessionListener.DebuggerSessionStarting()` callback, after launch
validation and queueing but before the helper starts. Stop and Inspect callbacks
navigate only registered project disk paths whose saved source still matches that
session's snapshot. Sources dirty at session start, changed afterward (even if
saved mid-session), or externally edited on disk refuse navigation until a fresh
debug session. Unknown adapter/DWARF source paths are not opened automatically.
This is a source-consistency gate; it does not prove the chosen executable was
rebuilt from those exact sources.

Serialized document bytes are compared with disk, preserving BOM and line-ending
metadata. Format-only unsaved changes are refused too; a clean saved CRLF source
can navigate even though the editor uses LF internally.

## Stop and adapter backpressure

Stop immediately clears the window's stack, locals and execution location, enters
Stopping, and refuses further stepping/Inspect commands. The bridge latches Stop
before queuing Disconnect, invalidates request generations and ignores late
initialize/configuration/stopped/stack/locals results. Such results cannot launch
a target after Stop or revive a stopped source highlight.

Adapter writes use one asynchronous worker and a bounded one-MiB queue, including
the in-flight frame. Disconnect has a reserved four-KiB control budget and removes
unsent work while preserving any partially written DAP frame. A native adapter
that stops reading cannot block the bridge event loop, startup deadline or Stop.
Teardown queues Disconnect as a best effort; it never synchronously writes or
flushes a blocked adapter pipe. Owned nonreading-adapter regressions exercise
valid approximately 60-KiB breakpoint requests, deadline expiry and cancellation.
The normal native Attach/Detach regression separately verifies its target survives.
