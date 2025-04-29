#pragma once
// gltf_loader.hpp
// Forward-declaration of the helper that converts a .gltf / .glb into a cgp::mesh
// The implementation lives in gltf_loader.cpp

#include <string>
#include "cgp/cgp.hpp"   // brings in cgp::mesh

/**
 * Load the first mesh / first primitive found in a .gltf or .glb file and
 * return it as a cgp::mesh ready to send to initialize_data_on_gpu().
 *
 * @param filename  Absolute or relative path to the file (".gltf" or ".glb")
 * @throw std::runtime_error on I/O or parsing errors.
 */
cgp::mesh mesh_load_file_gltf(const std::string& filename);