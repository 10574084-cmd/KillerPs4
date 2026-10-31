# KI1 widescreen test package

This package is a **display-layout test only**. It does not contain the Killer Instinct arcade game, the UltraCombo emulator core, ROM data or game audio.

## What it tests

- A 1920×1080 PS4 video output.
- A centered 1440×1080 content rectangle (4:3).
- 240-pixel side gutters on the left and right.
- Controller navigation of the test overlays.

The center shows an original geometric test pattern labeled 320×240. The side areas use a dimmed gradient and soft color bands to preview the side-fill treatment. They are not generated from the real Killer Instinct frame, and this test app does not execute the GLSL shader assets yet.

## DualShock 4 controls

| Button | Action |
|---|---|
| Cross | Toggle the side-fill backdrop |
| Triangle | Toggle the 40-pixel alignment grid |
| Square | Toggle the inner safe-frame border |
| Left / Right | Reduce or increase sidebar brightness |
| Circle | Reset the test options |

The controller is optional. The default 16:9 layout still draws when no pad is available.

## Get the test PKG

1. Open the repository's **Actions** tab.
2. Select **KI1 Widescreen Test PKG** and open a successful run.
3. Download the `KI1-Widescreen-Test-PS4` artifact.
4. Extract the ZIP and install the included PKG using your usual PS4 homebrew package workflow.

If the workflow failed, open its build job and inspect the first compiler or packaging error before trying to install anything.

## What this test does not prove

A successful install proves only that the test application package built and launched. It does not prove that Killer Instinct boots, that the emulator runs at full speed, or that the GLSL bloom and color-grade effects work on PS4. The actual emulator core and its renderer still need to be integrated.
