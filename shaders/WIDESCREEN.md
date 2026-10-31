# KI1 widescreen presentation

The arcade game outputs a fixed 320×240 frame (4:3). The `Wide Arcade 16:9` shader uses a centered, aspect-preserving 4:3 viewport and fills the extra horizontal space with a blurred, dimmed copy of the current frame.

On a 1920×1080 display, the game remains 1440×1080 in the center. Each side gutter is 240 pixels wide. The original frame is not stretched or cropped.

This is a widescreen presentation hack, **not a true expanded-FOV/game-code patch**. Killer Instinct's arcade video path produces a fixed 320×240 image; its game graphics and backgrounds are composed for that frame. A genuine wider camera view would require game-specific changes to how the game selects and draws its pre-rendered background frames, then extensive gameplay testing. The current emulator output does not expose an extra off-screen scene area that can simply be revealed.

## Renderer integration

1. Render the game at 320×240.
2. Apply the optional Lush Arcade color grade to the 4:3 game texture.
3. Run `passes/wide_preserve_4_3.frag` over the full 16:9 output.
4. Draw menus and OSD in the same centered 4:3 viewport, or explicitly design a separate widescreen OSD layer.

The shader source is committed but is not connected to a PS4 renderer yet. GitHub Actions validates shader syntax and JSON structure; it does not test the shader on PS4 hardware.

The CRT preset remains an optional alternative. Do not apply CRT scanlines automatically on top of Lush Arcade.
