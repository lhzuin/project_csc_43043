// nemo_actor.hpp
#pragma once
#include "skinned_actor.hpp"
#include "turtle_actor.hpp"
#include "cgp/cgp.hpp"



struct nemo_actor final: public skinned_actor {
    cgp::rotation_transform base_rotation;

   
    gltf_geometry_and_texture data;

    void initialize(cgp::opengl_shader_structure const& shader,
                    std::string const& gltf_file,
                    std::string const& texture_file) override;

    void start_position();
    void animate(float t) override;
    void follow(const turtle_actor& turtle,
            cgp::vec3 local_offset  = {0,0.15f,0.0f},   // 30 cm above the shell
            cgp::rotation_transform extra_yaw_pitch = cgp::rotation_transform());
};     

