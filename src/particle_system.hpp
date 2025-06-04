// File: include/particle_system.hpp
#pragma once

#include "cgp/cgp.hpp"
#include "environment.hpp"

using cgp::mesh;
using cgp::mesh_drawable;
using cgp::numarray;




struct ParticleParameters{
    int       inst_count = 200000;
    float     speed_in = 1.5f;
    float     spread_in = 6.0f;
    float     fall_in = 40.0f;
    float     swirl_in = 0.4f;
    cgp::vec3 color_in = cgp::vec3(0.8f, 0.9f, 1.0f);
    float     alpha_in = 0.15f;
    cgp::vec3 fog_col_in;
    float     fog_dist_in;
};

/** 
 * ParticleSystem 
 * ----------------
 * A “GPU‐only” instanced particle cloud (e.g. for water/dust currents).
 *
 * Each instance is a tiny sphere.  All per‐instance motion (falling in –Z,
 * wrapping, random X/Y spread, optional swirl) is computed in the vertex
 * shader.  This class bundles:
 *   • one mesh_drawable (sphere) + its shader
 *   • one large per‐instance “seed” array as a divisor-1 VBO attribute 
 *   • parameters like speed, spread_radius, fall_depth, swirl_strength, instance_count
 *   • uniform-upload logic for both constant and per-frame uniforms
 *
 * Usage sketch:
 *   ParticleSystem ps;
 *   ps.initialize(shader_path_vert, shader_path_frag, max_instances);
 *   ps.set_parameters(speed, spread, depth, swirl, color, alpha, fog_color, fog_dist);
 *   // then each frame:
 *   ps.upload_frame_uniforms(camera_view, camera_proj, light_pos);
 *   ps.draw();
 */
struct ParticleSystem {

    // ──────────────── mesh & shader ─────────────────────
    mesh_drawable          sphere;           ///< one tiny sphere drawn instanced
    int                    max_instances    = 0;    ///< how big the per‐instance seed array is

    // ──────────────── dynamic parameters ─────────────────
    int                    instance_count   = 1000;  ///< how many instances to draw (≤ max_instances)
    float                  speed            = 1.0f;  ///< units/sec downward speed (–Z)
    float                  spread_radius    = 10.0f; ///< X/Y disk radius
    float                  fall_depth       = 20.0f; ///< Z‐range before wrapping
    float                  swirl_strength   = 0.4f;  ///< small swirling in X/Y
    cgp::vec3              color            = {0.8f, 0.9f, 1.0f}; ///< base color
    float                  alpha            = 0.25f; ///< transparency (0…1)
    cgp::vec3              fog_color        = {0.85f, 0.94f, 1.0f}; ///< fog mixture
    float                  fog_distance_max = 30.0f; ///< fade out beyond this
    environment_structure  environment;
    opengl_shader_structure shader;

    // ──────────────── “static” uniform locations ─────────
    // (we store these once so we don’t repeatedly call glGetUniformLocation)
    GLint                  loc_model        = -1;
    GLint                  loc_spread       = -1;
    GLint                  loc_speed        = -1;
    GLint                  loc_fall         = -1;
    GLint                  loc_swirl        = -1;
    GLint                  loc_color        = -1;
    GLint                  loc_alpha        = -1;
    GLint                  loc_fog_color    = -1;
    GLint                  loc_fog_dist     = -1;

    // ──────────────── per‐frame uniform locations ─────────
    GLint                  loc_view         = -1;
    GLint                  loc_proj         = -1;
    GLint                  loc_light        = -1;
    GLint loc_time = -1;

    /**
     * Initialize the ParticleSystem.
     *   • Create one tiny sphere mesh (radius = sphere_radius_in, sector/stacks),
     *   • Upload it to GPU (mesh_drawable.initialize_data_on_gpu),
     *   • Load the vertex+fragment shaders at the given paths,
     *   • Build a per‐instance seed array of size max_instances,
     *   • Call initialize_supplementary_data_on_gpu(...) with that array at location=4, divisor=1,
     *   • Enable blending in OpenGL,
     *   • Query and store all “static” uniform locations,
     *   • Upload the constant uniforms (model=I, spread, speed, fall_depth, swirl, color, alpha, fog).
     *
     * @param vert_path  : full path to particle.vert.glsl
     * @param frag_path  : full path to particle.frag.glsl
     * @param max_insts  : maximum # of instances (allocates that many seeds)
     */
    void initialize(environment_structure const& env, std::string const& vert_path,
                    std::string const& frag_path,
                    int                max_insts,
                    float              sphere_radius_in = 0.008f,
                    int                sector_count    = 8,
                    int                stack_count     = 8);

    /**
     * Change any of the dynamic parameters at runtime.
     * You must call upload_static_uniforms() afterward to re‐send them to GPU.
     */
    void set_parameters(ParticleParameters params);

    /**
     * Re‐upload all “static” uniforms (those that change only when parameters are changed):
     *   spread_radius, speed, fall_depth, swirl_strength, color, alpha, fog_color, fog_distance_max
     */
    void upload_static_uniforms();

    /**
     * Upload the per‐frame uniforms: view, projection, light.
     * Call this once each frame just before calling draw().
     *
     * @param view_matrix  : camera’s 4×4 view
     * @param proj_matrix  : camera’s 4×4 projection
     * @param light_pos    : world‐space light position
     */
    void upload_frame_uniforms(cgp::mat4 const& view_matrix, cgp::mat4 const& proj_matrix, cgp::vec3 const& light_pos, float time);

    /**
     * Draw the instanced particles.  Must call upload_frame_uniforms(...) first.
     */
    void draw();

}; // struct ParticleSystem