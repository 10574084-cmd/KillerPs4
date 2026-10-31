# PS4 porting plan

## Selected game

Killer Instinct 1 (arcade, 1994), MAME-compatible set name `kinst`. The default revision is L1.5D; L1.4 may be selectable when its boot ROM is present.

## Candidate emulator core

[UltraCombo](https://github.com/thefixinhixon/UltraCombo) is the leading candidate instead of embedding the MAME application. Its MIT-licensed source already contains a standalone MIPS R4600 machine model, IDE/CHD handling, DCS/ADSP audio emulation, frame output and a KI1 game profile.

This is a candidate, not an already completed port. It currently targets desktop SDL2 and OpenGL. The PS4 work must isolate the emulation core from the desktop frontend, use a PS4-compatible SDL2/platform layer, and verify every required API at link and runtime.

### Runtime recompilation

UltraCombo can translate game code to C and launch a compiler to produce native modules at runtime. That path will be disabled for the first PS4 build. The first target is the interpreter-only path; its speed and timing must be measured on actual PS4 hardware before deciding whether a console-compatible static recompilation path is needed.

Do not commit generated C or native modules derived from the commercial game ROM.

## PS4 integration milestones

1. **Native platform shell**
   - Build a minimal PS4 ELF and PKG with the existing OpenOrbis-compatible package flow.
   - Verify application startup, clean exit, writable save-data path and DualShock 4 input.
   - Keep the title clearly marked as a development build until the emulator is connected.

2. **Video, audio and input backend**
   - Establish a 320×240 frame upload path to the PS4 renderer.
   - Add nearest and bilinear sampling.
   - Add shader passes only after the basic frame path is stable.
   - Convert the emulated audio stream to the PS4 output format without tying emulation speed to rendering.

3. **KI1 emulator core**
   - Import the required UltraCombo source and preserve its MIT license and third-party notices.
   - Separate machine initialization, one-frame execution, frame-buffer output and sound sample retrieval from the desktop UI.
   - Add a console build flag that omits runtime compiler/process launching.
   - Build and test the interpreter path before enabling any optional optimization.

4. **Local game-data loader**
   - Read the KI1 boot ROM, eight sound ROMs and CHD/raw disk from user-provided storage.
   - Validate sizes and checksums and display a useful missing-file report.
   - Keep all game data outside the source tree, Actions artifacts and published PKGs.

5. **Presentation and input**
   - Preserve the original 4:3 playfield in all layouts.
   - Offer a 16:9 presentation mode using side areas/background rather than non-uniformly stretching gameplay.
   - Add bilinear filtering and supported shader presets (sharp, smooth, scanlines and CRT).
   - Map the DualShock 4 to the arcade's six action buttons and two-player inputs.

6. **Hardware validation**
   - Confirm boot, attract mode, one full match, sound, two-player controls, reset and save data on a real console.
   - Record frame pacing against the arcade's approximately 58.96 Hz timing.
   - Do not label a green CI build as proof of a playable game.

## Data manifest

The normal KI1 set consists of:

| Role | File |
|---|---|
| Boot ROM, default | `ki-l15d.u98` |
| Boot ROM, optional older revision | `ki-l14.u98` |
| Sound ROMs | `u10-l1`, `u11-l1`, `u12-l1`, `u13-l1` |
| Sound ROMs | `u33-l1`, `u34-l1`, `u35-l1`, `u36-l1` |
| Arcade hard disk | `kinst.chd` or `kinst.raw` |

The game data is not supplied here. Users must provide files they are entitled to use.
