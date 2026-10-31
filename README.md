# KillerPs4

Native PlayStation 4 homebrew port project for **Killer Instinct (arcade, 1994)**.

## Target
- Game: Killer Instinct (1994), arcade set `kinst`.
- Initial revision: L1.5D, with L1.4 as an optional selectable revision.
- Platform: PlayStation 4 homebrew package for GoldHEN / OpenOrbis.
- Goal: run the original arcade game code, not a recreated game or a menu-only mock-up.

## Project status

**Early porting stage — not playable yet.**

A portable KI1 data-manifest scanner and host tests are now present in `src/ki1_data.c`. The emulator core and PS4 frontend have not yet been integrated. The scanner is not connected to a PS4 menu yet. A successful host test or future package build will not prove that the game runs; actual emulation must also be tested on hardware.

## Emulation approach

The first implementation candidate is the MIT-licensed [UltraCombo](https://github.com/thefixinhixon/UltraCombo) KI1 emulator core. It already models the Killer Instinct board components needed by this game, including the MIPS CPU, DCS audio and IDE hard disk. It is a native Windows/Linux program today, so its frontend and platform dependencies still need a PS4 adaptation.

The intended PS4 build will not require the MAME application to be installed or launched. It will, however, accept the arcade ROM and hard-disk data in the established `kinst` set format.

The interpreter path is the initial PS4 target. UltraCombo's optional runtime recompilation invokes external compiler tools, which is not a suitable assumption for a console build. Interpreter performance has to be measured before calling the port playable.

## Game data

**No game ROM, hard-disk image, BIOS, or other proprietary game data is included in this repository or in planned PKG releases.** Keep personal game files outside Git and provide them to the application locally.

The KI1 set normally needs:
- Boot ROM: `ki-l15d.u98` (default L1.5D) or `ki-l14.u98` (optional L1.4).
- Eight sound ROMs: `u10-l1`, `u11-l1`, `u12-l1`, `u13-l1`, `u33-l1`, `u34-l1`, `u35-l1`, `u36-l1`.
- Hard-disk image: `kinst.chd` (or a raw `kinst.raw` image).

The first manifest module selects a valid L1.5D or L1.4 boot ROM, checks the eight sound ROM sizes and CRC32 values, and checks the raw disk size or the CHD file signature. Full CHD integrity verification and wiring the report into a PS4 loader are still pending. Exact storage path and transfer workflow will be finalized with the first loader build.

## Video and controls target

The arcade image is 320×240 at approximately 58.96 Hz. The presentation layer is intended to provide:
- Original 4:3 presentation and an optional 16:9 layout that preserves the game's aspect ratio rather than stretching the playfield.
- Nearest-neighbour and bilinear filtering.
- Sharp, smooth, scanline and CRT-style shader options where supported by the PS4 graphics backend.
- DualShock 4 navigation and two-player arcade controls.

Filters and screen layout must not change the emulated game's timing.

## Build

The PS4 build scripts and package workflow are not in place yet. Host-only manifest tests can be run with `sh tools/test_ki1_data.sh`; GitHub Actions runs the same test on pushes and pull requests. These tests do not build a PS4 ELF or PKG. See [docs/PS4_PORTING_PLAN.md](docs/PS4_PORTING_PLAN.md) for the current implementation sequence and compatibility notes.

## Credits and licensing

- UltraCombo is MIT-licensed. Its license and the notices for any imported third-party code must be preserved if its source is integrated.
- Killer Instinct and its game code/data are the property of their respective rights holders. This independent homebrew project is not affiliated with them or Sony.
