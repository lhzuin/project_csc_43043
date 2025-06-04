#include "turtle_actor.hpp"
#include "cgp/cgp.hpp"
#include <random>
#include "../environment.hpp"
#include <random>
/**
 * Convenience: load, setup texture & joint groups all at once.
 */

rotation_transform base_rotation = rotation_transform::from_axis_angle({ 1, 0, 0 }, Pi / 2.0f);

void turtle_actor::initialize(cgp::opengl_shader_structure const& shader,
                std::string const& gltf_file,
                std::string const& texture_file) {
    // load glTF
    load_from_gltf(gltf_file, shader);
    drawable.texture.load_and_initialize_texture_2d_on_gpu(
        texture_file, GL_REPEAT, GL_REPEAT);

    base_rotation = rotation_transform::from_axis_angle({ 1, 0, 0 }, Pi / 2.0f);
    data = mesh_load_file_gltf( gltf_file);

    // define joint groups
    groups["RF"] = { 2,  3,  4,  5 };   // right-front flipper
    groups["RR"] = { 6,  7,  8,  9 };   // right-rear
    groups["LF"] = { 10, 11, 12, 13 };   // left-front
    groups["LR"] = { 14, 15, 16, 17 };   // left-rear


    {
        static std::mt19937 eng{ std::random_device{}() };
        std::uniform_real_distribution<float> U(0.0f, 2.0f * cgp::Pi);
        osc_phase_x = U(eng);
        osc_phase_y = U(eng);
    }
}

void turtle_actor::start_position() {
    drawable.model.rotation = base_rotation;

    vec3 initial_pos = { 0.2f, 0.4f, 0.5f };
    base_translation = initial_pos;

    drawable.model.translation = base_translation;

    upload_pose_to_gpu();
}



/**
 * Generate wiggling animation on body, tail, fins and jaw.
 */
void turtle_actor::animate(float t) {
    // ─── 1) Compute the small involuntary drift around the base position ───────
    // dx = A_x * sin(2π * f_x * t + phase_x), same for dy
    float dx = osc_amp_x * std::sin(2.0f * cgp::Pi * osc_freq_x * t + osc_phase_x);
    float dy = osc_amp_y * std::sin(2.0f * cgp::Pi * osc_freq_y * t + osc_phase_y);

    // We assume Z is “up,” so no vertical oscillation (or set dz=0 if you did not want Z):
    float dz = 0.0f;

    // Set drawable.model.translation = base_translation + (dx, dy, dz)
    drawable.model.translation = base_translation + cgp::vec3{dx, dy, dz};
    
    aFront = front_amplitude * std::sin( front_frequency * t );        // front pair
    aRear  = rear_amplitude * std::sin( rear_frequency * t + cgp::Pi ); // rear 180°

    reset_pose();
    rotate_group("RF", {0,0,1},  aFront);
    rotate_group("LF", {0,0,1},  aFront);
    rotate_group("RR", {0,0,1},  aRear );
    rotate_group("LR", {0,0,1},  aRear );
	upload_pose_to_gpu();  
}


void turtle_actor::move(vec3 const& direction, float dt)
{
    // 1) slide the turtle in world‐space exactly as before
    base_translation += direction;

    // 2) compute raw targets based on direction.x/z
    const float yaw_sensitivity = 10.0f; // degrees of bank per unit of direction.x
    const float pitch_sensitivity = 10.0f; // degrees of pitch per unit of direction.z

    float target_yaw = yaw_sensitivity * direction.x; // e.g. if direction.x == 1 → 10° bank
    float target_pitch = pitch_sensitivity * direction.z; // e.g. if direction.z == 1 → 10° pitch

    // 3) damping: use a dt‐based smoothing coefficient
    //    (so that behavior is framerate‐independent).
    //    “damping” here is how quickly we chase the target each second.
    //    You can tweak damping_strength to taste (higher → faster response).
    const float damping_strength = 2.5f;
    // Convert to a per‐frame “blend” factor: 
    float blend = 1.0f - std::exp(-damping_strength * dt/1000);
    // Now do an exponential‐smoothing step:
    current_yaw = current_yaw + (target_yaw - current_yaw) * blend;
    current_pitch = current_pitch + (target_pitch - current_pitch) * blend;

    // 4) if the user has released the key (direction.x/z ≈ 0), 
    //    and we're very close to zero, just snap to zero so we don't float:
    if (std::abs(current_yaw) < 0.01f)   current_yaw = 0.0f;
    if (std::abs(current_pitch) < 0.01f) current_pitch = 0.0f;

    // 5) clamp so we never tilt more than +/- 45 degrees (you can change this)
    const float max_bank = 45.0f * cgp::Pi / 180.0f; // 45° in radians
    const float max_pitch = 30.0f * cgp::Pi / 180.0f; // for example, only allow ±30° pitch

    current_yaw = std::clamp(current_yaw, -max_bank, max_bank);
    current_pitch = std::clamp(current_pitch, -max_pitch, max_pitch);

    // 6) rebuild the rotation exactly as before (pre‐multiply yaw then pitch on base_rotation)
    rotation_transform R = base_rotation;
    R = rotation_transform::from_axis_angle({ 0,1,0 }, current_yaw) * R; // bank (yaw)
    R = rotation_transform::from_axis_angle({ 1,0,0 }, current_pitch) * R; // pitch

    drawable.model.rotation = R;
}











