#include "skinned_actor.hpp"


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

void skinned_actor::rotate_hierarchical_group(std::string_view name,
    cgp::vec3 axis, float ang)
{
    auto it = groups.find(std::string(name));
    if (it == groups.end()) return;

    for (int j : it->second)
        rotate_joint(j, axis, ang);   // local_pose is modified
}

void skinned_actor::rotate_joint(int j, cgp::vec3 axis, float ang)
{
    /* on modifie la *locale* du joint j -------------- */
    cgp::mat4 R = cgp::affine_rt(
          cgp::rotation_transform::from_axis_angle(axis, ang),
          {0,0,0}).matrix();
    local_pose[j] = local_pose[j] * R;
}

void skinned_actor::update_pose()
{
    size_t J = local_pose.size();
    std::vector<cgp::mat4> global(J);

    for (size_t j = 0; j < J; ++j)
    {
        int p = parent[j];
        global[j] = (p == -1) ? local_pose[j]
                              : global[p] * local_pose[j];

        uBones[j] = global[j] * inverse_bind[j];
    }

    /* ---- feed the matrices to the debug skeleton ------------------ */
    update_skeleton(global);      // <-- new helper

}

void skinned_actor::update_skeleton(
    const std::vector<cgp::mat4>& global)
{
for (size_t j = 0; j < global.size(); ++j)
{
/* 1) put the mat4 into a temporary affine_rt ------------------ */
cgp::affine_rt A;
A.matrix = global[j];          // affine_rt *does* expose .matrix

/* 2) convert affine_rt → affine_rts (adds a scale field) ------ */
cgp::affine_rts T(A);          // calls the proper constructor

/* 3) store the result in the hierarchy node ------------------- */
skeleton[ joint_name[j] ].drawable.hierarchy_transform_model = T;
}

skeleton.update_local_to_global_coordinates();
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



/* skinned_actor.cpp ---------------------------------------------------- */
void skinned_actor::load_from_gltf(const std::string& file,
    const cgp::opengl_shader_structure& shader,
    int /*skin_id*/)
{
    gltf_geometry_and_texture data = mesh_load_file_gltf(file);

    /* ---------- mesh + skin (exactly what you already had) ------------- */
    drawable.initialize_data_on_gpu(data.geom, shader, data.tex);
    add_skin_attributes(drawable, data.joint_index, data.joint_weight);

    inverse_bind = std::move(data.inverse_bind);
    parent       = std::move(data.joint_parent);

    const size_t J = inverse_bind.size();

    /* ---------- bind-pose in local space ------------------------------- */
    std::vector<cgp::mat4> global_bind(J);
    for (size_t j = 0; j < J; ++j)
        global_bind[j] = inverse(inverse_bind[j]);

    local_pose.resize(J);
    for (size_t j = 0; j < J; ++j) {
        int p = parent[j];
        local_pose[j] = (p == -1) ?
        global_bind[j] :
        inverse(global_bind[p]) * global_bind[j];
    }

    /* ---------- identity pose on the GPU ------------------------------ */
    uBones.assign(J, cgp::mat4(1.0f));

    /* ====================================================================
    * =  HERE COMES THE NEW PART: build a CGP hierarchy for the bones   =
    * ================================================================= */
    joint_name.resize(J);
    skeleton.elements.clear();
    skeleton.name_map.clear();

    /* tiny sphere we will reuse for every joint ------------------------ */
    cgp::mesh_drawable tiny;
    tiny.initialize_data_on_gpu(cgp::mesh_primitive_sphere(0.02f));
    tiny.material.color = {0.9f,0.7f,0.2f};

    for (size_t j = 0; j < J; ++j)
    {
    joint_name[j] = "J" + std::to_string(j);
    std::string parent_name =
    (parent[j] == -1) ? "global_frame"
    : joint_name[parent[j]];

    /* local TRS extracted from bind-pose ------------------------- */
    cgp::affine_rts trs;
    trs.matrix = local_pose[j];     // affine_rts has a public ‘matrix’

    skeleton.add( tiny,                  // the little sphere
    joint_name[j],         // its name
    parent_name,           // parent in the hierarchy
    trs );                 // local transform
    }
}

void skinned_actor::load_from_gltf_old(const std::string& file,
    const cgp::opengl_shader_structure& shader,
    int skin_id)
{
    gltf_geometry_and_texture data = mesh_load_file_gltf(file);

    drawable.initialize_data_on_gpu(data.geom, shader, data.tex);
    add_skin_attributes(drawable, data.joint_index, data.joint_weight);

    inverse_bind = std::move(data.inverse_bind);
    joint_node   = std::move(data.joint_node);
    //uBones.resize(inverse_bind.size());
    //uBones.assign(inverse_bind.size(), cgp::mat4(1.0f));
    local_pose  = std::move(data.joint_local);   // bind-pose
    parent      = std::move(data.joint_parent);
    uBones.resize(local_pose.size());            // sera rempli chaque frame
    uBones.assign(local_pose.size(), cgp::mat4(1.0f));
    reset_pose();                                // copie bind-pose dans uBones
    //uBones.resize(inverse_bind.size(), cgp::mat4::identity());
}


void skinned_actor::reset_pose()
{
    for (auto& M : uBones)
        M = cgp::mat4(1.0f);   // CGP/GLM identity constructor
}