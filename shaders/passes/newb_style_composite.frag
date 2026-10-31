#version 330 core

in vec2 v_uv;
layout(location = 0) out vec4 frag_color;

uniform sampler2D u_scene;
uniform sampler2D u_bloom;
uniform float u_exposure;
uniform float u_bloom_strength;
uniform float u_saturation;
uniform float u_vibrance;
uniform float u_contrast;
uniform float u_warm_highlights;
uniform float u_cool_shadows;
uniform float u_vignette;
uniform float u_effect_strength;

vec3 srgb_to_linear(vec3 c)
{
    vec3 low = c / 12.92;
    vec3 high = pow((c + 0.055) / 1.055, vec3(2.4));
    vec3 use_low = vec3(1.0) - step(vec3(0.04045), c);
    return mix(high, low, use_low);
}

vec3 linear_to_srgb(vec3 c)
{
    c = max(c, vec3(0.0));
    vec3 low = c * 12.92;
    vec3 high = 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055;
    vec3 use_low = vec3(1.0) - step(vec3(0.0031308), c);
    return mix(high, low, use_low);
}

vec3 aces_tonemap(vec3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
    vec3 original_srgb = texture(u_scene, v_uv).rgb;
    vec3 scene = srgb_to_linear(original_srgb);
    vec3 bloom = texture(u_bloom, v_uv).rgb;

    float luma = dot(scene, vec3(0.2126, 0.7152, 0.0722));
    float shadow_mask = 1.0 - smoothstep(0.06, 0.58, luma);
    float highlight_mask = smoothstep(0.38, 0.95, luma);

    vec3 graded = scene;
    graded *= u_exposure;
    graded += bloom * u_bloom_strength;

    // Cool shadows and warm highlights: a subtle arcade-friendly grade.
    graded += vec3(-0.010, 0.002, 0.016) * shadow_mask * u_cool_shadows;
    graded += vec3(0.030, 0.012, -0.010) * highlight_mask * u_warm_highlights;

    float gray = dot(graded, vec3(0.2126, 0.7152, 0.0722));
    vec3 saturated = mix(vec3(gray), graded, u_saturation);

    float max_channel = max(saturated.r, max(saturated.g, saturated.b));
    float min_channel = min(saturated.r, min(saturated.g, saturated.b));
    float chroma = max_channel - min_channel;
    float vibrance_gain = 1.0 + (1.0 - clamp(chroma, 0.0, 1.0)) * u_vibrance;
    saturated = graded + (saturated - graded) * vibrance_gain;

    saturated = (saturated - 0.18) * u_contrast + 0.18;
    vec3 mapped = aces_tonemap(max(saturated, vec3(0.0)));

    float vignette_distance = length((v_uv - 0.5) * vec2(1.0, 0.82));
    float vignette_factor = 1.0 - smoothstep(0.34, 0.78, vignette_distance) * u_vignette;
    mapped *= vignette_factor;

    vec3 effected = linear_to_srgb(mapped);
    vec3 final_color = mix(original_srgb, effected, clamp(u_effect_strength, 0.0, 1.0));

    frag_color = vec4(final_color, 1.0);
}
