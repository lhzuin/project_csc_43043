// File: src/particle_system.cpp
#include "particle_system.hpp"
#include <random>

using namespace cgp;

// Set up sphere, shader, instance‐seed VBO, blending, static uniforms
void ParticleSystem::initialize(environment_structure  const& env, std::string const& vert_path,
                                std::string const& frag_path,
                                int                max_insts,
                                cgp::vec3 bubble_center,
                                float              sphere_radius_in,
                                int                sector_count,
                                int                stack_count)
{
    environment = env;
    max_instances = max_insts;

    // Build a small sphere mesh (centered at world origin)
    mesh sphere_mesh = mesh_primitive_sphere(sphere_radius_in, {0,0,0}, sector_count, stack_count);
    sphere.initialize_data_on_gpu(sphere_mesh);
    shader.load(vert_path, frag_path);
    sphere.shader = shader;


    numarray<vec3> instance_seeds(max_instances);
    std::mt19937 eng{std::random_device{}()};
    std::uniform_real_distribution<float> U(0.0f,1.0f);

    for(int i=0; i<max_instances; ++i) {
        float s = U(eng);
        instance_seeds[i] = vec3{s, 0.0f, 0.0f}; 
    }
    sphere.initialize_supplementary_data_on_gpu(instance_seeds, 4, 1);

    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Query all “static” uniform locations and store them
    GLuint pid = sphere.shader.id;
    glUseProgram(pid);

    loc_model     = glGetUniformLocation(pid, "model");
    loc_spread    = glGetUniformLocation(pid, "spread_radius");
    loc_speed     = glGetUniformLocation(pid, "speed");
    loc_fall      = glGetUniformLocation(pid, "fall_depth");
    loc_swirl     = glGetUniformLocation(pid, "swirl_strength");
    loc_color     = glGetUniformLocation(pid, "particle_color");
    loc_alpha     = glGetUniformLocation(pid, "alpha");
    loc_fog_color = glGetUniformLocation(pid, "fog_color");
    loc_fog_dist  = glGetUniformLocation(pid, "fog_distance_max");

    // Per‐frame uniforms:
    loc_view  = glGetUniformLocation(pid, "view");
    loc_proj  = glGetUniformLocation(pid, "projection");
    loc_light = glGetUniformLocation(pid, "light");
    loc_time = glGetUniformLocation(pid, "time");

    // Upload “static” uniforms once with our default parameters:
    cgp::mat4 I = cgp::mat4(1.0f);
    glUniformMatrix4fv(loc_model,  1, GL_FALSE, &I[0][0]);

    // Speed, spread_radius, fall_depth, swirl_strength:
    glUniform1f(loc_spread, spread_radius);
    glUniform1f(loc_speed,  speed);
    glUniform1f(loc_fall,   fall_depth);
    glUniform1f(loc_swirl,  swirl_strength);

    // Color + alpha:
    glUniform3fv(loc_color, 1, &color[0]);
    glUniform1f(loc_alpha,   alpha);

    // Fog:
    glUniform3fv(loc_fog_color, 1, &fog_color[0]);
    glUniform1f(loc_fog_dist,    fog_distance_max);

    glUseProgram(0);

    sphere.model.translation = bubble_center;
}


// Modify any dynamic parameters and re‐upload static uniforms
void ParticleSystem::set_parameters(ParticleParameters params)
{
    instance_count   = cgp::clamp(params.inst_count, 0, max_instances);
    speed            = params.speed_in;
    spread_radius    = params.spread_in;
    fall_depth       = params.fall_in;
    swirl_strength   = params.swirl_in;
    color            = params.color_in;
    alpha            = params.alpha_in;
    fog_color        = params.fog_col_in;
    fog_distance_max = params.fog_dist_in;

    upload_static_uniforms();
}


// Send parameters that only change when set_parameters is called
void ParticleSystem::upload_static_uniforms()
{
    GLuint pid = sphere.shader.id;
    glUseProgram(pid);

    cgp::mat4 I = cgp::mat4(1.0f);
    glUniformMatrix4fv(loc_model, 1, GL_FALSE, &I[0][0]);

    // Re‐send “spread_radius, speed, fall_depth, swirl_strength”
    glUniform1f(loc_spread, spread_radius);
    glUniform1f(loc_speed,  speed);
    glUniform1f(loc_fall,   fall_depth);
    glUniform1f(loc_swirl,  swirl_strength);

    // Re‐send color + alpha
    glUniform3fv(loc_color, 1, &color[0]);
    glUniform1f(loc_alpha,   alpha);

    // Re‐send fog
    glUniform3fv(loc_fog_color, 1, &fog_color[0]);
    glUniform1f(loc_fog_dist,    fog_distance_max);

    glUseProgram(0);
}


// Send per‐frame view/proj/light
void ParticleSystem::upload_frame_uniforms(cgp::mat4 const& view_matrix,
                                           cgp::mat4 const& proj_matrix,
                                           cgp::vec3 const& light_pos, float time)
{
    if(world_frame){
        environment.camera_view = view_matrix;
        environment.light = light_pos;
    }
    
    GLuint pid = sphere.shader.id;
    glUseProgram(pid);

    glUniformMatrix4fv(loc_view, 1, GL_FALSE, &view_matrix[0][0]);
    glUniformMatrix4fv(loc_proj, 1, GL_FALSE, &proj_matrix[0][0]);
    glUniform3fv(loc_light, 1, &light_pos[0]);
    glUniform1f(loc_time,  time);

    glUseProgram(0);
}

// Call CGP’s draw() on the instanced sphere, passing instance_count
void ParticleSystem::draw()
{
    // Disable writing to depth buffer so that transparency blends properly:
    glDepthMask(GL_FALSE);

    // Let CGP draw “instance_count” copies of sphere:
    cgp::draw(sphere, environment, instance_count);

    // Restore depth‐writes for any subsequent opaque draws:
    glDepthMask(GL_TRUE);
}