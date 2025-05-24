#include "skinned_actor.hpp"
#include "cgp/cgp.hpp"

// Static cache
static std::unordered_map<std::string,
    std::shared_ptr<ActorResources>> resource_cache;

//-----------------------------------------------------------------------------
// ActorResources
//-----------------------------------------------------------------------------
void ActorResources::compute_radius() {
    cgp::vec3 pmin, pmax;
    geometry.get_bounding_box_position(pmin, pmax);
    cgp::vec3 d = pmax - pmin;
    radius = 0.4f * std::max({d.x,d.y,d.z});
}


void ActorResources::compute_bounding_box()
{
    cgp::vec3 pmin, pmax;
    geometry.get_bounding_box_position(pmin, pmax);
    half_extents   = (pmax - pmin) * 0.5f;
    center_offset  = (pmax + pmin) * 0.5f;
    std::cout << half_extents;
    radius         = std::max({half_extents.x, half_extents.y, half_extents.z});
}


//-----------------------------------------------------------------------------
// skinned_actor
//-----------------------------------------------------------------------------
void skinned_actor::rotate_group(std::string_view group_name,
    cgp::vec3 axis, float angle_rad)
{
    auto it = groups.find(std::string(group_name));
    if (it == groups.end()) return;              // unknown group → no-op
    for (int j : it->second)                     // all joints in group
    {
        cgp::mat4 bind = inverse( res->inverse_bind[j] );  // rest-pose
        cgp::mat4 R = cgp::affine_rt(
                cgp::rotation_transform::
                    from_axis_angle(axis, angle_rad),
                {0,0,0}).matrix();
        uBones[j] = bind * R * res->inverse_bind[j];
    }
}


void skinned_actor::upload_pose_to_gpu() const
{
    glUseProgram(drawable.shader.id);
    for (size_t j = 0; j < uBones.size(); ++j) {
        std::string name = "uBones[" + std::to_string(j) + "]";
        GLint loc = glGetUniformLocation(drawable.shader.id, name.c_str());
        if (loc >= 0)
            glUniformMatrix4fv(loc, 1, GL_FALSE, &uBones[j](0,0));
    }
    glUseProgram(0);
}


void skinned_actor::load_from_gltf(const std::string& file,
    const cgp::opengl_shader_structure& shader)
{
   // Lookup or fill the cache
    auto it = resource_cache.find(file);
    if(it == resource_cache.end()) {
        // load glTF once
        gltf_geometry_and_texture data = mesh_load_file_gltf(file);

        auto R = std::make_shared<ActorResources>();
        R->geometry      = data.geom;  
        R->inverse_bind  = std::move(data.inverse_bind);
        R->joint_node    = std::move(data.joint_node);
        R->joint_index   = data.joint_index;
        R->joint_weight  = data.joint_weight;

        // Build prototype drawable
        R->prototype.initialize_data_on_gpu(
          data.geom, shader, data.tex);
        add_skin_attributes(
          R->prototype,
          R->joint_index, R->joint_weight);

        R->compute_radius();
        R->compute_bounding_box();

        resource_cache[file] = R;
        res = R;
    }
    else {
        res = it->second;
    }

    // Actor‐local initialization
    uBones.resize(res->inverse_bind.size());
    reset_pose();

    // copy the shared VAO/VBO into our own drawable
    drawable = res->prototype;  // shallow copy of handles is fine
}


void skinned_actor::reset_pose()
{
    for (auto& M : uBones)
        M = cgp::mat4(1.0f);   // CGP/GLM identity constructor
}