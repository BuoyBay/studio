# Source editor maintenance notes

Studio's editor integrates source navigation, language-server requests,
documentation lookup, folding, multi-cursor editing and native text input.
See the [Studio guide](../README.md) for user-facing commands and the
[debugger guide](DEBUGGER.md) for debugging behavior.

## Preserve workspace state

Editor changes need to preserve each document's caret, selection, scroll and fold
state. Fold anchors use saved header text as well as line numbers so edits before
a region can be handled without folding an unrelated region. Keep saved-state
changes compatible with existing workspace sessions.

Review close/reopen and application restart behavior whenever changing document
checkpointing. Pay attention to split panes and closed documents as well as the
active editor.

## Keyboard and native text input

Respect platform shortcuts and user keybindings. Native desktop-reserved chords
can be intercepted before Studio receives them. Exercise conflicts, unbound
commands, selection, multi-cursor operations and the editing commands affected by
a change on the target desktop.

Keep IME composition, surrogate/UTF-16 positions, drag-and-drop and clipboard
behavior consistent with the editor's source model. A change to one input path
should preserve selection and undo behavior in the other paths.

## Language-server and documentation integration

Treat source snapshots and server responses as versioned data. Handle stopping,
restarting, broken pipes, cancellation and stale responses without applying an old
edit to a newer document. Preserve ambiguity checks when linking symbols into
package documentation.

## Extending the editor

Describe an extension's command, state and native-platform behavior before adding
it. Build the affected source and exercise the user workflow, including undo,
redo, focus changes and workspace persistence. Compiler checks alone do not
establish desktop input, accessibility or language-server interaction behavior.
