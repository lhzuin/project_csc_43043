#include "skinned_actor.hpp"
#include "cgp/cgp.hpp"


void skinned_actor::rotate_group(std::string_view group_name,
    cgp::vec3 axis, float angle_rad)
{
    auto it = groups.find(std::string(group_name));
    if (it == groups.end()) return;              // unknown group → no-op

    for (int j : it->second)                     // all joints in group
    {
        cgp::mat4 bind = inverse( inverse_bind[j] );  // rest-pose
        cgp::mat4 R = cgp::affine_rt(
                cgp::rotation_transform::
                    from_axis_angle(axis, angle_rad),
                {0,0,0}).matrix();
        uBones[j] = bind * R * inverse_bind[j];
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
    const cgp::opengl_shader_structure& shader,
    int skin_id)
{
    gltf_geometry_and_texture data = mesh_load_file_gltf(file);

    drawable.initialize_data_on_gpu(data.geom, shader, data.tex);
    add_skin_attributes(drawable, data.joint_index, data.joint_weight);

    inverse_bind = std::move(data.inverse_bind);
    joint_node   = std::move(data.joint_node);
    uBones.resize(inverse_bind.size());
    uBones.assign(inverse_bind.size(), cgp::mat4(1.0f));
    //uBones.resize(inverse_bind.size(), cgp::mat4::identity());
}


void skinned_actor::reset_pose()
{
    for (auto& M : uBones)
        M = cgp::mat4(1.0f);   // CGP/GLM identity constructor
}