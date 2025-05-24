#include "turtle_actor.hpp"
#include "cgp/cgp.hpp"
#include <random>
#include "environment.hpp"
/**
 * Convenience: load, setup texture & joint groups all at once.
 */
void turtle_actor::initialize(cgp::opengl_shader_structure const& shader,
                std::string const& gltf_file,
                std::string const& texture_file) {
    // load glTF
    load_from_gltf(project::path + gltf_file, shader);
    drawable.texture.load_and_initialize_texture_2d_on_gpu(
        project::path + texture_file, GL_REPEAT, GL_REPEAT);

    data = mesh_load_file_gltf(project::path + "assets/sea_turtle/sea_turtle.gltf");

    // define joint groups
    groups["RF"] = { 2,  3,  4,  5 };   // right-front flipper
    groups["RR"] = { 6,  7,  8,  9 };   // right-rear
    groups["LF"] = { 10, 11, 12, 13 };   // left-front
    groups["LR"] = { 14, 15, 16, 17 };   // left-rear
}

void turtle_actor::start_position() {

    drawable.model.rotation = rotation_transform::from_axis_angle({ 1, 0, 0 }, Pi / 2.0f);
    vec3 turtle_pos = { 0.2f, 0.4f, 0.5f };
    drawable.model.translation = turtle_pos;
    upload_pose_to_gpu();
}



/**
 * Generate wiggling animation on body, tail, fins and jaw.
 */
void turtle_actor::animate(float t) {
    aFront = front_amplitude * std::sin( front_frequency * t );        // front pair
    aRear  = rear_amplitude * std::sin( rear_frequency * t + cgp::Pi ); // rear 180°

    reset_pose();
    rotate_group("RF", {0,0,1},  aFront);
    rotate_group("LF", {0,0,1},  aFront);
    rotate_group("RR", {0,0,1},  aRear );
    rotate_group("LR", {0,0,1},  aRear );
	upload_pose_to_gpu();  
}


void turtle_actor::move(vec3 const& direction)
{
    // 1) shift the turtle’s position
    drawable.model.translation += direction;
    
    // 2) re-compute camera to keep the same offset
    //vec3 pos = turtle.drawable.model.translation;
    
    ////camera_control.look_at(pos + camera_offset, pos, { 0,0,1 });
    //camera_control.look_at(
    //    turtle.drawable.model.translation + camera_offset,
    //    turtle.drawable.model.translation, { 0,0,1 }
    //);
}




