// turtle_actor.hpp
#pragma once
#include "skinned_actor.hpp"
#include "../environment.hpp"
#include "cgp/cgp.hpp"




/// A specialized skinned_actor with autonomous swimming behavior
struct fish_actor final: public skinned_actor {
    // ----- internal animation parameters -----
    float        front_frequency    = 2.0f;      ///< wave freq (Hz)
    float        front_amplitude    = 0.1f;     ///< wave amplitude
    float        rear_amplitude     = 0.1f;     ///< jaw open amplitude
    float        rear_frequency      = 2.0f;     ///< fin beat amplitude
    float aFront;
    float aRear;

    cgp::vec3 base_translation = {0,0,0};

    // ─── Oscillation parameters ─────────────────────────────────────────
    // These control how large/frequent the involuntary drift is in each axis:
    float osc_amp_x       = 0.1f;      // max ±0.03 units along X
    float osc_amp_y       = 0.08f;      // max ±0.02 units along Y
    float osc_freq_x      = 0.31f;       // ~0.7 Hz along X
    float osc_freq_y      = 0.7f;       // ~1.1 Hz along Y
    float osc_phase_x     = 0.0f;       // random phase offset in [0,2π]
    float osc_phase_y     = 0.0f;       // random phase offset in [0,2π]
    
    //rotation_transform base_rotation;
    cgp::rotation_transform base_rotation; // cgp::rotation_transform::from_axis_angle({ 1, 0, 0 }, Pi / 2.0f);
    // --- new for smoothing ---
    float current_yaw = 0.0f;
    float current_pitch = 0.0f;
    float smoothing = 0.15f;  // how fast to catch up: 0 < s <= 1
   
    gltf_geometry_and_texture data;

    // ─── INSTANCING FIELDS ────────────────────────────────────────────────────
    int                  instance_count = 200;          ///< how many copies to draw
    cgp::numarray<cgp::vec3> instance_offset;           ///< per-instance (x,y,z)

    // Cached uniform locations for the instanced shader (only those we need to set once)
    GLint                loc_model  = -1;
    GLint                loc_view   = -1;
    GLint                loc_proj   = -1;
    GLint                loc_light  = -1;
    GLint                loc_particle_color = -1;
    GLint                loc_alpha  = -1;
    GLint                loc_fog_color      = -1;
    GLint                loc_fog_distance_max = -1;
    GLint                loc_fish_texture  = -1;

    void initialize(cgp::opengl_shader_structure const& shader,
                    std::string const& gltf_file,
                    std::string const& texture_file) override;

    /**
     * Initializes position and target for the shark
     */

    void start_position();

    /**
     * Swim movement + directional alignment.
     */
    void move(cgp::vec3 const& direction);

    bool check_for_collision(skinned_actor const& actor);
    void animate(float t) override;

    void draw(environment_structure const& env, cgp::camera_projection_perspective const& camera_projection);
};     

