#version 330 core

in vec2 v_uv;
layout(location = 0) out vec4 frag_color;

uniform sampler2D u_source;
uniform vec2 u_texel_size;
uniform vec2 u_direction;

void main()
{
    vec2 step_uv = u_texel_size * u_direction;

    vec3 color = texture(u_source, v_uv).rgb * 0.2270270270;
    color += texture(u_source, v_uv + step_uv * 1.0).rgb * 0.1945945946;
    color += texture(u_source, v_uv - step_uv * 1.0).rgb * 0.1945945946;
    color += texture(u_source, v_uv + step_uv * 2.0).rgb * 0.1216216216;
    color += texture(u_source, v_uv - step_uv * 2.0).rgb * 0.1216216216;
    color += texture(u_source, v_uv + step_uv * 3.0).rgb * 0.0540540541;
    color += texture(u_source, v_uv - step_uv * 3.0).rgb * 0.0540540541;
    color += texture(u_source, v_uv + step_uv * 4.0).rgb * 0.0162162162;
    color += texture(u_source, v_uv - step_uv * 4.0).rgb * 0.0162162162;

    frag_color = vec4(color, 1.0);
}
