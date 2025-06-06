#pragma once
#include "cgp/cgp.hpp"
#include "../loader/gltf_loader.hpp"
#include "../loader/gpu_skin_helper.hpp"
#include <unordered_map>
#include <vector>
#include <string_view>

#include <filesystem>



struct ActorResources {
    // Raw skin data straight from glTF 
    std::vector<cgp::mat4> inverse_bind;
    std::vector<int> joint_node;
    

    cgp::mesh_drawable prototype;
    cgp::numarray<cgp::uint4>        joint_index;
    cgp::numarray<cgp::vec4>         joint_weight;
    
    // Collision mechanism
    cgp::mesh geometry;
    float  radius;
    cgp::vec3   half_extents; 
    cgp::vec3   center_offset;
    void compute_radius(); 
    void compute_bounding_box();
};

/// Generic GPU–skinned model loaded from a glTF file.
struct skinned_actor
{
    virtual ~skinned_actor() = default;
    std::vector<cgp::mat4> uBones;
    std::shared_ptr<ActorResources> res; // shared data
    cgp::mesh_drawable     drawable;

    using joint_group = std::vector<int>;
    std::unordered_map<std::string, joint_group> groups;

    /// Apply a rotation *about `axis`* to **all** joints in `group_name`.
    void rotate_group(std::string_view group_name,
                      cgp::vec3 axis, float angle_rad);

    /// Tell OpenGL the current pose
    void upload_pose_to_gpu() const;
    
    void reset_pose();    

    /// load everything from disk, send mesh to the GPU, keep skin data
    void load_from_gltf(const std::string& file,
                        const cgp::opengl_shader_structure& shader);

    virtual void initialize(cgp::opengl_shader_structure const& shader,
                    std::string const& gltf_file,
                    std::string const& texture_file) = 0;

    virtual void animate(float t) = 0;
};
