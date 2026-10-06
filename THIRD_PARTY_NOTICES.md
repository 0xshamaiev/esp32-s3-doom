# Third-party notices

Third-party code retains its original copyright and license terms. A project
license or a credits entry does not replace those terms.

## DOOM engine lineage

`components/doom/` is derived from [DoomGeneric](https://github.com/ozkl/doomgeneric),
with earlier Chocolate Doom, fbDOOM, Frosted Doom, and id Software history retained
in Git. The upstream snapshot immediately preceding this port is
`dcb7a8dbc7a16ce3dda29382ac9aae9d77d21284`.

The original engine source notices name id Software, Simon Howard, Raven Software,
Andrey Budko, and Ben Ryves. Additional provenance is recorded in
[AUTHORS.md](AUTHORS.md) and in the source files themselves.

Most engine files explicitly grant GNU GPL version 2 or any later version.
Their original notices remain intact. The upstream repository's GPLv2 license
text has been restored verbatim as [licenses/GPL-2.0.txt](licenses/GPL-2.0.txt)
from the snapshot above; it was originally added upstream in commit
`0b085de3725d60d24ed4dac4dfed68a3aca86140`.

For this port's license scope, see
[docs/licensing.md](docs/licensing.md).

## ESP-IDF and runtime dependencies

ESP-IDF is installed separately. Its original code is primarily licensed under
Apache License 2.0, with copyright held by Espressif Systems and the respective
contributors. A copy of the installed SDK's root license is included in
[licenses/Apache-2.0.txt](licenses/Apache-2.0.txt). The build also downloads the
Apache-2.0 `espressif/usb` and `espressif/usb_host_hid` components, pinned in
`dependencies.lock`.

ESP-IDF also incorporates third-party code under other licenses. For example,
its copyright inventory identifies MIT-licensed FreeRTOS and Xtensa headers, and
libc code with its own BSD and other notices. The Apache license does not replace
those licenses. Consult the exact SDK version's source headers, component license
files, and [copyright inventory](https://github.com/espressif/esp-idf/blob/master/docs/en/COPYRIGHT.rst)
when preparing a firmware distribution.

The locally verified SDK identifies itself as `v6.1-dirty`; that is a local build
identifier, not a reproducible release specification. A firmware release should
record its exact SDK revision and any local SDK changes, and carry the applicable
dependency notices and corresponding source. This document is an attribution
inventory, not certification that every possible SDK configuration is compatible.

## Game data

No DOOM IWAD, graphics, maps, sound recordings, or other game data is included.
The engine's software license does not grant rights to those assets. Any game data
supplied separately remains subject to its own terms.

## Warranty

Keep the warranty and liability provisions in the accompanying licenses intact.
The port provides no additional warranty, support commitment, or endorsement on
behalf of upstream authors.
