#version 330 core
/* ────────────── vertex attributes coming from the VAO ────────────── */
layout(location = 0) in vec3  vertex_position;   // POSITION
layout(location = 1) in vec3  vertex_normal;     // NORMAL
layout(location = 2) in vec3  vertex_color;      // COLOR_0   (optional)
layout(location = 3) in vec2  vertex_uv;         // TEXCOORD_0
layout(location = 4) in uvec4 vertex_joint;      // JOINTS_0  (added)
layout(location = 5) in vec4  vertex_weight;     // WEIGHTS_0 (added)

/* ────────────── standard CGP uniforms ─────────────────────────────── */
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

/* ────────────── skinning matrices (filled from C++) ───────────────── */
uniform mat4 uBones[64];

/* ────────────── data sent to the fragment shader ──────────────────── */
out struct fragment_data
{
    vec3 position;   // world space
    vec3 normal;     // world space
    vec3 color;      // vertex colour
    vec2 uv;         // texture coordinates
} fragment;

/* -------------------------------------------------------------------- */
void main()
{
    /* --------- linear-blend skinning --------------------------------- */
    mat4 skin =
          vertex_weight.x * uBones[int(vertex_joint.x)] +
          vertex_weight.y * uBones[int(vertex_joint.y)] +
          vertex_weight.z * uBones[int(vertex_joint.z)] +
          vertex_weight.w * uBones[int(vertex_joint.w)];

    vec4 Pskinned = skin * vec4(vertex_position, 1.0);
    vec3 Nskinned = mat3(skin) * vertex_normal;

    /* --------- to world space, then to clip space -------------------- */
    vec4 Pworld = model * Pskinned;
    gl_Position = projection * view * Pworld;

    /* --------- pass to fragment shader ------------------------------- */
    fragment.position = Pworld.xyz;
    fragment.normal   = normalize(mat3(model) * Nskinned);
    fragment.color    = vertex_color;          // keep original colour
    fragment.uv       = vertex_uv;
}