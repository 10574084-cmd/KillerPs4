#version 330 core

in vec2 v_uv;
layout(location = 0) out vec4 frag_color;

uniform sampler2D u_scene;
uniform vec2 u_source_texel;
uniform vec2 u_output_size;
uniform float u_blur_pixels;
uniform float u_side_brightness;
uniform float u_side_saturation;
uniform float u_side_vignette;

const float GAME_ASPECT = 4.0 / 3.0;

vec3 blurred_backdrop(vec2 uv)
{
    vec2 offset = u_source_texel * max(u_blur_pixels, 0.0);

    vec3 color = texture(u_scene, uv).rgb * 0.25;
    color += texture(u_scene, uv + vec2( offset.x, 0.0)).rgb * 0.125;
    color += texture(u_scene, uv + vec2(-offset.x, 0.0)).rgb * 0.125;
    color += texture(u_scene, uv + vec2(0.0,  offset.y)).rgb * 0.125;
    color += texture(u_scene, uv + vec2(0.0, -offset.y)).rgb * 0.125;

    color += texture(u_scene, uv + vec2( offset.x,  offset.y)).rgb * 0.0625;
    color += texture(u_scene, uv + vec2(-offset.x,  offset.y)).rgb * 0.0625;
    color += texture(u_scene, uv + vec2( offset.x, -offset.y)).rgb * 0.0625;
    color += texture(u_scene, uv + vec2(-offset.x, -offset.y)).rgb * 0.0625;

    return color;
}

void main()
{
    float output_aspect = u_output_size.x / max(u_output_size.y, 1.0);

    // Fit the complete game frame into the output. For a 16:9 display
    // this produces a centered 4:3 viewport exactly 75% of output width.
    vec2 game_size = vec2(1.0);
    if (output_aspect > GAME_ASPECT) {
        game_size.x = GAME_ASPECT / output_aspect;
    } else {
        game_size.y = output_aspect / GAME_ASPECT;
    }

    vec2 game_uv = (v_uv - 0.5) / game_size + 0.5;
    bool inside_game = all(greaterThanEqual(game_uv, vec2(0.0))) &&
                       all(lessThanEqual(game_uv, vec2(1.0)));

    if (inside_game) {
        // The original 320x240 frame is sampled without changing its aspect.
        frag_color = vec4(texture(u_scene, game_uv).rgb, 1.0);
        return;
    }

    // Fill the side gutters with a cropped, blurred copy of the game frame.
    // The crop uses "cover" mapping; no part of the main game viewport is stretched.
    vec2 backdrop_scale = vec2(
        min(1.0, output_aspect / GAME_ASPECT),
        min(1.0, GAME_ASPECT / output_aspect)
    );
    vec2 backdrop_uv = (v_uv - 0.5) * backdrop_scale + 0.5;

    vec3 backdrop = blurred_backdrop(backdrop_uv);
    float luminance = dot(backdrop, vec3(0.2126, 0.7152, 0.0722));
    backdrop = mix(vec3(luminance), backdrop, clamp(u_side_saturation, 0.0, 1.5));

    float half_gutter = max(0.5 * (1.0 - game_size.x), 0.0001);
    float side_distance = abs(v_uv.x - 0.5) - 0.5 * game_size.x;
    float side_position = clamp(side_distance / half_gutter, 0.0, 1.0);
    float edge_dim = mix(1.0, 1.0 - clamp(u_side_vignette, 0.0, 1.0),
                         smoothstep(0.1, 1.0, side_position));
    float brightness = mix(u_side_brightness, u_side_brightness * 0.78,
                           smoothstep(0.0, 1.0, side_position));

    frag_color = vec4(backdrop * brightness * edge_dim, 1.0);
}
