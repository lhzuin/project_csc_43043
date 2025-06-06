#version 330 core

// ── Vertex attributes from the skinned GLTF mesh ────────────────────────────
layout(location = 0) in vec3 a_position;    // POSITION
layout(location = 1) in vec3 a_normal;      // NORMAL
layout(location = 2) in vec3 a_color;       // COLOR_0
layout(location = 3) in vec2 a_uv;          // TEXCOORD_0
layout(location = 4) in uvec4 a_bone_index; // JOINTS_0
layout(location = 5) in vec4 a_bone_weight; // WEIGHTS_0

// ── Per-instance offset: each fish has its own (x,y,z) translation ─────
layout(location = 6) in vec3 instance_offset;

// ── “fragment_data” must match exactly the input struct in custom_mesh.frag ─
out struct fragment_data {
    vec3 position;  // world-space
    vec3 normal;    // world-space
    vec3 color;     // vertex color
    vec2 uv;        // texture coordinates
} fragment;

// ── Uniforms ─────────────────────────────────────────────────────────────────
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// skinning matrices
uniform mat4 uBones[64];

void main()
{
    // 1) — LINEAR-BLEND SKINNING (exactly as in your original actor.vert) —
    mat4 skin =
          a_bone_weight.x * uBones[int(a_bone_index.x)] +
          a_bone_weight.y * uBones[int(a_bone_index.y)] +
          a_bone_weight.z * uBones[int(a_bone_index.z)] +
          a_bone_weight.w * uBones[int(a_bone_index.w)];

    vec4 Pskinned = skin * vec4(a_position, 1.0);
    vec3 Nskinned = mat3(skin) * a_normal;

    // 2) — PER-INSTANCE TRANSLATION —
    //    Add the per-instance (x,y,z) before applying “model”:
    vec4 worldPos4 = model * Pskinned + vec4(instance_offset, 1.0);

    gl_Position = projection * view * worldPos4;

    // 3) — NORMAL IN WORLD SPACE —
    //    Translations do not affect normals, so we only need “model” for normal transform:
    mat3 normalMat = transpose(inverse(mat3(model)));
    vec3 worldNormal = normalize(normalMat * Nskinned);

    // 4) — PACK INTO “fragment_data” (matching custom_mesh.frag) —
    fragment.position = worldPos4.xyz;
    fragment.normal   = worldNormal;
    fragment.color    = a_color; 
    fragment.uv       = a_uv;
}