#version 330 core

in vec2 v_uv;
layout(location = 0) out vec4 frag_color;

uniform sampler2D u_scene;
uniform float u_threshold;
uniform float u_soft_knee;

vec3 srgb_to_linear(vec3 c)
{
    vec3 low = c / 12.92;
    vec3 high = pow((c + 0.055) / 1.055, vec3(2.4));
    return mix(high, low, lessThanEqual(c, vec3(0.04045)));
}

void main()
{
    vec3 scene = srgb_to_linear(texture(u_scene, v_uv).rgb);
    float brightness = max(scene.r, max(scene.g, scene.b));

    float knee = max(u_soft_knee, 0.0001);
    float soft = clamp(brightness - u_threshold + knee, 0.0, 2.0 * knee);
    soft = (soft * soft) / (4.0 * knee);

    float contribution = max(brightness - u_threshold, soft);
    contribution /= max(brightness, 0.0001);

    frag_color = vec4(scene * contribution, 1.0);
}
