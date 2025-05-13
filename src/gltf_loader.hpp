#pragma once
// gltf_loader.hpp
// Forward-declaration of the helper that converts a .gltf / .glb into a cgp::mesh
// The implementation lives in gltf_loader.cpp

#include <string>
#include "cgp/cgp.hpp"   // brings in cgp::mesh
#include "tiny_gltf.h"


/**
 * Load the first mesh / first primitive found in a .gltf or .glb file and
 * return it as a cgp::mesh ready to send to initialize_data_on_gpu().
 *
 * @param filename  Absolute or relative path to the file (".gltf" or ".glb")
 * @throw std::runtime_error on I/O or parsing errors.
 */

 struct gltf_geometry_and_texture {
    cgp::mesh          geom;        // positions, normals, uv, connectivity …
    cgp::opengl_texture_image_structure tex;

    cgp::numarray<cgp::uint4> joint_index;   // JOINTS_0
    cgp::numarray<cgp::vec4>  joint_weight;  // WEIGHTS_0

    std::vector<cgp::mat4>    inverse_bind;  // one per joint
    std::vector<int>          joint_node;    // maps joint → node index

    std::vector<int>      joint_parent;   // parent pour chaque joint  (-1 = racine)
    std::vector<cgp::mat4> joint_local;   // matrice locale (bind-pose) de chaque joint
};
gltf_geometry_and_texture mesh_load_file_gltf(const std::string& filename);
