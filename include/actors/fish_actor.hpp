// fish_actor.hpp
#pragma once
#include "skinned_actor.hpp"
#include "../environment.hpp"
#include "cgp/cgp.hpp"


struct fish_actor final: public skinned_actor {
    
    cgp::rotation_transform base_rotation;
   
    gltf_geometry_and_texture data;

    // INSTANCING FIELDS
    int                  instance_count = 200; 
    cgp::numarray<cgp::vec3> instance_offset; 

    // Cached uniform locations for the instanced shader
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

    void start_position();
    void animate(float t) override;

    void draw(environment_structure const& env, cgp::camera_projection_perspective const& camera_projection);
};     

