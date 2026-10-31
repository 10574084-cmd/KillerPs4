# KI1 post-processing shaders

These are original GLSL 3.30 post-processing shaders for the planned PS4 presentation backend. They are inspired by the broad visual goals of Minecraft's Newb Shader — richer lighting, warm/cool color treatment and soft highlights — but do not copy its shader source, assets or presets.

## Presets

### Lush Arcade

A multi-pass bloom and color-grading treatment:

1. Render the native 320×240 game image.
2. Extract highlights into a half-resolution linear-light texture.
3. Blur the highlight texture horizontally.
4. Blur it vertically.
5. Composite the original frame with bloom, a subtle cool-shadow/warm-highlight grade, vibrance, ACES tone mapping and a restrained vignette.

The result aims for a brighter, more colorful and softly illuminated look without changing the arcade's timing or game logic.

### Arcade CRT

An optional alternative with scanlines, a subtle RGB aperture mask and vignette. It is not applied on top of Lush Arcade by default.

## Important limits

Killer Instinct's emulated video output is a flat 2D 320×240 frame. It does not expose per-object normals, depth, material maps or world geometry to this post-process. Therefore these shaders can add color grading, glow and screen-space effects, but cannot create physically correct cast shadows, moving environment objects, reflective water or volumetric clouds. Those effects would require scene data the arcade game does not provide.

The shader files target GLSL 3.30 Core. A PS4 renderer still needs to create the GL context, fullscreen geometry, source texture, framebuffers and uniforms, and load these files at runtime. This repository currently does not have that renderer wired into a game loop, so adding these files does not yet make the effects appear in a playable build.

## Performance plan

- Keep the native scene texture at 320×240.
- Use half-resolution bloom buffers (160×120) and separable blur.
- Run the final composite once at display resolution.
- Allow the preset to be disabled or reduced for performance.
- Preserve a fitted 4:3 viewport on 16:9 displays; never stretch the game image non-uniformly.

## Validation

Run:

```sh
sh tools/test_shaders.sh
```

The script compiles all vertex and fragment shader files with `glslangValidator`.
