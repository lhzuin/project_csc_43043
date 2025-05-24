// shark_actor.hpp
#pragma once
#include "skinned_actor.hpp"
#include "cgp/cgp.hpp"

/// A specialized skinned_actor with autonomous swimming behavior
struct turtle_actor : public skinned_actor {
    // ----- internal animation parameters -----
    float        front_frequency    = 2.0f;      ///< wave freq (Hz)
    float        front_amplitude    = 0.1f;     ///< wave amplitude
    float        rear_amplitude     = 0.1f;     ///< jaw open amplitude
    float        rear_frequency      = 2.0f;     ///< fin beat amplitude
    float aFront;
    float aRear;

    gltf_geometry_and_texture data;

    /**
     * Convenience: load, setup texture & joint groups all at once.
     */
    void initialize(cgp::opengl_shader_structure const& shader,
                    std::string const& gltf_file,
                    std::string const& texture_file);

    /**
     * Initializes position and target for the shark
     */

    void start_position();

    /**
     * Swim movement + directional alignment.
     */
    void move(cgp::vec3 const& direction);

    /**
     * Generate wiggling animation on body, tail, fins and jaw.
     */
    void animate(float t);

    /**
     * Checks for collision between the shark and another skinned_actor
     */
     bool check_for_collision(skinned_actor actor);
};     

