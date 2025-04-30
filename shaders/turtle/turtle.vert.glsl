#version 330 core
layout(location = 0) in vec3  vertex_position;
layout(location = 1) in vec3  vertex_normal;
layout(location = 2) in vec3  vertex_color;        // not used, but keep it
layout(location = 3) in vec2  vertex_uv;

uniform mat4  model;
uniform mat4  view;
uniform mat4  projection;

uniform float uTime;

uniform float uPivotX   = 0.6;
uniform float uRange    = 0.4;
uniform float uAmplitude= 0.6;
uniform float uFreq     = 2.5;

out struct fragment_data {
    vec3 position;
    vec3 normal;
    vec3 color;
    vec2 uv;
} fragment;

vec3 flap(vec3 p)
{
    float w = clamp(1.0 - abs(p.x - uPivotX)/uRange, 0.0, 1.0);
    float angle = uAmplitude * w * sin(uFreq * uTime) * sign(p.x - uPivotX);
    mat2 R = mat2(cos(angle), -sin(angle),
                  sin(angle),  cos(angle));
    vec2 yz = R * vec2(p.y, p.z);
    return vec3(p.x, yz.x, yz.y);
}

void main()
{
    vec3 pos = flap(vertex_position);
    vec3 nor = flap(vertex_position + vertex_normal) - flap(vertex_position);

    vec4 Pworld = model * vec4(pos,1.0);
    gl_Position = projection * view * Pworld;

    fragment.position = Pworld.xyz;
    fragment.normal   = normalize(mat3(model) * nor);
    fragment.color    = vec3(1.0);        // or vertex_color if you wish
    fragment.uv       = vertex_uv;        // now the real UVs
}