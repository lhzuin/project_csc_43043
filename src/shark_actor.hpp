// shark_actor.hpp
#pragma once
#include "skinned_actor.hpp"
#include "cgp/cgp.hpp"

/// A specialized skinned_actor with autonomous swimming behavior
struct shark_actor : public skinned_actor {
    float        speed             = 1.0f;      ///< units per second
    cgp::vec3    origin            {0,0,0};     ///< current position
    cgp::vec3    target            {0,0,0};     ///< destination point

    // ----- internal animation parameters -----
    float        body_frequency    = 0.2f;      ///< wave freq (Hz)
    float        body_amplitude    = 0.16f;     ///< wave amplitude
    float        jaw_amplitude     = 0.15f;     ///< jaw open amplitude
    float        fin_amplitude     = 0.12f;     ///< fin beat amplitude
    float        body_lag          = 0.40f;     ///< phase lag along spine
    float amplitude_ratio = 0.5f;


    /**
     * Convenience: load, setup texture & joint groups all at once.
     */
    void initialize(cgp::opengl_shader_structure const& shader,
                    std::string const& gltf_file,
                    std::string const& texture_file);

    /**
     * Initializes position and target for the shark
     */

    void start_position(skinned_actor target_actor);

    /**
     * Swim movement + directional alignment.
     */
    void update_position(float dt);

    /**
     * Generate wiggling animation on body, tail, fins and jaw.
     */
    void animate(float t);

    /**
     * Checks for collision between the shark and another skinned_actor
     */

     bool check_for_collision(skinned_actor actor);

     bool check_for_end_of_life();

private:
    /// Align the mesh forward (-Y) to given direction
    void align_to(cgp::vec3 const& dir);
};     

