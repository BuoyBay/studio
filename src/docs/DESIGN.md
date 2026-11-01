# Buoy Studio design

Studio follows the classic REALbasic/Xojo IDE arrangement, built from native
Buoy controls and adapted to macOS proportions (BAMA `mac-design`,
`ux-design`, `layout-grid`). It is a Buoy app, not a SwiftUI port.

## Design plan

1. **Archetype:** single-window document IDE with a sidebar (Navigator), a
   central editor and a secondary panel (Agent).
2. **Information architecture:** Navigator → Project items → layouts →
   controls (nested Views nest), plus a Library of designable controls. Two
   levels of meaning: *what file* and *which control*.
3. **Feeling:** calm and direct. The user's layout is the largest thing on
   screen; chrome is compact and grouped.

```
┌ Toolbar: Open Save │ Undo Redo ··················· Navigator Agent ┐
├ Navigator ┃ ContactWindow.bui        [Design|Source] ┃ Agent        ┤
│  Filter   ┃ Layout · 10 controls · in memory         ┃ provider     │
│  Project  ┃ ┌ canvas ───────────────┐ ┌ Inspector ┐  ┃ transcript   │
│   items   ┃ │  WYSIWYG layout       │ │ frame     │  ┃              │
│   controls┃ │                       │ │ property  │  ┃ composer     │
│  Library  ┃ ├━━━━━━━━━━━━━━━━━━━━━━━┴─┴───────────┤  ┃ Stop  Send   │
│           ┃ │ Messages                             │ ┃              │
└ status line ······································· Revision 3 ┘
 ┃ = native Splitter divider (drag, double-click collapses a side panel)
```

- Three panes of one vertical `Splitter`; the document column holds a
  horizontal `Splitter` (editor above Messages). Side panes and Messages hold
  their size when the window resizes; the editor absorbs the change.
- Minimums: Navigator 180, document 440, Agent 300, Messages 64, canvas 220.
  Navigator ≤ 420, Agent ≤ 640. Sizes and collapsed state persist in `Prefs`
  (`com.buoy.studio`, keys `studio.*`) and are clamped on load.
- Divider double-click collapses the *leading* pane natively; if that is the
  document column, Studio restores it and collapses the side panel instead.

## Command map

| Command | Menu | Shortcut | Toolbar |
| --- | --- | --- | --- |
| Open… | File | ⌘O | Open |
| Save | File | ⌘S | Save |
| Undo / Redo | Edit | ⌘Z / ⇧⌘Z | Undo / Redo |
| Design / Source | View | ⌘1 / ⌘2 | segmented switch in the document header |
| Show or Hide Navigator / Agent / Messages | View | ⌥⌘1 / ⌥⌘2 / ⌥⌘3 | Navigator, Agent toggles |
| Help search (macOS) | Help | ⇧⌘/ | system Help search field |

Ctrl replaces ⌘ on Windows and Linux. Help search is macOS-only: the system
Help menu field, registered through `desktopmenus.RegisterSystemMenus` after
each window attaches a Help menu.

## States

- **Startup:** the Contact sample opens in Design with `nameField` selected,
  so the canvas, Inspector and outline are populated immediately.
- **Document without a layout** (Markdown, plain code): opens in Source;
  the Design segment is disabled; the header says why.
- **Nothing selected:** "Choose an item in the Navigator".
- **Errors** (rejected edits, refused refactors, discovery failures) appear
  in Messages and the status line in plain language; nothing modal.
- **Agent signed out:** Refresh/Sign Out buttons are hidden until signed in.

## Reused native components

`containers/integrations/{treeview,searchfield,segmentedbutton}` modules are
imported unchanged (NSOutlineView/GtkTreeListModel/SysTreeView32,
NSSearchField/GtkSearchEntry/EDIT, NSSegmentedControl/GTK linked toggles/
BUTTON). `tools/build_studio.py` copies those exact files into the isolated
build. Toolbar, MenuBar and Splitter are Buoy built-ins.

## Documentation design plan and command map

The documentation surface is a separate reading window: a source sidebar, one
article, and an outline. It should feel calm and quick. Page content remains
readable while the sidebar changes scope. The native window and static export
share the reader, commands, and accessible labels.

| Action | Entry point | Mac | Windows / Linux |
| --- | --- | --- | --- |
| Search all documentation | Sidebar, Edit menu | Cmd+Shift+F, Cmd+K | Ctrl+Shift+F, Ctrl+K |
| Find on page | Reader toolbar, Edit menu | Cmd+F | Ctrl+F |
| Next / previous match | Find buttons, Edit menu | Cmd+G / Cmd+Shift+G; F3 / Shift+F3 | Ctrl+G / Ctrl+Shift+G; F3 / Shift+F3 |
| Back / forward | Reader toolbar, Go menu | Cmd+Left / Right; Cmd+[ / ] | Alt+Left / Right |
| Scroll by a page | Article | Space / Shift+Space; Page Down / Up | Same |
| Top / bottom | Article | Cmd+Up / Down; Home / End | Ctrl+Home / End; Home / End |
| Text size | Text-size control, View menu | Cmd+Plus / Minus / 0 | Ctrl+Plus / Minus / 0 |
| Copy a heading/member link | Anchor control | Keyboard activation or click | Same |
| Copy a code-line range link | Gutter, block Copy link button | Click, then Shift-click | Same |
| Open a code reference | Cmd/Ctrl-click, block References button | Cmd-click | Ctrl-click |
| Refresh package documentation | Reader status button, documentation window Help menu | Cmd+Shift+R | Ctrl+Shift+R |

Typing, text-selection shortcuts, native Option-arrow word movement, and Space
activation on buttons retain their editing/control meanings. Copy-link controls
never navigate or move the reading viewport. Explicit navigation to a copied
link does reveal its target. Closing find and dialogs returns focus predictably.
The code References control avoids thousands of token/gutter tab stops.

The BAMA mac-design and ux-design skills at the repository's pinned f0c2cdb
revision informed this pass. Their SwiftUI implementation patterns are adapted to
Buoy's native controls and HTMLView. The cross-platform Buoy glyph set is used
for consistent command/control identities, rather than introducing an SF Symbols
dependency on Windows and Linux.
