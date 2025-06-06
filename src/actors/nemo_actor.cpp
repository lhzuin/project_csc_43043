#include "nemo_actor.hpp"
#include "cgp/cgp.hpp"
#include <random>
#include "../environment.hpp"

inline cgp::vec3 rotate(const rotation_transform& R, const cgp::vec3& v)
{
    return cgp::mat3(R.matrix()) * v;
}

void nemo_actor::initialize(cgp::opengl_shader_structure const& shader,
                std::string const& gltf_file,
                std::string const& texture_file) {
    load_from_gltf(gltf_file, shader);
    drawable.texture.load_and_initialize_texture_2d_on_gpu(
        texture_file, GL_REPEAT, GL_REPEAT);

    float s = 1.0f / 1600.0f;
    drawable.model.scaling = s;

    base_rotation = rotation_transform::from_axis_angle({1,0,0},  -Pi/2)*rotation_transform::from_axis_angle({0,0,1},  Pi) * rotation_transform::from_axis_angle({1,0,0},  Pi/2);


}

void nemo_actor::start_position() {
    drawable.model.rotation = base_rotation;

    upload_pose_to_gpu();
}




void nemo_actor::animate(float t) {
	upload_pose_to_gpu();  
}


void nemo_actor::follow(const turtle_actor& turtle,
                        cgp::vec3 local_offset,
                        cgp::rotation_transform extra)
{
    // Reuse turtle’s pose
    const rotation_transform& R_turtle  = turtle.drawable.model.rotation;
    cgp::vec3                 T_turtle  = turtle.drawable.model.translation;

    // Rotation: share turtle yaw/pitch to keep Nemo's own “facing right” ---------- */
    drawable.model.rotation =  R_turtle * extra * base_rotation;

    // Translation:                
    cgp::vec3 local  = rotate(drawable.model.rotation, res->center_offset) * drawable.model.scaling;

    cgp::vec3 target = turtle.drawable.model.translation
                + rotate(turtle.drawable.model.rotation, local_offset);

    drawable.model.translation = target - local;
}









