// angler_actor.hpp
#pragma once
#include "npc_actor.hpp"
#include "cgp/cgp.hpp"

struct angler_actor final : public npc_actor {
    float jaw_frequency = 0.8f; 
    float jaw_amplitude  = 0.00040f;

    float spawn_distance = 20.0f;
    float spawn_decay_rate = 0.2f;
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
    void align_to(cgp::vec3 const& dir) override;
};     

