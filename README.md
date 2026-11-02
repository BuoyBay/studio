# Native IDE

Buoy Bay Studio combines a project navigator, source editor, visual layout canvas,
property inspector, build messages and an agent panel. Native split panes keep
those tools in one desktop workspace; source edits and design edits share the
project's document model.

## Work with a project

- Browse project files and reusable controls in the Navigator and Library.
- Switch between Source and Design views; inspect and edit selected controls.
- Build the project and inspect compiler messages beside the document.
- Use explicit agent actions for project context, tools and proposed edits.

The [Studio guide](src/README.md) describes editor actions and application behavior.
The [design reference](src/docs/DESIGN.md) explains layout and interaction rules.

## Build on Linux

Use the [native build guide](src/docs/NATIVE_BUILD.md) to provision source/package
dependencies and compile the driver supplied by `devtools`. From the assembled
integration directory, the native workflow is:

```sh
mkdir -p dist/studio-builder
buoy tools/build_studio.bui -o dist/studio-builder/build-studio-native
./dist/studio-builder/build-studio-native --check-only
./dist/studio-builder/build-studio-native
```

The driver checks all six compiler targets, then links the Linux application.
It does not certify native behavior on the other platforms or replace release
signing/packaging. Keep its runtime libraries isolated from other build tools.
Linux Picture support needs JPEG ABI 8; HTML documentation/chat views additionally
need WebKitGTK 6.0. Those backends must be present to validate their features.

Studio application code is in `src/`; reusable shared modules are in `shared/`.
The generated-project scaffold exception and component license boundaries are
explained in [LICENSING.md](LICENSING.md).

[Contributing](CONTRIBUTING.md) · [License](LICENSE)
