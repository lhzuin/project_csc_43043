// shark_actor.hpp
#pragma once
#include "npc_actor.hpp"
#include "cgp/cgp.hpp"

struct angler_actor final : public npc_actor {
    // ----- internal animation parameters -----
    float tail_frequency = 0.3f;   // Hz – whole-body beat
    float tail_amplitude = 0.25f;  // rad
    float fin_amplitude  = 0.20f;
    float jaw_amplitude  = 0.00040f;
    float lamp_amplitude = 0.015f;
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

