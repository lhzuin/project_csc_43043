#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION          // let TinyGLTF decode PNG/JPG
#define STB_IMAGE_WRITE_IMPLEMENTATION     // not strictly required for loading
#include "tiny_gltf.h"

#include "cgp/cgp.hpp"
using namespace cgp;

/* -------------------------------------------------------------------------- */
/*  mesh_load_file_gltf – minimal loader that fills a cgp::mesh               */
/* -------------------------------------------------------------------------- */
mesh mesh_load_file_gltf(const std::string& filename)
{
    tinygltf::TinyGLTF loader;
    tinygltf::Model    model;
    std::string        warn, err;

    /* -------- read .glb (binary) or .gltf (text + externals) -------------- */
    bool ok = loader.LoadBinaryFromFile(&model, &err, &warn, filename);
    if (!ok)
        throw std::runtime_error("TinyGLTF error while loading " + filename +
                                 "\nWarn: " + warn + "\nErr : " + err);

    /* ---- for simplicity, use the *first* mesh and *first* primitive ------ */
    const auto& prim  = model.meshes.at(0).primitives.at(0);
    mesh out;

    /* -------------------------------------------------------------------------- */
    /*  Helpers that copy attribute data into a numarray<vec3> / numarray<vec2>   */
    /* -------------------------------------------------------------------------- */
    auto copy_vec3 = [&](const std::string& attr, cgp::numarray<cgp::vec3>& dst)
    {
        auto it = prim.attributes.find(attr);
        if (it == prim.attributes.end()) return;

        const auto& acc  = model.accessors[it->second];
        const auto& view = model.bufferViews[acc.bufferView];
        const auto& buf  = model.buffers[view.buffer];

        const float* f = reinterpret_cast<const float*>(
            buf.data.data() + view.byteOffset + acc.byteOffset);

        dst.resize(acc.count);
        for (size_t i = 0; i < acc.count; ++i)
            dst[i] = { f[3*i], f[3*i+1], f[3*i+2] };
    };

    auto copy_vec2 = [&](const std::string& attr, cgp::numarray<cgp::vec2>& dst)
    {
        auto it = prim.attributes.find(attr);
        if (it == prim.attributes.end()) return;

        const auto& acc  = model.accessors[it->second];
        const auto& view = model.bufferViews[acc.bufferView];
        const auto& buf  = model.buffers[view.buffer];

        const float* f = reinterpret_cast<const float*>(
            buf.data.data() + view.byteOffset + acc.byteOffset);

        dst.resize(acc.count);
        for (size_t i = 0; i < acc.count; ++i)
            dst[i] = { f[2*i], f[2*i+1] };
    };

    /* -------------------------------------------------------------------------- */
    /*  Use them for each attribute                                               */
    /* -------------------------------------------------------------------------- */
    copy_vec3("POSITION"   , out.position);
    copy_vec3("NORMAL"     , out.normal);
    copy_vec2("TEXCOORD_0" , out.uv);

    /* ---------------------- indices → connectivity ------------------------ */
    if (prim.indices < 0)
        throw std::runtime_error("Primitive has no indices – glTF requires them");

    const auto& accI  = model.accessors [prim.indices];
    const auto& viewI = model.bufferViews[accI.bufferView];
    const auto& bufI  = model.buffers   [viewI.buffer];
    const unsigned char* base = bufI.data.data() + viewI.byteOffset + accI.byteOffset;

    out.connectivity.resize(accI.count / 3);      // triangles

    if (accI.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
    {
        const uint16_t* id = reinterpret_cast<const uint16_t*>(base);
        for (size_t t = 0; t < out.connectivity.size(); ++t)
            out.connectivity[t] = { id[3*t], id[3*t+1], id[3*t+2] };
    }
    else  /* assume uint32 */
    {
        const uint32_t* id = reinterpret_cast<const uint32_t*>(base);
        for (size_t t = 0; t < out.connectivity.size(); ++t)
            out.connectivity[t] = { id[3*t], id[3*t+1], id[3*t+2] };
    }

    /* fall-back if normals weren’t provided */
    if (out.normal.size() == 0)
        out.normal_update();
    out.fill_empty_field();
    return out;
}