#include "nemo_actor.hpp"
#include "cgp/cgp.hpp"
#include <random>
#include "../environment.hpp"
#include <random>

inline cgp::vec3 rotate(const rotation_transform& R, const cgp::vec3& v)
{
    return cgp::mat3(R.matrix()) * v;   // mat3-cast extracts the upper-left 3×3
}
/**
 * Convenience: load, setup texture & joint groups all at once.
 */
void nemo_actor::initialize(cgp::opengl_shader_structure const& shader,
                std::string const& gltf_file,
                std::string const& texture_file) {
    // load glTF
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



/**
 * Generate wiggling animation on body, tail, fins and jaw.
 */
void nemo_actor::animate(float t) {
	upload_pose_to_gpu();  
}


void nemo_actor::follow(const turtle_actor& turtle,
                        cgp::vec3 local_offset,
                        cgp::rotation_transform extra)
{
    /* ---------- 1) reuse turtle’s pose ---------- */
    const rotation_transform& R_turtle  = turtle.drawable.model.rotation;
    cgp::vec3                 T_turtle  = turtle.drawable.model.translation;

    /* ---------- 2) rotation: share turtle yaw/pitch,
                              keep Nemo’s own “facing right” ---------- */
    drawable.model.rotation =  R_turtle * extra * base_rotation;

    /* ---------- 3) translation:                T_world  =  T_turtle
     *                                    +  R_turtle * local_offset
     *                                    –  R_nemo   * centre * scale   */
    
    cgp::vec3 local  = rotate(drawable.model.rotation, res->center_offset) * drawable.model.scaling;

    cgp::vec3 target = turtle.drawable.model.translation
                + rotate(turtle.drawable.model.rotation, local_offset);

    drawable.model.translation = target - local;
}









