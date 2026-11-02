# Buoy glyphs

The Buoy glyph set has 77 original monochrome SVG glyphs for small IDE
surfaces: project types, navigator groups, project items, every control and
object type the layout tools support, code symbols, and fallbacks. App
identities (the rich, coloured application icons) live separately under
[`identity/`](identity/).

| Path | What it is |
|------|------------|
| `manifest.json` | The **canonical** source. Every glyph, its native SF Symbol / Fluent equivalent, and every mapping table (controls, navigator tags, project items, project types, apps, code symbols, fallbacks). |
| `svg/<key>.svg` | The artwork, one file per glyph key. |
| `gallery.html` | Generated review page: every glyph at 16/20/24/48 px, light and dark, all mapping tables, app identities and the coverage report. Open it in a browser. |
| `contact-sheet.png` | Generated raster sheet, light and dark, produced by Buoy's own `Picture.FromSVG`. It shows exactly what Studio draws. |
| `identity/` | App-identity vector designs (BuoyStudio and the Permissions proposal), plus image-generator prompts for them. |
| `../../apps/BuoyStudio/StudioGlyphs.bui` | Generated Studio module: SVG sources, the mapping functions and the `StudioGlyphView` slot. Do not edit it by hand. |

## Build and validate

The tool needs only Python's standard library. `--contact-sheet` also needs
the `buoy` compiler, and the rendered console binary must be able to load
the runtime's image libraries. On hosts without `libjpeg.so.8`, set
`LD_LIBRARY_PATH` to a directory that provides it.

Validation fails on any of the following:

- **SVG rules.** The SVG doesn't parse, uses a different `viewBox` or root stroke attributes, uses an element outside `path/rect/circle/ellipse/line/polyline/polygon`, uses any colour other than `currentColor`/`none`, sets opacity below 0.2, places geometry off the 24px canvas, or uses styles, transforms, text, images, gradients or scripts.
- **Manifest integrity.** A mapping points at a missing glyph, a glyph is never mapped, an SVG file is undeclared, or a Fluent name is not in the installed `fluenticons-1.0.0` package.
- **Coverage against the real inventory.** Each of the following must have a glyph:
  - every `LayoutCatalog.RegisterBuiltinControls` type;
  - every control in any `resources/buoy-designer/plugin.json` (in `libraries/` and inside `packages/*.buipkg`);
  - every type the layout model special-cases (`Window`, `View`, `GroupBox`, `RadioGroup`, `Splitter`, scanned from LayoutCanvas/LayoutDocument);
  - every control page in the installed `docs/documentation/ui/` reference, plus `StackView`, `Canvas`, `Toolbar` and `MenuBar`;
  - every native component control in `containers/integrations` (and every Studio `COMPONENTS` module);
  - every Studio navigator tag prefix and toolbar action glyph;
  - every app in `apps/`, which also needs an existing identity or a proposal.
- **Generated files.** `StudioGlyphs.bui` or `gallery.html` is stale.

## Design grid

- `viewBox="0 0 24 24"`, live area 2–22 px, `fill="none"`, `stroke="currentColor"`, `stroke-width="1.75"`, round caps and joins. These attributes go on the root `<svg>` only. Individual shapes do not override stroke width.
- Strokes scale to 1.17 px at 16 px, 1.46 px at 20 px and 1.75 px at 24 px. Keep parallel strokes at least 3 units apart (centre to centre) so they stay separate at 16 px.
- Corner radii: 2–2.5 for frames, 3–5.5 for pill controls, 1–1.5 for inner details.
- Tint comes only from `currentColor`. Small solid accents (dots, filled thumbs) use `fill="currentColor" stroke="none"`. Secondary parts use `opacity` (0.35–0.45 is typical; 0.2 is the floor). No hard-coded colours, so the same file works on light and dark backgrounds.
- No lettering except the conventional code-symbol monograms (C, I, E, S, P), which are drawn as paths.
- Shape language: **types** (class, enum, structure) sit in rounded squares and an interface uses a dashed square. **Members** (method, property, constant) sit in circles. **Files** share one folded-corner sheet with a distinct inner mark. **Controls** draw the control's own silhouette, not a metaphor.

## Resolution and fallback

Every consumer resolves a glyph in this order (`manifest.json` → `resolution`):

1. **Declared.** A plugin control may add `"icon": "<glyph key>"` to its `plugin.json` entry. The validator rejects unknown keys, and `StudioGlyphKeyForControl(typeName, declared)` honours a declared key. Plugins can also provide a validated drawing recipe or PNG through LayoutKit's icon descriptor fields; those are host-rendered data, separate from this glyph-key lookup.
2. **Mapping.** The manifest tables: `controls.*`, `navigator`, `items`, `projectTypes`, `symbols`, `actions`.
3. **Category fallback.**
   - Package-qualified control types (containing `.`) → `control.package`.
   - Other controls → `control.generic`.
   - Items → `file.document`.
   - Projects → `brand.buoy`.
4. **Last resort.** `status.unknown`. `StudioGlyph` never returns a blank for an unknown key.

## Native platform conventions

Studio's toolbar and document header use the shared Buoy SVG glyphs.
The manifest also records optional SF Symbol and Fluent equivalents for other
consumers that prefer native platform artwork. A host that wants native-first rendering tries the native
name first: `Picture.FromSystemIcon` / `Picture.FromFluentIcon`, falling
back to the Buoy SVG when the result is `Nil` or no equivalent is listed.

- **Fluent names** are verified by the build against the installed package.
- **SF Symbol names** were checked with AppKit on m4-casa (macOS 27.2): all 55 resolve. Older macOS releases may lack individual symbols; treat any name returning `Nil` as absent.

Buoy-specific types such as StatusBadge, BevelButton and the layout roots
have no exact native counterpart. They always use the Buoy glyph, so Studio
uses the Buoy glyph for content and object surfaces on every platform.

## Studio integration

- `StudioWindow.DocumentGlyph` (a `StudioGlyphView`) sits in the document header. It shows the item glyph from `StudioGlyphKeyForItem`: layout, entry `main.bui`, source, test, config, text, image, or folder. With no selection it shows `brand.buoy`. It is re-tinted for light/dark on every chrome refresh and rasterized at 2× for HiDPI.
- The Studio self-test asserts that the header shows `file.layout` for the sample. It also asserts that every catalog control, including discovered package controls, resolves to a non-generic glyph that rasterizes.
- The toolbar uses the seven `toolbar` action glyphs. Navigator and Library source integration assigns generated PNG/ICO files through `TreeViewNode.IconPath`; runtime verification of those evolving native row paths is separate from the validated glyph inventory. The manifest's `navigator` table maps groups, while item/control lookup functions select row glyphs.
- The LayoutCanvas palette lives in the sealed `layoutcanvas` package; changes to that surface require its package build and runtime checks.

## Adding or changing a glyph

1. Draw it on the grid above. Start from the closest existing glyph so stroke rhythm and radii match.
2. Save it as `svg/<category>.<name>.svg`. The key is the file name.
3. Add an `icons` entry to `manifest.json` (title, category, file, and `native` names you are sure of). Then map it from the right table. Unmapped glyphs fail validation.
4. For a new control type, add it under `controls.catalog` (catalog or plugin controls), `controls.framework`, `controls.components` or `controls.layoutRoots`. Package authors can instead declare `"icon"` in their `plugin.json`.
5. Run `python3 tools/build_iconset.py --write --contact-sheet`. Then review `gallery.html` and `contact-sheet.png` at 16 px in both light and dark.
6. Studio picks the glyph up through the regenerated `StudioGlyphs.bui`. Run `python3 tools/build_studio.py --check-only`, which checks all six targets.

## App identities

The existing artwork in `apps/*/resources` (BuoyDocs, Calls, Xippets) is
untouched and is referenced from `manifest.json` → `apps`. BuoyStudio now also
has packaged artwork under `apps/BuoyStudio/resources`. `identity/BuoyStudio.svg`
and `identity/Permissions.svg` provide vector designs in the same visual
language (88% rounded tile, ivory glyph, upper-left light).
The Permissions design remains a proposal rather than an assigned application
icon. `identity/prompts.json` holds
matching image-generator prompts in the style of `apps/icon-prompts.json`, for
when a raster master in the ceramic style of the existing icons is wanted.
After approval, package a raster master with `tools/build_icons.py`.

Studio app PNG, ICO, and ICNS files and tinted native tree-row icons are generated
from the checked-in SVGs with `python3 tools/build_studio_icons.py` (ImageMagick
required to regenerate). Builds package these resources without an image-tool
dependency. The generator emits 20-pixel tree artwork and 40-pixel HiDPI variants.
Studio configures navigator rows with 16-point icons; TreeView scales the artwork
to its configured logical icon size.
The designer control picker uses LayoutCatalog icon descriptors in its native
list/grid palette, including validated package-provided icons.
