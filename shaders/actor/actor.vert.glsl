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
    /* --------- linear-blend skinning --------------------------------- 

    /* 1) re-normalise the four weights (protects against rounding issues) */
    vec4 w = vertex_weight;
    float invSum = inversesqrt(dot(w, w) + 1e-8);
    w *= invSum;

    /* 2) add whatever is still missing to joint 0 (root)                */
    float wRoot = clamp(1.0 - (w.x + w.y + w.z + w.w), 0.0, 1.0);

    /* 3) build the skinning matrix                                       */
    mat4 skin =
        wRoot      * uBones[0]                       +
        w.x * uBones[int(vertex_joint.x)]            +
        w.y * uBones[int(vertex_joint.y)]            +
        w.z * uBones[int(vertex_joint.z)]            +
        w.w * uBones[int(vertex_joint.w)];

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