#include "shark_actor.hpp"
#include "cgp/cgp.hpp"

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
    origin = target_actor.drawable.model.translation + cgp::vec3{ 0.0f, 20.0f, 0.5f };
    target = target_actor.drawable.model.translation + cgp::vec3{ 0.0f, 0.0f, 0.5f };  // desired swim-to point
    speed  = 2.0f;                // units/sec
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
    auto T = actor.drawable.model.translation;
    auto S = drawable.model.translation;
    float d = cgp::norm(drawable.model.translation - actor.drawable.model.translation);
    return d < actor.radius + radius;
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


