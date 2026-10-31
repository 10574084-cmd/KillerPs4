#version 330 core

in vec2 v_uv;
layout(location = 0) out vec4 frag_color;

uniform sampler2D u_scene;
uniform vec2 u_output_size;
uniform float u_scanline_strength;
uniform float u_mask_strength;
uniform float u_vignette;

void main()
{
    vec3 color = texture(u_scene, v_uv).rgb;

    float row = mod(floor(gl_FragCoord.y), 3.0);
    float scan = row < 1.0 ? 1.0 - u_scanline_strength : 1.0;

    float column = mod(floor(gl_FragCoord.x), 3.0);
    vec3 mask = vec3(1.0);
    if (column < 1.0) {
        mask = vec3(1.0, 1.0 - u_mask_strength, 1.0 - u_mask_strength);
    } else if (column < 2.0) {
        mask = vec3(1.0 - u_mask_strength, 1.0, 1.0 - u_mask_strength);
    } else {
        mask = vec3(1.0 - u_mask_strength, 1.0 - u_mask_strength, 1.0);
    }

    float d = length((v_uv - 0.5) * vec2(1.0, 0.82));
    float edge = 1.0 - smoothstep(0.40, 0.78, d) * u_vignette;

    // Avoid visible mask striping when output resolution is too low.
    float mask_scale = step(900.0, u_output_size.x);
    vec3 final_color = color * scan * mix(vec3(1.0), mask, mask_scale) * edge;

    frag_color = vec4(final_color, 1.0);
}
