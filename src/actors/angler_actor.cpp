#include "angler_actor.hpp"
#include "cgp/cgp.hpp"
#include <random>
/**
 * Convenience: load, setup texture & joint groups all at once.
 */
void angler_actor::initialize(cgp::opengl_shader_structure const& shader,
                std::string const& gltf_file,
                std::string const& texture_file) {
    // load glTF
    load_from_gltf(gltf_file, shader);
    drawable.texture.load_and_initialize_texture_2d_on_gpu(
        texture_file, GL_REPEAT, GL_REPEAT);
    // define joint groups
    groups = {
        {"Tail0",  { 8 }},          // tail_1_06
        {"Tail1",  { 9 }},          // tail_2_09
        {"Tail2",  {10 }},          // tail_3_08
        {"FinL",   {11,12}},        // left fins
        {"FinR",   {13,14}},        // right fins
        {"Jaw",    {2,5,7}},//{ 5, 6, 7}},       // first two mouth bones
        {"Lamp",   { 4 }}           // lantern_1_02 (tip will follow)
    };

    float s = 1.0f / 40.0f;
    drawable.model.scaling = s;
    base_rotation = cgp::rotation_transform::from_axis_angle({0,0,1},  cgp::Pi);
}
auto phase = [](float w, float t, float lag){ return w*t - lag; };
void angler_actor::start_position(skinned_actor const& target_actor) {
    drawable.model.rotation = base_rotation;

    //random position for its origin and for its speed
    static std::mt19937 engine{ std::random_device{}() };
    std::uniform_real_distribution<float> dist_real(-5.0f, 5.0f);
    std::uniform_real_distribution<float> dist_real2(-1.0f, 1.0f);
    std::uniform_real_distribution<float> speed_real(2.0f, 8.0f);
    float rnd_f = dist_real(engine);
    float rnd_f_target = dist_real(engine);
    float speed_f = speed_real(engine);

    origin = target_actor.drawable.model.translation + cgp::vec3{ rnd_f, 20.0f, 0.5f };
    target = target_actor.drawable.model.translation + cgp::vec3{ rnd_f_target, 0.0f, 0.5f };  // desired swim-to point
    target = 2*target - origin;
    speed  = speed_f;                // units/sec
    drawable.model.translation = origin;
    upload_pose_to_gpu();
}

/**
 * Generate wiggling animation on body, tail, fins and jaw.
 */
void angler_actor::animate(float t)
{
    const float w = 2.0f * cgp::Pi * tail_frequency;       // angular speed

    /* --- (a) tail : travelling wave ------------------------------------- 
    rotate_group("Tail0", {0,0,1},  tail_amplitude * sin( phase(w,t,0.0f) ));
    rotate_group("Tail1", {0,0,1},  tail_amplitude * sin( phase(w,t,0.6f) ));
    rotate_group("Tail2", {0,0,1},  tail_amplitude * sin( phase(w,t,1.2f) ));*/

    /* --- (b) pectoral fins --------------------------------------------- */
    float fin = fin_amplitude * sin(w*t + cgp::Pi*0.5f);
    //rotate_group("FinL", {0,1,0},  fin);    // mirror them  
    //rotate_group("FinR", {0,1,0}, -fin);

    /* --- (c) jaw -------------------------------------------------------- */
    float jaw = jaw_amplitude * cgp::clamp( std::sin(w*t), 0.f, 1.f );  // open only half cycle
    rotate_group("Jaw", {0,1,0}, jaw);

    upload_pose_to_gpu();          // push updated matrices to the shader
}

bool angler_actor::check_for_collision(skinned_actor  const& actor){
    // 1) Get shark‐local frame and world‐space centers of each actor’s bounding‐box center:
    cgp::mat4 M1     = drawable.model.matrix();
    cgp::vec3 C1     = (M1 * cgp::vec4(res->center_offset, 1)).xyz();
    cgp::mat4 M2     = actor.drawable.model.matrix();
    cgp::vec3 C2     = (M2 * cgp::vec4(actor.res->center_offset, 1)).xyz();

    // 2) World‐space delta
    cgp::vec3 d_world = C2 - C1;

    // 3) Transform delta into shark’s local rotated+scaled space:
    //    since M1 = T·R·S, we undo R·S by applying (R·S)⁻¹ = S⁻¹·Rᵀ
    cgp::mat3 RS    = cgp::mat3(M1);         // contains rotation * scale
    cgp::mat3 invRS = cgp::inverse(RS);      // S⁻¹·Rᵀ
    cgp::vec3 d_loc = invRS * d_world;

    // 4) Cylinder dimensions (shark) in its local space:
    //    shrinkXY lets you “cut off” fins, shrinkZ shortens the height if desired
    constexpr float shrinkXY = 1.0f;
    constexpr float shrinkZ  = 0.6f;
    cgp::vec3   E1     = res->half_extents;               // (Ex, Ey, Ez)
    float  radius = std::max(E1.x, E1.y) * shrinkXY; // cylinder radius
    float  halfH  = E1.z * shrinkZ;                  // cylinder half‐height

    // 5) Box dimensions (other actor) in *shark‐local* axes:
    //    we’ll treat it as an AABB in this same frame
    cgp::vec3   E2     = actor.res->half_extents;         

    // 6) Horizontal (X–Y) distance from cylinder axis to box:
    //    if the box spans [–E2.x, +E2.x] in X, the closest X on the box to the axis is:
    float dx = std::max(std::abs(d_loc.x) - E2.x, 0.0f);
    float dy = std::max(std::abs(d_loc.y) - E2.y, 0.0f);
    bool  overlapXY = (dx*dx + dy*dy) <= (radius*radius);

    // 7) Vertical overlap (Z‐axis):
    //    cylinder is [–halfH, +halfH], box is [d_loc.z–E2.z, d_loc.z+E2.z]
    bool  overlapZ  = std::abs(d_loc.z) <= (halfH + E2.z);

    return overlapXY && overlapZ;
}


/// Align the mesh forward (-Y) to given direction
void angler_actor::align_to(cgp::vec3 const& dir) {
    cgp::vec3 forward = {0,0,1};
    float cosang = dot(forward, dir);
    if (cgp::abs(cosang + 1.0f) < 1e-3f) {
        drawable.model.rotation = base_rotation*cgp::rotation_transform::from_axis_angle({0,0,1}, cgp::Pi);
    }
    else if (cgp::abs(cosang - 1.0f) < 1e-3f) {
        drawable.model.rotation = base_rotation*cgp::rotation_transform();
    }
    else {
        cgp::vec3 axis = normalize(cross(forward, dir));
        float    angle = acos(cosang);
        drawable.model.rotation = base_rotation*cgp::rotation_transform::from_axis_angle(axis, angle);
    }
}


