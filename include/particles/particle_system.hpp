// File: particle_system.hpp
#pragma once

#include "cgp/cgp.hpp"
#include "include/environment.hpp"

using cgp::mesh;
using cgp::mesh_drawable;
using cgp::numarray;




struct ParticleParameters{
    int       inst_count = 200000;
    float     speed_in = 1.5f;
    float     spread_in = 4.0f;
    float     fall_in = 40.0f;
    float     swirl_in = 0.4f;
    cgp::vec3 color_in = cgp::vec3(0.8f, 0.9f, 1.0f);
    float     alpha_in = 0.15f;
    cgp::vec3 fog_col_in;
    float     fog_dist_in;
};

// A “GPU‐only” instanced particle cloud (e.g. for water/dust currents).
struct ParticleSystem {
    mesh_drawable sphere; 
    int max_instances = 0; 

    // ──────────────── dynamic parameters ─────────────────
    int instance_count;  ///< how many instances to draw (≤ max_instances)
    float speed;  
    float spread_radius; //  X/Z disk radius
    float fall_depth; // Y‐range before wrapping
    float swirl_strength;  // small swirling in X/Z
    cgp::vec3 color; // base color
    float alpha; // transparency (0…1)
    cgp::vec3 fog_color; // fog mixture
    float fog_distance_max = 30.0f;
    environment_structure environment;
    opengl_shader_structure shader;
    bool world_frame = true; // Decides if the bubbles will be in the world or local frames

    // ──────────────── “static” uniform locations ─────────
    GLint loc_model = -1;
    GLint loc_spread = -1;
    GLint loc_speed = -1;
    GLint loc_fall = -1;
    GLint loc_swirl = -1;
    GLint loc_color = -1;
    GLint loc_alpha = -1;
    GLint loc_fog_color = -1;
    GLint loc_fog_dist = -1;

    // ──────────────── per‐frame uniform locations ─────────
    GLint loc_view = -1;
    GLint loc_proj = -1;
    GLint loc_light = -1;
    GLint loc_time = -1;

    void initialize(environment_structure const& env, std::string const& vert_path,
                    std::string const& frag_path,
                    int                max_insts,
                    cgp::vec3 bubble_center = { 0.2f, 0.4f, 0.5f },
                    float              sphere_radius_in = 0.008f,
                    int                sector_count    = 8,
                    int                stack_count     = 8);


    void set_parameters(ParticleParameters params);


    void upload_static_uniforms();

    void upload_frame_uniforms(cgp::mat4 const& view_matrix, cgp::mat4 const& proj_matrix, cgp::vec3 const& light_pos, float time);

    void draw();

}; 