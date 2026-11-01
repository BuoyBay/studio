# Studio licensing

Studio's application source in `src/` is licensed `AGPL-3.0-only`.
The community edition remains available under that license. The Project Steward
is Donald Lee Lunder 3rd, doing business as Zerova (public name Don Lunder,
GitHub [donl](https://github.com/donl)).

## License boundaries

| Material | License |
| --- | --- |
| Studio application implementation | `AGPL-3.0-only` |
| Shared reusable modules in `shared/`, developer tools and reusable integration code | `MIT OR Apache-2.0` |
| Studio New Project scaffolds and generated project templates | `0BSD` |
| Documentation prose | `CC-BY-4.0` |
| Documentation code samples | License of the code being described |
| Third-party material | Its retained upstream terms |

The scaffold exception covers generated user-project code, not the IDE
implementation that produces it. User programs compiled with Buoy or edited in
Studio do not become derivatives of Studio merely through that use. Studio may
import permissive packages; reusable packages must not import Studio core code.
Full license texts are in [LICENSES/](LICENSES/). Specific upstream notices take
precedence for the material they identify.

## Contributions and additional licensing

Original outside contributions to the AGPL application core require the applicable
[individual](legal/cla-individual.md) or [entity](legal/cla-entity.md) Harmony CLA
(Option Five). Contributors keep copyright. The steward may offer additional
commercial/app-store licenses under that grant while retaining the community
edition's AGPL license and the existing permissive/shared/scaffold boundaries.
The CLA does not grant compiler/runtime, third-party redistribution or trademark
rights. The compiler/runtime is a separately licensed product.

Contributions confined to permissive shared code, tools or generated-project
scaffolds do not require a CLA or DCO. A mixed patch that contributes original
AGPL-core work still needs the applicable CLA for that work. Upstream material
keeps its own license and is not represented as original CLA work.
See [CONTRIBUTING.md](CONTRIBUTING.md) for the review and signing process.

Copyright licenses do not grant trademark rights. The public product names are
**Buoy Bay Studio** and **Buoy Bay**. Existing third-party rights remain unchanged.
