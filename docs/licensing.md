# Licensing and provenance

## License scope

Copyright © 2026 Danylo Shamaiev for original TinyS3 port contributions.

The original TinyS3 application, build configuration, tooling, and documentation
are free software under the **GNU General Public License, version 3 or, at your
option, any later version** (`GPL-3.0-or-later`). The full, unmodified license is
in [LICENSE](../LICENSE). This covers `main/`, `tools/`, the project and component
build files, project configuration files, `README.md`, and the documentation.
The rewritten `components/doom/src/config.h` is also covered by this grant.

The inherited engine is treated separately. Its existing license and copyright
notices are preserved. Danylo Shamaiev's changes to inherited engine files other
than the rewritten `config.h` are offered under **GPL-2.0-or-later**, retaining
the upstream engine's version flexibility. This permission covers those changes
only; it cannot expand another author's grant. Unmodified inherited files retain
their original notices without a new claim of authorship.

The license texts themselves are reproduced verbatim under their respective
copying permissions. They are not original project documentation.

## Upstream code

The engine derives from DoomGeneric and its DOOM, Chocolate Doom, fbDOOM, and
Frosted Doom lineage, at upstream snapshot
`dcb7a8dbc7a16ce3dda29382ac9aae9d77d21284`. It retains the original authors'
copyright notices and applicable license terms. Moving or modifying these files
does not replace those terms with this project's license for original code.
See [AUTHORS.md](../AUTHORS.md) for credits and
[THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md) for upstream license references.

## Modifications to the upstream engine

Danylo Shamaiev's modifications adapt the engine to his specific hardware build
using the ESP32-S3 TinyS3 development board.

## Warranty and liability

The original project code is provided without warranty, including any implied
warranty of merchantability or fitness for a particular purpose. The warranty
disclaimer and limitation of liability in GPLv3 sections 15–17 apply, to the
extent permitted by applicable law. Inherited code retains the disclaimers in
its applicable licenses. No additional warranty or maintenance obligation is
offered by this project.

These terms do not promise exemption from liabilities that cannot legally be
excluded.

## Source and binary distributions

Preserve applicable copyright, modification, license, and attribution notices.
When distributing GPL-covered firmware, provide the corresponding source and
build/install material required by the applicable license; linking to a moving
development branch alone does not identify the source of a particular binary.
Retain the notices for the SDK components actually included in that build.

See [AUTHORS.md](../AUTHORS.md) and
[THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md). DOOM game assets are separate
from the engine's software license and are not included in this repository.
