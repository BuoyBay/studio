# Native Linux build

The developer-tools repository supplies `tools/build_studio.bui`. Run from the
assembled BuoyBay integration workspace, with Buoy 0.72+, a C11 compiler, GTK 4,
the source repositories and their sealed dependency packages available.

Bootstrap the builder by sealing the public ProcessKit source into the integration
`packages/` directory. Its matching host runtime slice must be available from the
package's own source build or published runtime inputs. Also place the installed
toolchain's Crypto, Regex and Zip SDK archives in that directory. These SDK
archives are local build dependencies, not files to publish in Studio.
Use a separate output directory for the builder and each other executable:
compiler-deployed shared libraries must not be replaced while a tool uses them.

```sh
mkdir -p packages dist/studio-builder
/usr/bin/buoy pkg seal libraries/processkit -o packages/processkit-0.1.0.buipkg
# Copy the matching installed Crypto, Regex and Zip SDK archives into packages/.
buoy tools/build_studio.bui -o dist/studio-builder/build-studio-native
./dist/studio-builder/build-studio-native --check-only
./dist/studio-builder/build-studio-native
```

Use `--root PATH` when invoking outside the integration directory. `--compiler`,
`--packager` and `--cc` select a single executable path or PATH name; command
strings containing flags are not accepted. Linux defaults to `/usr/bin/buoy`
directly and supplies the installed 0.72 runtime, stubs and CRT paths without a
shell launcher. `--output PATH` selects the desktop executable. `--keep-work`
retains private staged inputs and receipts; failed builds always retain them.

The builder copies Studio, shared snippet sources and native integrations into
a private directory under `dist/`. It snapshots and source-seals all 27 declared
Studio dependency packages from the integration's `libraries/` tree. Existing
native slices are copied, not rebuilt or certified; each declared non-system
host library must have its matching runtime slice or the build fails with an
instruction to build that package first. Cached archives under the integration's
`packages/` are never used for Studio dependencies. It records
source and archive hashes in `studio-package-sources.json` and hashes the staged
inputs in `studio-build-info.json`; this native receipt schema does not promise
the same fingerprint as the historical Python builder.

The Linux terminal transport, pane clipping viewports and GTK lifecycle guard
compile directly with the C compiler. The viewports keep embedded WebKit's child
minimum requisitions from overriding native splitter allocations. The staged main
entry and its imports receive semantic checks for
all six desktop targets before the Linux desktop executable is linked. Those
checks establish language validity, not runtime behavior on the other targets.
The credential profile uses production/direct SecretStore behavior.

This entry point does not execute Python. Studio's existing discovery,
documentation and debugging Python helpers are still bundled as runtime
resources until those IDE integrations are migrated. The documentation snapshot
comes from the locally staged compiler documentation; the native builder does
not merge the Studio primer into its search index yet. The primer is bundled
separately. macOS/Windows builds, cross compilation, release/private native SDK
rebuild modes, development credential broker integration, automatic BuoyScript
helper deployment and the larger Python acceptance harness remain migration
work. Unsupported options fail explicitly. This builder must not be used as a
release-packaging substitute for those modes.

After a successful build, the existing native smoke test can run in a private
headless Linux display:

```sh
xvfb-run -a dist/BuoyStudio/BuoyStudio --self-test
```

Require exit status zero and `BUOY STUDIO SELF-TEST PASSED:`. This native smoke
test covers the application's built-in actions; it does not replace the broader
discovery, debugging, process and platform acceptance suites.
Failed built-in self-tests return status one.

The installed Linux Picture runtime requires genuine JPEG ABI 8
(`libjpeg.so.8`). Debian hosts that provide ABI 62 need a compatible ABI-8
runtime directory supplied through `LD_LIBRARY_PATH`; renaming or symlinking an
incompatible ABI is not a substitute. HTML documentation/chat views additionally
need WebKitGTK 6.0 (`libwebkitgtk-6.0.so.4`). The built-in smoke test can pass with
that optional view backend absent, so its success does not prove HTML views work.
