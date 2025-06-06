#include "turtle_actor.hpp"
#include "cgp/cgp.hpp"
#include <random>
#include "../environment.hpp"

rotation_transform base_rotation = rotation_transform::from_axis_angle({ 1, 0, 0 }, Pi / 2.0f);

void turtle_actor::initialize(cgp::opengl_shader_structure const& shader,
                std::string const& gltf_file,
                std::string const& texture_file) {
    load_from_gltf(gltf_file, shader);
    drawable.texture.load_and_initialize_texture_2d_on_gpu(
        texture_file, GL_REPEAT, GL_REPEAT);

    base_rotation = rotation_transform::from_axis_angle({ 1, 0, 0 }, Pi / 2.0f);
    data = mesh_load_file_gltf( gltf_file);

    groups["RF"] = { 2,  3,  4,  5 };
    groups["RR"] = { 6,  7,  8,  9 }; 
    groups["LF"] = { 10, 11, 12, 13 };
    groups["LR"] = { 14, 15, 16, 17 }; 


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


void turtle_actor::animate(float t) {
    // Compute the small involuntary drift around the base position 
    float dx = osc_amp_x * std::sin(2.0f * cgp::Pi * osc_freq_x * t + osc_phase_x);
    float dy = osc_amp_y * std::sin(2.0f * cgp::Pi * osc_freq_y * t + osc_phase_y);

    float dz = 0.0f;

    drawable.model.translation = base_translation + cgp::vec3{dx, dy, dz};
    
    aFront = front_amplitude * std::sin( front_frequency * t );   
    aRear  = rear_amplitude * std::sin( rear_frequency * t + cgp::Pi );

    reset_pose();
    rotate_group("RF", {0,0,1},  aFront);
    rotate_group("LF", {0,0,1},  aFront);
    rotate_group("RR", {0,0,1},  aRear );
    rotate_group("LR", {0,0,1},  aRear );
	upload_pose_to_gpu();  
}


void turtle_actor::move(vec3 const& direction, float dt)
{
    // Slide the turtle in world‐space
    base_translation += direction;

    // Compute raw targets based on direction.x/z
    const float yaw_sensitivity = 10.0f;
    const float pitch_sensitivity = 10.0f;

    float target_yaw = yaw_sensitivity * direction.x; 
    float target_pitch = pitch_sensitivity * direction.z; 

    // Damping: use a dt‐based smoothing coefficient
    const float damping_strength = 2.5f;
    // Convert to a per‐frame “blend” factor: 
    float blend = 1.0f - std::exp(-damping_strength * dt);
    // Exponential‐smoothing:
    current_yaw = current_yaw + (target_yaw - current_yaw) * blend;
    current_pitch = current_pitch + (target_pitch - current_pitch) * blend;

    // If the user has released the key and we're very close to zero:
    if (std::abs(current_yaw) < 0.01f)   current_yaw = 0.0f;
    if (std::abs(current_pitch) < 0.01f) current_pitch = 0.0f;

    // Clamp so we never tilt too much
    const float max_bank = 45.0f * cgp::Pi / 180.0f; 
    const float max_pitch = 30.0f * cgp::Pi / 180.0f;

    current_yaw = std::clamp(current_yaw, -max_bank, max_bank);
    current_pitch = std::clamp(current_pitch, -max_pitch, max_pitch);

    // Rebuild the rotation
    rotation_transform R = base_rotation;
    R = rotation_transform::from_axis_angle({ 0,1,0 }, current_yaw) * R; 
    R = rotation_transform::from_axis_angle({ 1,0,0 }, current_pitch) * R; 

    drawable.model.rotation = R;
}











