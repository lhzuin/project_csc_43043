// shark_actor.hpp
#pragma once
#include "npc_actor.hpp"
#include "cgp/cgp.hpp"

struct shark_actor final : public npc_actor {
    // ----- internal animation parameters -----
    float        body_frequency    = 0.2f;      ///< wave freq (Hz)
    float        body_amplitude    = 0.16f;     ///< wave amplitude
    float        jaw_amplitude     = 0.15f;     ///< jaw open amplitude
    float        fin_amplitude     = 0.12f;     ///< fin beat amplitude
    float        body_lag          = 0.40f;     ///< phase lag along spine
    float amplitude_ratio = 0.5f;
    // How high above the turtle the shark should spawn
    float spawn_distance = 20.0f;

    //  How much to reduce spawn_distance each time (per respawn)
    float spawn_decay_rate = 0.2f;

    float min_spawn_distance = 12.0f;
    float target_dist = 5.0f;
    float min_target_dist = 2.0f;
    float target_decay_rate = 0.2f;

    void initialize(cgp::opengl_shader_structure const& shader,
                    std::string const& gltf_file,
                    std::string const& texture_file) override;


    void start_position(skinned_actor const& target_actor) override;

    bool check_for_collision(skinned_actor const&  actor) override;

    void animate(float t) override;

private:
    /// Align the mesh forward (-Y) to given direction
    void align_to(cgp::vec3 const& dir) override;
};     

