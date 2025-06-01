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
    load_from_gltf(project::path + gltf_file, shader);
    drawable.texture.load_and_initialize_texture_2d_on_gpu(
        project::path + texture_file, GL_REPEAT, GL_REPEAT);

    base_rotation = rotation_transform::from_axis_angle({ 1, 0, 0 }, Pi / 2.0f);
    data = mesh_load_file_gltf(project::path + "assets/sea_turtle/sea_turtle.gltf");

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


//void turtle_actor::move(vec3 const& direction)
//{
//    drawable.model.translation += direction;
//
//    const float yaw_sensitivity = 4.0f;
//    const float pitch_sensitivity = 6.0f;
//
//    float dx = direction.x;
//    float dz = direction.z;
//
//    // start from your base (or last) orientation:
//    rotation_transform R = base_rotation; 
//
//    if (std::abs(dx) > 1e-5f && std::abs(dz) < 1e-5f) {
//        float yawAngle = yaw_sensitivity * dx;
//        // bank around Y **in turtle-local space**:
//        R = rotation_transform::from_axis_angle({ 0,1,0 }, yawAngle) * R;
//    }
//    else if (std::abs(dz) > 1e-5f && std::abs(dx) < 1e-5f) {
//        float pitchAngle = pitch_sensitivity * dz;
//        // pitch around X **in turtle-local space**:
//        R = rotation_transform::from_axis_angle({ 1,0,0 }, pitchAngle) * R;
//    }
//    // else R stays as base_rotation
//
//    drawable.model.rotation = R;
//    upload_pose_to_gpu();
//}

void turtle_actor::move(vec3 const& direction)
{
    // 1) slide the turtle
    base_translation += direction;

    // 2) compute targets based on your original sensitivities
    const float yaw_sensitivity = 20.0f;  // X → bank
    const float pitch_sensitivity = 20.0f;  // Z → pitch
    float target_yaw = yaw_sensitivity * direction.x;
    float target_pitch = pitch_sensitivity * direction.z;

    // 3) ease the current angles toward the targets
    current_yaw += (target_yaw - current_yaw) * smoothing;
    current_pitch += (target_pitch - current_pitch) * smoothing;

    // 4) rebuild your rotation in the same order you had before:
    //    yaw around Y then pitch around X, both pre‐multiplied on your base
    rotation_transform R = base_rotation;
    R = rotation_transform::from_axis_angle({ 0,1,0 }, current_yaw) * R;
    R = rotation_transform::from_axis_angle({ 1,0,0 }, current_pitch) * R;

    // 5) apply & upload
    drawable.model.rotation = R;
}









