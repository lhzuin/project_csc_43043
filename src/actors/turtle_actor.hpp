// shark_actor.hpp
#pragma once
#include "skinned_actor.hpp"
#include "cgp/cgp.hpp"

/// A specialized skinned_actor with autonomous swimming behavior
struct turtle_actor final: public skinned_actor {
    // ----- internal animation parameters -----
    float        front_frequency    = 2.0f;      ///< wave freq (Hz)
    float        front_amplitude    = 0.1f;     ///< wave amplitude
    float        rear_amplitude     = 0.1f;     ///< jaw open amplitude
    float        rear_frequency      = 2.0f;     ///< fin beat amplitude
    float aFront;
    float aRear;

    gltf_geometry_and_texture data;

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
};     

