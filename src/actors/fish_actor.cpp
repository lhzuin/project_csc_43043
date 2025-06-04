#include "fish_actor.hpp"
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
void fish_actor::initialize(cgp::opengl_shader_structure const& shader,
                  std::string const& gltf_file,
                  std::string const& texture_file) {
     // ─── (1) Load the skinned GLTF exactly as before ─────────────────────────
     load_from_gltf(gltf_file, shader);
     drawable.texture.load_and_initialize_texture_2d_on_gpu(
         texture_file, GL_REPEAT, GL_REPEAT);

     float s = 1.0f/100.0f;
     drawable.model.scaling = s;

     base_rotation = 
         rotation_transform::from_axis_angle({0,0,1}, Pi)
       * rotation_transform::from_axis_angle({1,0,0}, Pi/2.0f);

     // 2) Build 200 random offsets, now with z ∈ [0, 5]:
    instance_offset.resize(instance_count);
    std::mt19937 eng{ std::random_device{}() };
    std::uniform_real_distribution<float> U01(0.0f, 1.0f);

    float R = 8.0f;    // same disk radius as before
    for (int i = 0; i < instance_count; ++i) {
        float u = U01(eng);
        float r = R * std::sqrt(u);
        float theta = 2.0f * cgp::Pi * U01(eng);
        float x = r * std::cos(theta);
        float y = r * std::sin(theta);

        // New line: pick z uniformly between 0 and 5
        float z = U01(eng) * 5.0f + 1.5f;

        instance_offset[i] = { x, y, z };
    }

    // 3) Upload as divisor=1 attribute at location=6 (no change):
    drawable.initialize_supplementary_data_on_gpu(instance_offset, /*loc=*/6, /*div=*/1);

    // 4) Cache & upload all the same “static” uniforms as before:
    GLuint pid = drawable.shader.id;
    glUseProgram(pid);

    loc_model  = glGetUniformLocation(pid, "model");
    loc_view   = glGetUniformLocation(pid, "view");
    loc_proj   = glGetUniformLocation(pid, "projection");
    loc_light  = glGetUniformLocation(pid, "light");

    loc_particle_color   = glGetUniformLocation(pid, "particle_color");
    loc_alpha            = glGetUniformLocation(pid, "alpha");

    loc_fog_color        = glGetUniformLocation(pid, "fog_color");
    loc_fog_distance_max = glGetUniformLocation(pid, "fog_distance_max");

    loc_fish_texture     = glGetUniformLocation(pid, "fish_texture");

    // Upload once:
    glUniformMatrix4fv(loc_model, 1, GL_FALSE, &drawable.model.matrix()[0][0]);
    glUniform3fv(loc_particle_color, 1, &cgp::vec3{1,1,1}[0]);
    glUniform1f(loc_alpha, 1.0f);
    glUniform3fv(loc_fog_color, 1, &cgp::vec3{0.85f,0.94f,1.0f}[0]);
    glUniform1f(loc_fog_distance_max, 30.0f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, drawable.texture.id);
    glUniform1i(loc_fish_texture, 0);

    glUseProgram(0);
 }

void fish_actor::start_position() {
    drawable.model.rotation = base_rotation;
    drawable.model.translation = {0, 0.5f, 1.0f};

    upload_pose_to_gpu();
}



/**
 * Generate wiggling animation on body, tail, fins and jaw.
 */
void fish_actor::animate(float t) {
	upload_pose_to_gpu();  
    // ─── (2) UPLOAD PER‐FRAME UNIFORMS FOR INSTANCING ────────────────────────
    GLuint pid = drawable.shader.id;
    glUseProgram(pid);

    glUseProgram(0);
}




void fish_actor::draw(environment_structure const& env, cgp::camera_projection_perspective const& camera_projection){
    // 1) Bind camera view/proj/light to the instanced shader uniforms:
    GLuint pid = drawable.shader.id;
    glUseProgram(pid);

    // view & proj from CGP’s environment
    glUniformMatrix4fv(loc_view,  1, GL_FALSE, &env.camera_view[0][0]);
    glUniformMatrix4fv(loc_proj,  1, GL_FALSE, &camera_projection.matrix()[0][0]);
    glUniform3fv(loc_light,       1, &env.light[0]);

    glUseProgram(0);

    // 2) Draw all 200 instances at once
    glDepthMask(GL_FALSE);
    cgp::draw(drawable, env, instance_count);
    glDepthMask(GL_TRUE);
}








