#pragma once
#include "cgp/cgp.hpp"
#include "gltf_loader.hpp"
#include "gpu_skin_helper.hpp"
#include <unordered_map>
#include <vector>
#include <string_view>

#include <filesystem>
namespace fs = std::filesystem;


/// Generic GPU–skinned model loaded from a glTF file.
/// All concrete “animals” are just specialisations that fill in
/// their own joint groups (flippers, tail, jaw, …).
struct skinned_actor
{
    /*=============== raw skin data straight from glTF ===============*/
    std::vector<cgp::mat4> inverse_bind;   ///< |J| inverse-bind matrices
    std::vector<int>       joint_node;     ///< |J| skin->node map
    std::vector<cgp::mat4> uBones;         ///< |J| final pose (shader)
    std::vector<cgp::mat4> local_pose;   // même taille que joint_local
    std::vector<int>       parent;       // parent par joint
    cgp::hierarchy_mesh_drawable skeleton;           // <-- new
    std::vector<std::string>     joint_name;         // cache “J0”, “J1” …

    cgp::mesh_drawable     drawable;       ///< the mesh we actually draw

    /*=============== high-level helpers =============================*/
    /// a named set of joints, e.g. "Tail", "Mouth", "RF" (right-front fin) …
    using joint_group = std::vector<int>;
    std::unordered_map<std::string, joint_group> groups;

    /// Apply a rotation *about `axis`* to **all** joints in `group_name`.
    void rotate_group(std::string_view group_name,
                      cgp::vec3 axis, float angle_rad);

    void rotate_hierarchical_group(std::string_view name, cgp::vec3 axis, float ang);

    /// Tell OpenGL the current pose (call once per frame *before* draw()).
    void upload_pose_to_gpu() const;
    void reset_pose();    

    /*=============== construction ==================================*/
    /// load everything from disk, send mesh to the GPU, keep skin data
    void load_from_gltf(const std::string& file,
                        const cgp::opengl_shader_structure& shader,
                        int skin_id = 0);
    void load_from_gltf_old(const std::string& file,
                        const cgp::opengl_shader_structure& shader,
                        int skin_id = 0);

    void rotate_joint(int j, cgp::vec3 axis, float ang);
    void update_pose();
    void update_skeleton(const std::vector<cgp::mat4>& global);
};
