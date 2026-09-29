# Authors and acknowledgements

This port builds on the work of the original DOOM authors and the developers who
maintained, extended, and made the engine easier to port. Substantial changes to
their code do not diminish their contributions.

The TinyS3 adaptation and its changes are maintained by **Danylo Shamaiev**.
Problems introduced by this port should be reported here, rather than attributed
to the upstream authors. This project does not imply their endorsement.

## Engine and upstream projects

- **id Software and the original DOOM team** — the original engine. Retained
  source notices credit id Software, Inc., 1993–1996.
- **Simon Howard and the [Chocolate Doom contributors](https://github.com/chocolate-doom/chocolate-doom)**
  — engine maintenance and portability work. Retained notices credit Simon
  Howard, 2005–2014.
- **[ozkl and the DoomGeneric contributors](https://github.com/ozkl/doomgeneric)**
  — the portable engine interface on which this port is based.
- **Maxime Vincent** — the Frosted Doom and fbDOOM stages of the retained Git
  history, including the move to the Chocolate Doom codebase.
- **Raven Software** — contributions identified by retained 1993–2008 notices.
- **Andrey Budko** — contributions identified in the engine's source notices.
- **Ben Ryves** — MUS-to-MIDI conversion code, credited in `mus2mid.c`.
- **Florian** — credited by that name in the inherited `doom.h`; the original
  attribution is preserved without guessing a fuller identity.

The retained upstream Git history also credits Akhsarbek Gozoev, Arawn Davies,
Daniel Bomar, Fabian Ruhland, Georgi Gerganov, Travis Bradshaw, Trider12,
Turo Lamminen, bcoles, indigoparadox, isif00, lukneu, techflashYT, and Ørjan.
Some of their platform-specific contributions are outside the scope of this
trimmed port. This acknowledgement is not a claim that every historical file is
still included, or an exhaustive list of everyone who contributed to DOOM.

## Platform software

Thanks to **Espressif Systems and the ESP-IDF contributors**, and to the authors
of the runtime libraries used by ESP-IDF. Their software keeps its own copyright
and license notices; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## TinyS3 port

Copyright © 2026 Danylo Shamaiev for original work adapting the engine to his
specific hardware build using the ESP32-S3 TinyS3 development board. See
[the licensing notes](docs/licensing.md) for license scope.

This file supplements the original source notices; it does not replace them or
transfer ownership of upstream work.
