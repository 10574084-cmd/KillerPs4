# UltraCombo KI1 machine-core integration

## Pinned source

The PS4 build fetches [UltraCombo](https://github.com/thefixinhixon/UltraCombo) at commit
`b7ae26e7605efbe53cb60433ade26d5579900d8e`. The upstream project is MIT-licensed;
the notice is preserved in [UltraCombo-MIT.txt](../licenses/UltraCombo-MIT.txt).

## What this milestone does

- Adds a PS4-facing adapter for game initialization, frame stepping, active-low
  arcade inputs, 320x240 RGB frame extraction and DCS sample retrieval.
- Cross-compiles the interpreter, KI1 board profile, IDE/CHD reader, DCS/ADSP
  sound components, state support and dependencies for the OpenOrbis target.
- Builds those objects into `build/libki1core.a` as a separate CI artifact.

## What it does not do yet

The archive is a compile milestone only. It is **not linked into the currently
working framebuffer-test executable**, and the PS4 screen still shows the
synthetic test pattern. The KI1 asset loader and frame renderer need to be
connected to the application before this is a game build.

The adapter looks for user-supplied files in these locations, in order:

1. `/data/KillerPs4/roms/kinst/`
2. `/data/KillerPs4/kinst/`
3. `/data/KillerPs4/`
4. `/mnt/usb0/KillerPs4/roms/kinst/` and sibling folders
5. `/mnt/usb1/KillerPs4/roms/kinst/` and sibling folders

The expected files are `ki-l15d.u98` (or `ki-l14.u98`), the eight
sound ROMs listed in the project README, and `kinst.chd` or `kinst.raw`.
No copyrighted game data is stored in this repository or in the build artifact.

## Next integration work

1. Link the core archive into a separate experimental EBOOT while preserving
   the known-good visual test build as a fallback.
2. Connect the adapter's 320x240 frame output to the center 4:3 viewport.
3. Map DualShock inputs to the KI1 two-player arcade ports.
4. Connect the DCS sample queue to PS4 audio output and test frame pacing.
5. Test boot, attract mode, gameplay and audio on real hardware.
