#version 330 core

// ────── Vertex attributes ────────────────────────────────────────────────
layout(location = 0) in vec3 vertex_position;  // from sphere VBO
layout(location = 1) in vec3 vertex_normal;    // from sphere VBO
layout(location = 2) in vec3 vertex_color;     // unused (sphere has no vertex color)
layout(location = 3) in vec2 vertex_uv;        // unused
layout(location = 4) in vec3 instance_seed3;

// ────── Outputs to fragment shader ───────────────────────────────────────
out vec3 world_position;
out vec3 world_normal;

// ────── Uniforms ───────────────────────────────────────────────────────────
uniform mat4 model;           // identity matrix
uniform mat4 view;
uniform mat4 projection;

uniform float spread_radius;   // how far in X/Y
uniform float speed;           // how fast toward −Z
uniform float fall_depth;      // wrap‐around depth
uniform float swirl_strength;  // swirl intensity
uniform float time;

// ────── Fog / light (passed to FS) ────────────────────────────────────────
// (not needed here; FS will recompute camera from view & use the same fog uniforms)

//
// A quick GLSL “random” based on a float:
//
float rand1(in float x) {
    return fract(sin(x * 12.9898) * 43758.5453);
}

// Generate a random point in a disk of radius = spread_radius:
vec2 randomDisk(in float s) {
    float angle  = 6.2831853 * rand1(s + 0.123);
    float radius = spread_radius * sqrt(rand1(s + 0.456));
    return vec2(cos(angle), sin(angle)) * radius;
}

void main()
{
    float instance_seed = instance_seed3.x;
    // ─── 1) Compute per‐instance Z “initial offset” and current Z position ───────

    // (A) generate each instance’s “starting Z” once
    float z_offset0 = fall_depth * rand1(instance_seed + 0.789);

    // (B) now use the real time uniform to drive downward motion
    float z_raw = z_offset0 + speed * time;

    // (C) wrap around so that when z < –fall_depth it jumps back up to 0
    float z_final = -fract( z_raw / fall_depth ) * fall_depth + fall_depth - 1;

    // ─── 2) Spread in X/Y on disk ────────────────────────────────────────────────
    vec2 baseXY = randomDisk(instance_seed);

    // ─── 3) Add swirl in X/Y ───────────────────────────────────────────────────
    float swirl_phase = 6.2831853 * rand1(instance_seed + 1.234);
    float swirl = swirl_strength * sin(time * 0.5 + swirl_phase);
    float ca = cos(swirl);
    float sa = sin(swirl);
    vec2 XY = vec2(
        baseXY.x * ca - baseXY.y * sa,
        baseXY.x * sa + baseXY.y * ca
    );

    // ─── 4) Build per‐instance translation matrix T ──────────────────────────────
    mat4 T = mat4(1.0);
    T[3].x = XY.x;
    T[3].y = z_final;
    T[3].z = XY.y;

    // 7) final position:
    vec4 worldPos = model * T * vec4(vertex_position, 1.0);
    gl_Position   = projection * view * worldPos;

    // 8) pass normal and world_pos to fragment:
    world_position = worldPos.xyz;
    mat3 normalMat = transpose(inverse(mat3(model * T)));
    world_normal   = normalize(normalMat * vertex_normal);
}