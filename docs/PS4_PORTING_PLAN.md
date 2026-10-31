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
   - Decide and build the supported PS4 graphics backend (the committed shader assets target GLSL 3.30 Core; runtime availability still needs proof).
   - Add nearest and bilinear sampling independently from shader presets.
   - Wire the `Lush Arcade` passes: bright-pass extraction, half-resolution separable blur, final bloom/color-grade composite.
   - Wire the optional `Arcade CRT` pass as an alternative, not an extra mandatory layer.
   - Keep the game viewport 4:3 and add side areas for 16:9 displays.
   - Integrate `shaders/passes/wide_preserve_4_3.frag` after the optional color-grade pass.
   - Keep menus and OSD aligned to the centered 4:3 game viewport.
   - Treat this as aspect-preserving presentation, not a claim of extra game-world visibility.
   - Convert the emulated audio stream to the PS4 output format without tying emulation speed to rendering.

3. **KI1 emulator core — initial cross-compile milestone reached**
   - Pin the UltraCombo source revision and preserve its MIT notice.
   - Add a PS4-facing adapter for machine initialization, one-frame execution, active-low arcade inputs, RGB frame extraction and DCS sample retrieval.
   - Cross-compile the KI1 interpreter, board profile, IDE/CHD, DCS/ADSP and state modules into `build/libki1core.a` in CI.
   - The archive is currently a separate artifact and is not linked into the working display-test EBOOT.
   - Next: resolve link dependencies, link a separate experimental EBOOT, then connect the 320×240 frame output, pad input and DCS audio to the PS4 app.
   - Keep runtime compiler/process launching disabled for the first console build and measure interpreter performance before calling the port playable.

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

## Shader implementation notes

The original shader sources and preset manifests live in `shaders/`. See [shaders/README.md](../shaders/README.md) for the design, pass order and limitations.

The shader direction takes inspiration from the lighting/color goals of Minecraft's Newb Shader but is independently authored for a 2D arcade frame. There is no depth, normal or per-object material buffer in the 320×240 output, so the current effects are screen-space only. Proper cast shadows and reflections are not possible without additional game-specific scene data.

The shader sources are validated on the host with `sh tools/test_shaders.sh`. That checks GLSL syntax and JSON manifests, not PS4 shader compilation or frame rendering. The renderer still needs to load the shader files, create its framebuffers, bind the uniform values from the preset and present the composited frame.
