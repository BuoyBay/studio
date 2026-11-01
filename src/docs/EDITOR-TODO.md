# Studio source editor: outstanding items

Open work on the source editor (`StudioCodeEditor.bui`) and the features around it:
language server, docs lookup, keyboard, folding, multi-cursor and drag and drop.
Last updated 2026-11-01. Done items are deleted, not ticked.

## 1. Before committing (repository state)

- [ ] **The working copy is behind HEAD in places.** Several Studio sessions edit the
  same files, and snapshots get committed automatically.
  - `StudioLanguageServer.bui` drops HEAD's rename staleness and overlap checks and
    its response-id validation (commit `a7468029`), and deletes that commit's three
    protocol-test files.
  - `documentation.html` and `tests/documentation_viewer_test.cjs` lack the
    package-aware, ambiguity-rejecting `definitionTarget` logic and tests from
    `344ab93c`.
  - `StudioDefinitionHost.bui` (from `db0b01e2`) is deleted in the working copy. It is
    probably replaced by `StudioLanguageController.bui`.
  - The session that owns `StudioLanguageController` should rebase onto HEAD. It also
    needs to carry over the client review fixes:
    - A write to a dead `buoy-lsp` raised an uncaught `ProcessException: Broken pipe`
      from the Timer.
    - `Poll`'s error path relaunched the server from inside a catch.
    - A `Stop()` during a restart brought the server back.
    - Requests re-issued while stopping were not rejected.
- [ ] **processkit native slices are git-ignored** (`libraries/processkit/.gitignore`
  ignores `/runtime/` and `/link/`). Add them with `git add -f`:
  - `runtime/aarch64-pc-windows-msvc/buoy_processkit.dll` and `.source-sha256`
  - `link/aarch64-pc-windows-msvc/buoy_processkit.lib`
  - the untracked Linux ARM64 slice
- [ ] **The x86_64 Linux `.so` in processkit** no longer matches
  `validation/arm64-bridge-published.json`. Another session changed it; confirm it
  is intended, then refresh the receipt.

## 2. Saved folds across restarts (built 2026-11-01)

Studio already saved open tabs' folds (line numbers only) in the workspace session,
`ApplicationData()/BuoyStudio/workspaces/<StudioShellStateKey>/studio-documents-v1.json`,
written atomically by `StudioDocumentStore`. The new work extends that file instead of
adding a second one:

- `StudioEditorViewState.FoldHeaders`: the trimmed text of each folded header line.
  `CaptureViewState` fills it; `RestoreViewState` passes the pairs through
  `AnchorFolds`, which uses the saved line if its text still matches, else the
  nearest unclaimed matching line within ±50 (`FoldAnchorReach`). Exact matches are
  placed first, so repeated headers (`Try`, `Get`) keep their own blocks. Unmatched
  folds are dropped, and `SetFoldedLines` still skips lines that no longer start a
  region.
- Session JSON: each view gains `foldHeaders`; the session gains `closedFolds`, a
  list of `{ path, folded, foldHeaders }` for closed disk files (500 most recent,
  entries for deleted files dropped on load).
- `main.bui`: `CloseItems` remembers folds from the pane last used for the file (the
  split pane counts) and clears them from the in-session views, which keep caret and
  scroll. `SelectItemId` merges them back when the file reopens.
  `CheckpointDocuments`/`OpenDocumentStore` write and read them. Turning "Remember
  Folds" off clears the remembered set.
- View ▸ Editor: "Remember Folds" (on; off writes no folds) and "Fold Comments on
  Open" (off; collapses comment blocks when a file opens with nothing remembered).
- Verified: Studio check-only build (all targets the script checks; run on a private
  copy because another session's `LayoutEditor.FinishInspectorEdit`/`FocusCanvas`
  calls don't match the sealed layoutcanvas package yet), editor suites in
  `buoy:0.72.0-desktop` (371 checks, including moved, changed and repeated headers).
- [ ] Not yet covered: a Studio self-test for close/reopen and restart; remembered
  folds are dropped if a file is opened and closed while Code Folding is off.

## 3. Keyboard

- **User keybindings: no separate file needed.** Studio already has Navigate ▸
  Customize Keybindings (`StudioCommands`, saved in Prefs as
  `navigation.binding.<id>`). Every editor command in `StudioEditorCommandList` is
  registered there as `source:<id>`, and host bindings are resolved before the
  editor's own `KeyCommand` table, so a user binding overrides a default.
  - Fixed 2026-11-01: the catalog's shortcut hints become host bindings, and they
    still had the Windows chords on Linux (Shift+Alt+↑ was bound to Copy Lines Up,
    overriding the new Linux add-cursor key). They are now per-OS.
  - [ ] Unbinding a default, so the desktop gets the key, is not supported. On Linux
    the desktop grabs its own keys first anyway.
  - [ ] Duplicate-chord warnings exist (`Bind` rejects conflicts), but there is no
    listing of the editor's built-in `KeyCommand` defaults; the bindings window shows
    only host bindings.
- [ ] **No keyboard column select up/down on Linux.** Ctrl+Shift+Alt+↑/↓ is copy
  lines there, as in VS Code. Shift+Alt-drag and add-cursor cover it. Bind
  "Column Select Up/Down" in Customize Keybindings if wanted.
- [ ] `undoCursor` has no default key, because Primary+U is Run All Tests.
- [ ] **Unbound:**
  - Windows Alt+Backspace (undo)
  - Emacs mark commands
  - Word case changes
  - GTK Insert (overwrite mode)
- [ ] Two consecutive Ctrl+K kills undo as one step, because kills are recorded as
  ordinary deletes.
- [ ] `MoveParagraph` moves the caret in two steps, so `SelectionChanged` fires
  twice.

## 3a. Caret, blinking and drag and drop (2026-11-01 follow-ups)

Built and verified in `buoy:0.72.0-desktop` (Linux): caret styles, full-height
carets, block-caret glyphs, the blink frame cache, edge auto-scroll, the drop
indicator, Escape cancel, and moves between split panes. Ctrl+]/Ctrl+[ were also
verified with real keys for empty-line carets, mixed multiple carets, and multiline
selections spanning empty lines; outdent restores text and positions. In-editor
copy-modifier refresh was verified without pointer motion on GTK/X11. Still open:

- [ ] **Windows: the test window never received input.** A cross-compiled window
  with a `StudioCodeEditor` rendered correctly in the `containers/windows` VM (block
  caret, CJK, emoji, combining marks). Clicks, keys and drags injected through the VNC
  bridge didn't reach it, while the same clicks worked on the Run dialog.
  - Unknown whether this is the editor, the test window (Studio calls `Open()` from
    its own startup), or the VM. Compare against a build without this session's
    changes before trusting Windows input.
  - The bridge's `drag` takes 200 ms, too fast for OLE `DoDragDrop`. A slower
    40-step drag through `control.connection()` also did nothing.
  - The VM desktop is shared by several sessions. Check it is idle first, and
    launch through a `.cmd` on the share rather than typing long commands.
- [ ] **Never run on macOS:**
  - caret layer and frame cache on Retina (the picture is built at the window's
    `ScaleFactor`)
  - blink and auto-scroll timers
  - native drag with edge scroll
  - `ShowsDragImage`. Enable it only after a drag still drops.
- [ ] **Fractional scaling (Windows 125%/150%):** the cached frame is rounded to
  whole pixels and drawn back at the logical size. Glyphs next to the caret may change
  weight on the first blink, and offscreen text may lose ClearType. If they do, round
  the cache to the device grid or skip the cache on fractional scales.
- [ ] **Remaining blink cost is the toolkit's.** GTK4 redraws the whole widget on
  every paint, so blinking still adds ~21 CPU ticks per 8 s versus ~40 without the
  cache (idle ~9, 2,000-line file). Options:
  - stop blinking after a shorter idle time
  - follow the system blink rate (Windows `GetCaretBlinkTime`, which can disable
    blinking; GTK `gtk-cursor-blink-time`)
- [ ] **Moving text between split panes is two undo steps** (an insert in one pane,
  a delete in the other). The host's sync also collapses the dropped selection to a
  caret in the receiving pane. Undo belongs to the host, not either editor:
  `StudioCodeEditor` raises `UndoRequested`; `main.bui:EditorChangedFrom` calls
  `Workspace.ReplaceText` for each edit and synchronizes the other pane through
  its `Text` setter with a captured/restored view state. A single undo requires
  a host transaction spanning target insertion and source deletion (including
  cancellation and document identity checks), or one atomic document replacement
  for same-document moves followed by explicit view-state updates. An editor-only
  undo group cannot combine these host history records.
- [ ] **Native GTK drag: stationary copy-modifier changes.** On Buoy 0.72 under
  Xvfb, Ctrl down/up reaches Canvas key handlers in mode 10, but produces no
  `NativeDragOver` callback. The native drop indicator/action therefore waits for
  pointer motion; its cursor is toolkit-owned. Mode 8 (in-editor fallback) now
  refreshes immediately on both transitions, with Escape cancellation verified.
  Wayland modifier-release timing remains unverified: `Keyboard.IsKeyDown` uses
  GDK modifier state there rather than X11's physical-key query.
- [ ] **Possible caret options:**
  - smooth (fading) blink
  - line width in pixels (VS Code `cursorWidth`)
  - a separate shape for an overwrite mode, if one is added
- [ ] **Testing:** host builds link with `-L native-link` (an unversioned
  `libgtk-4.so` link). The `buoy:0.72.0-desktop` linker can't find packaged
  `libpcre2-8.so.0` even with `-L`. Run the binaries in the container.
  - Under Xvfb a GTK native drag needs `xdotool` relative moves with about 40 ms
    pauses.
  - Studio's `Print` output needs `stdbuf -oL` to survive `kill`.

## 4. Folding and minimap

- [ ] **Replace All expands too much.** It expands every collapsed fold between the
  first and last match, and every fold in regex mode, even ones with no match inside.
  The fix is to apply replacements per range instead of as one span edit.
- [ ] **Moving a line down onto a collapsed header** swaps it with the header only,
  and that fold expands.
- [ ] **Duplicating a collapsed block** expands the original, and the copy is
  unfolded.
- [ ] **Changing the indent width** can stretch an indentation-based fold over the
  caret without expanding it.
- [ ] **Region recalculation is a full pass**, about 7 ms per edit at 10k lines while
  any fold is collapsed. Make it incremental.
- [ ] **The minimap is drawn at 1x**, so it looks soft on Retina. Drawing it at 2x
  made every paint 4–5× slower. Try a 2x cache that is redrawn only on change.

## 5. Rendering and settings

- [ ] **Ligatures off only splits operator characters in code.** Strings and
  comments still draw whole, so a ligature font still joins `=>` inside them.
- [ ] **The Font menu is a fixed list of candidates** filtered by fontkit's installed
  families. Consider an "Other…" entry that lists all installed monospaced families.
  The full font scan currently runs on the UI thread when the menu is built.

## 6. Language server and documentation

- [ ] Find in Project from the context menu is a status-message stub. Connect it to
  `StudioProjectSearchWindow`, which another session is building.
- [ ] **Client robustness:**
  - Re-entrancy: a listener that opens a modal or calls `Poll` from a callback can
    re-enter message parsing.
  - No timeout for `initialize`, or for requests not yet sent.
  - Request positions are converted when queued, so they go stale if typing
    continues.
  - Full-text `didChange` messages are not coalesced.
- [ ] **`buoy-lsp` itself:**
  - It never publishes diagnostics for files with non-ASCII identifiers, so requests
    wait out the 5 s analysis timeout.
  - Columns are wrong on lines containing emoji.
  - Diagnostics carry no version.
  - References return read sites only, as zero-width ranges.
  - Definitions are line-level.
- [ ] The native Studio `--documentation-test` path hasn't been run with the topic
  lookup.
- [ ] `tests/documentation_viewer_test.cjs` fails in an unrelated package audit
  (`libraries/fieldkit` has no documentation index).

## 7. Never run on these platforms yet (compile-checked only)

- **macOS:**
  - AppKit selector mapping (`MacSelectorCommand`)
  - the NSEvent monitor shortcuts
  - native drag and drop
  - `buoy-lsp` path resolution under `/Applications/Buoy`
  - Retina rendering of text, selections and the minimap
- **Windows:**
  - `WindowsKeyCommand` and the Studio key hook
  - swallowing the stray menu mnemonic after Shift+Alt+I
  - native drag and drop
  - the language-server crash/restart path
- **Windows ARM64:** the processkit DLL has only been linked against. Run the
  processkit native pipe test on a Windows-on-ARM machine (Surface, or Parallels on
  Apple Silicon).
- **Test runner:** `tests/run_studio_code_editor_native_test.py` can't run on a
  Linux host as written. Host binaries need `libjpeg.so.8`, and `xvfb-run` can hang
  in the image. Give it a container mode that builds on the host and runs the
  binaries in `buoy:0.72.0-desktop` with Xvfb started directly.

## Platform notes worth keeping

- **Linux drag images:** on Buoy 0.72 Linux, a non-Nil `DragImage` breaks native
  drags, so the editor keeps `ShowsDragImage` off. Even a 40x20 solid `Picture` (1x
  or 2x) stops `DragOver`/`DragDropped` from arriving; `Nil` works.
- **`Canvas.Invalidate()` is virtual:** an `Overrides Sub Invalidate()` sees calls
  made through a `Canvas` reference (checked in the container). The editor's blink
  cache depends on this. `InvalidateRect` isn't overridden, and GTK4 repaints the
  whole widget for it anyway.
- **Array literals are fixed-size:** `Dim a As Array Of String = ["x"]` can't be
  appended to. Start from `[]` and `Array.Append`.
