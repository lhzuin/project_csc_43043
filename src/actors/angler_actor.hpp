// shark_actor.hpp
#pragma once
#include "npc_actor.hpp"
#include "cgp/cgp.hpp"

struct angler_actor final : public npc_actor {
    // ----- internal animation parameters -----
    float jaw_frequency = 0.8f;   // Hz – whole-body beat
    //float tail_amplitude = 0.25f;  // rad
    //float fin_amplitude  = 0.20f;
    float jaw_amplitude  = 0.00040f;
    //float lamp_amplitude = 0.015f;


    // 1) How high above the turtle the shark should spawn
    float spawn_distance = 20.0f;

    // 2) How much to reduce spawn_distance each time (per respawn)
    //    You can tune this so it takes a few seconds or waves to get really close.
    float spawn_decay_rate = 0.2f;         // units per respawn call

    // 3) The minimum distance (so the shark does not appear inside the turtle)
    float min_spawn_distance = 12.0f;

    float target_dist = 4.0f;
    float min_target_dist = 1.0f;
    float target_decay_rate = 0.2f;

    cgp::rotation_transform base_rotation;

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

