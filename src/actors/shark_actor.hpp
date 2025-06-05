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
    // 1) How high above the turtle the shark should spawn
    float spawn_distance = 20.0f;

    // 2) How much to reduce spawn_distance each time (per respawn)
    //    You can tune this so it takes a few seconds or waves to get really close.
    float spawn_decay_rate = 2.0f;         // units per respawn call

    // 3) The minimum distance (so the shark does not appear inside the turtle)
    float min_spawn_distance = 5.0f;

    void initialize(cgp::opengl_shader_structure const& shader,
                    std::string const& gltf_file,
                    std::string const& texture_file) override;


    void start_position(skinned_actor const& target_actor) override;

    void start_position(skinned_actor const& target_actor, float elapsed_time);

    bool check_for_collision(skinned_actor const&  actor) override;

    void animate(float t) override;

private:
    /// Align the mesh forward (-Y) to given direction
    void align_to(cgp::vec3 const& dir) override;
};     

