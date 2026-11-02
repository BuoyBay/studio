# Contributing to studio

This repository owns Buoy Bay Studio under `src/` and the shared
modules explicitly listed in the workspace inventory. Public product names are
**Buoy Bay Studio** and **Buoy Bay**. Reusable package changes belong in their own
repositories; keep IDE-only implementation in Studio.

## Development conventions

Use the installed Buoy language reference and verify changed `.bui` files with
the compiler. Prefer native Buoy/BuoyScript for new automation. Preserve public
command names, environment-variable contracts and explicit platform limitations.

## Build context

Read the [Studio source guide](src/README.md) and
[native Linux build guide](src/docs/NATIVE_BUILD.md). Build from the
assembled integration workspace with sibling sources, sealed dependencies and
locally provisioned toolchain resources. The documentation snapshot is a local
build input; do not commit compiler documentation without redistribution rights.

```sh
mkdir -p dist/studio-builder
buoy tools/build_studio.bui -o dist/studio-builder/build-studio-native
./dist/studio-builder/build-studio-native --check-only
./dist/studio-builder/build-studio-native
```

These commands run from `local/integration/`, not this standalone checkout.
Keep each executable's deployed runtime libraries in its own output directory.
The native entrypoint currently supports the documented Linux workflow; release,
SDK, signing and other-host modes retain their stated migration limitations.

## Verification and pull requests

For each edited Buoy entrypoint, in its documented staged dependency context:

```sh
buoy path/to/entrypoint.bui --check --diagnostic-format json
```

Require exit status zero and an empty `errors` array. Read warnings. Check each
affected conditional platform branch with `--target`; compiler checks do not prove
native runtime behavior on another OS. Build/link and exercise the changed API or
workflow on its supported host as well.

Include your compiler/tool versions, host/target, exact build and
reproduction commands, expected behavior and observed output in the pull request.
Do not commit generated builds, caches, credentials or machine
configuration. Keep the patch focused and note any unverified platforms or runtime
prerequisites. Inventory checks alone are not build or test acceptance.

## License of contributions

Studio application code is `AGPL-3.0-only`. Original contributions to that core
require the [individual CLA](legal/cla-individual.md), or the
[entity CLA](legal/cla-entity.md) when an employer/entity owns the work, before
merge. Arrange signing with a maintainer; CLA automation is not currently active.
Contributors keep copyright, and the community edition retains its AGPL license
under the policy in [LICENSING.md](LICENSING.md).

Permissive shared-module/tool changes and `0BSD` generated-project scaffold
changes do not require a CLA or DCO signing step unless the patch also contributes
original AGPL-core work. Documentation prose is `CC-BY-4.0`; code examples follow
the code they describe. See [LICENSE](LICENSE) and the [license texts](LICENSES/).

Retain upstream copyright, license, attribution and provenance files for material
you do not own. Identify it in the pull request as
`Submitted on behalf of a third party: [project or copyright holder]`; do not
represent that material as your own CLA contribution.
