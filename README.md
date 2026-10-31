# KillerPs4

Native PlayStation 4 homebrew port project for **Killer Instinct (arcade, 1994)**.

## Target
- Game: Killer Instinct (1994), arcade set `kinst`.
- Initial revision: L1.5D, with L1.4 as an optional selectable revision.
- Platform: PlayStation 4 homebrew package for GoldHEN / OpenOrbis.
- Goal: run the original arcade game code, not a recreated game or a menu-only mock-up.

## Project status

**Early porting stage — not playable yet.**

A portable KI1 data-manifest scanner and host tests are present in `src/ki1_data.c`. The working PS4 application is still a separate OpenOrbis 16:9 display test in `src/main.cpp`; it draws a synthetic 320×240 pattern, not Killer Instinct gameplay. A PS4-facing machine adapter and pinned UltraCombo source-build check have now been added; see [ULTRACOMBO_CORE.md](docs/ULTRACOMBO_CORE.md). The core archive is not yet linked into the running application.

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

## Video and shader target

The arcade image is 320×240 at approximately 58.96 Hz. Original GLSL 3.30 shader sources and preset manifests are now in `shaders/`:

- **Lush Arcade**: Newb-style visual direction with warm highlights, cool shadows, richer color, soft bloom and a restrained vignette.
- **Arcade CRT**: optional scanlines, subtle RGB aperture mask and vignette.

The Lush Arcade preset uses a bright-pass extraction, two separable blur passes and a final color-grade/composite pass. It keeps the source at 320×240 and plans to use a half-resolution bloom buffer.

**These shader files are not connected to a PS4 frame renderer yet.** The repository does not have the emulator video loop or an OpenGL context at this stage. GitHub Actions now validates the GLSL syntax and preset JSON, but that is not a PS4 runtime test.

The intended presentation still preserves the original 4:3 playfield. The new `Wide Arcade 16:9` pass keeps the full game frame centered and fills the side areas with a darker, blurred crop of the current scene. At 1920×1080, gameplay remains 1440×1080 with 240-pixel side gutters. This is a presentation patch, not a true expanded camera/FOV hack: the arcade outputs a fixed 320×240 frame. See [WIDESCREEN.md](shaders/WIDESCREEN.md).

Nearest-neighbour and bilinear scaling remain independent of the Lush Arcade / CRT presets. Filters must not change emulation timing.

## Build and test package

The host-only manifest tests can be run with `sh tools/test_ki1_data.sh`. Shader syntax and preset JSON are checked with `sh tools/test_shaders.sh`.

The workflow `.github/workflows/ps4-widescreen-test.yml` builds a separate PS4 **widescreen test PKG** using OpenOrbis and uploads the `KI1-Widescreen-Test-PS4` artifact. It is a synthetic display test, not a game build. Only install it after its Actions run finishes successfully. See [docs/WIDESCREEN_TEST_BUILD.md](docs/WIDESCREEN_TEST_BUILD.md).

The real KI1 ELF/PKG is not ready: the machine core has an initial PS4 adapter and a separate cross-compile target, but is not yet linked to the running app or connected to the renderer/audio output. See [docs/PS4_PORTING_PLAN.md](docs/PS4_PORTING_PLAN.md) for the actual porting milestones.

## Credits and licensing

- UltraCombo is MIT-licensed. Its license and the notices for any imported third-party code must be preserved if its source is integrated.
- Killer Instinct and its game code/data are the property of their respective rights holders. This independent homebrew project is not affiliated with them or Sony.
