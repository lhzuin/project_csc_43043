#include "shark_actor.hpp"
#include "cgp/cgp.hpp"
#include <random>
/**
 * Convenience: load, setup texture & joint groups all at once.
 */
void shark_actor::initialize(cgp::opengl_shader_structure const& shader,
                std::string const& gltf_file,
                std::string const& texture_file) {
    // load glTF
    load_from_gltf(gltf_file, shader);
    drawable.texture.load_and_initialize_texture_2d_on_gpu(
        texture_file, GL_REPEAT, GL_REPEAT);
    // define joint groups
    groups = {
        {"Tail",   {6,7,8,9,10}},
        {"Body0",  {2}},
        {"Body1",  {3,17,18,19}},
        {"Body2",  {4,15,16}},
        {"Body3",  {5,11,12,13,14}},
        {"FinL",   {20,21,22}},
        {"FinR",   {25,26,27}},
        {"Jaw",    {29,30}}
    };
}

void shark_actor::start_position(skinned_actor target_actor) {

    //random position for its origin and for its speed
    static std::mt19937 engine{ std::random_device{}() };
    std::uniform_real_distribution<float> dist_real(-10.0f, 10.0f);
    std::uniform_real_distribution<float> speed_real(2.0f, 10.0f);
    float rnd_f = dist_real(engine);
    float speed_f = speed_real(engine);

    origin = target_actor.drawable.model.translation + cgp::vec3{ rnd_f, -20.0f, 0.5f };
    target = target_actor.drawable.model.translation + cgp::vec3{ rnd_f, 0.0f, 0.5f };  // desired swim-to point
    target = 2*target - origin;
    speed  = speed_f;                // units/sec
    drawable.model.translation = origin;
}

/**
 * Swim movement + directional alignment.
 */
void shark_actor::update_position(float dt)
{
    cgp::vec3 dir = target - origin;
    float dist = cgp::norm(dir);
    if (dist < 1e-4f) return;

    dir /= dist;            // normalize
    origin += dir * speed * dt;
    drawable.model.translation = origin;
    align_to(dir);
}

/**
 * Generate wiggling animation on body, tail, fins and jaw.
 */
void shark_actor::animate(float t) {
    // Body wave
    float w = 2*cgp::Pi*body_frequency;
    std::array<std::string,4> seg = {"Body0","Body1","Body2","Body3"};
    for (int i=0;i<seg.size();++i) {
        float amp = body_amplitude * (amplitude_ratio + (1-amplitude_ratio)*(i/seg.size()));
        float phase = w*t - i*body_lag;
        rotate_group(seg[i], {0,0,1}, amp*std::sin(phase));
    }

    size_t last = seg.size() - 1;           
    // Tail
    float amp_last   = body_amplitude * (amplitude_ratio + (1-amplitude_ratio)*(last/seg.size()));
    float phase_last = w*t - last*body_lag;
    rotate_group("Tail", {0,0,1}, amp_last*std::sin(phase_last));
    // Fins
    float fin = fin_amplitude * std::sin(w*t + cgp::Pi/2);
    rotate_group("FinL",{0,1,0},  fin);
    rotate_group("FinR",{0,1,0}, -fin);
    // Jaw
    float jaw = jaw_amplitude * std::max(0.f, std::sin(w*t));
    rotate_group("Jaw",{1,0,0}, jaw);
}

bool shark_actor::check_for_collision(skinned_actor actor){
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
    constexpr float shrinkXY = 0.7f;
    constexpr float shrinkZ  = 0.8f;
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
void shark_actor::align_to(cgp::vec3 const& dir) {
    cgp::vec3 forward = {0,0,1};
    float cosang = dot(forward, dir);
    if (cgp::abs(cosang + 1.0f) < 1e-3f) {
        drawable.model.rotation = cgp::rotation_transform::from_axis_angle({0,0,1}, cgp::Pi);
    }
    else if (cgp::abs(cosang - 1.0f) < 1e-3f) {
        drawable.model.rotation = cgp::rotation_transform();
    }
    else {
        cgp::vec3 axis = normalize(cross(forward, dir));
        float    angle = acos(cosang);
        drawable.model.rotation = cgp::rotation_transform::from_axis_angle(axis, angle);
    }
}


