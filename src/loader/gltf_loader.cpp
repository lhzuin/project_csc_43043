#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION  
#define STB_IMAGE_WRITE_IMPLEMENTATION 
#include "gltf_loader.hpp"
#include "cgp/cgp.hpp"
using namespace cgp;



/* -------------------------------------------------------------------------- */
/*  TinyGLTF image → OpenGL texture (CGP)                                     */
/* -------------------------------------------------------------------------- */
static void upload_texture_from_gltf(const tinygltf::Image& img,
        cgp::opengl_texture_image_structure& tex,
        GLint wrap = GL_REPEAT)
    {
    using cgp::vec3;
    using cgp::grid_2D;

    const int w = img.width;
    const int h = img.height;

    // Fill a grid_2D<vec3> with RGB values
    grid_2D<vec3> rgb; 
    rgb.resize(w, h);

    const unsigned char* src = img.image.data();
    for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x)
    {
    int i = (y * w + x) * img.component;
    rgb(x, y) = {
    src[i + 0] / 255.0f,
    src[i + 1] / 255.0f,
    src[i + 2] / 255.0f
    };
    }

    // Upload 
    tex.initialize_texture_2d_on_gpu(
    rgb,    
    wrap, wrap,
    /*is_mipmap=*/true);
}

static cgp::mat4 make_mat4(const float* m);

/* -------------------------------------------------------------------------- */
/*  mesh_load_file_gltf - minimal loader that fills a cgp::mesh               */
/* -------------------------------------------------------------------------- */
gltf_geometry_and_texture mesh_load_file_gltf(const std::string& filename)
{
    tinygltf::TinyGLTF loader;
    tinygltf::Model    model;
    std::string        warn, err;

    // Read .glb (binary) or .gltf
    auto load_model_from_file = [&](const std::string& file) -> bool
    {
        auto ext = file.substr(file.find_last_of('.') + 1);
        if (ext == "glb" || ext == "GLB")
            return loader.LoadBinaryFromFile(&model, &err, &warn, file);
        else // treat everything else as .gltf
            return loader.LoadASCIIFromFile (&model, &err, &warn, file);
    };

    bool ok = load_model_from_file(filename);
    if (!ok)
        throw std::runtime_error("TinyGLTF error while loading " + filename +
                                "\nWarn: " + warn + "\nErr : " + err);
    
    // For simplicity, use the *first* mesh and *first* primitive
    const auto& prim  = model.meshes.at(0).primitives.at(0);
    gltf_geometry_and_texture result;

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

    auto copy_uint4 = [&](const std::string& attr,
            cgp::numarray<cgp::uint4>& dst)
    {
        auto it = prim.attributes.find(attr);
        if (it == prim.attributes.end()) return;

        const auto& acc  = model.accessors[it->second];
        const auto& view = model.bufferViews[acc.bufferView];
        const auto& buf  = model.buffers[view.buffer];

        const unsigned char* d = buf.data.data()+view.byteOffset+acc.byteOffset;
        const uint16_t* u16 = reinterpret_cast<const uint16_t*>(d);
        const uint8_t * u8  = reinterpret_cast<const uint8_t *> (d);

        dst.resize(acc.count);

        if(acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE){
        for(size_t i=0;i<acc.count;++i)
        dst[i] = {u8[4*i+0],u8[4*i+1],u8[4*i+2],u8[4*i+3]};
        }else{ /* assume UNSIGNED_SHORT */
        for(size_t i=0;i<acc.count;++i)
        dst[i] = {u16[4*i+0],u16[4*i+1],u16[4*i+2],u16[4*i+3]};
        }
    };

    auto copy_vec4f = [&](const std::string& attr,
                      cgp::numarray<cgp::vec4>& dst)
{
    auto it = prim.attributes.find(attr);
    if(it == prim.attributes.end()) return;

    const auto& acc  = model.accessors[it->second];
    const auto& view = model.bufferViews[acc.bufferView];
    const auto& buf  = model.buffers[view.buffer];
    const uint8_t* src = buf.data.data() + view.byteOffset + acc.byteOffset;

    dst.resize(acc.count);

    if(acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)             // 5126
    {
        const float* f = reinterpret_cast<const float*>(src);
        for(size_t i=0;i<acc.count;++i)
            dst[i] = {f[4*i+0],f[4*i+1],f[4*i+2],f[4*i+3]};
    }
    else if(acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) // 5121
    {
        for(size_t i=0;i<acc.count;++i)
        {
            const uint8_t* u = src + 4*i;
            dst[i] = {u[0]/255.f, u[1]/255.f, u[2]/255.f, u[3]/255.f};
        }
    }
    else /* assume UNSIGNED_SHORT (5123) */                             // 5123
    {
        const uint16_t* u = reinterpret_cast<const uint16_t*>(src);
        for(size_t i=0;i<acc.count;++i)
            dst[i] = {u[4*i]/65535.f, u[4*i+1]/65535.f,
                      u[4*i+2]/65535.f,u[4*i+3]/65535.f};
    }
};

    copy_vec3("POSITION"   , result.geom.position);
    copy_vec3("NORMAL"     , result.geom.normal);
    copy_vec2("TEXCOORD_0" , result.geom.uv);
    copy_uint4("JOINTS_0" , result.joint_index);
    copy_vec4f("WEIGHTS_0", result.joint_weight);

    // indices → connectivity
    if (prim.indices < 0)
        throw std::runtime_error("Primitive has no indices - glTF requires them");

    const auto& accI  = model.accessors [prim.indices];
    const auto& viewI = model.bufferViews[accI.bufferView];
    const auto& bufI  = model.buffers   [viewI.buffer];
    const unsigned char* base = bufI.data.data() + viewI.byteOffset + accI.byteOffset;

    result.geom.connectivity.resize(accI.count / 3);      // triangles

    if (accI.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
    {
        const uint16_t* id = reinterpret_cast<const uint16_t*>(base);
        for (size_t t = 0; t < result.geom.connectivity.size(); ++t)
            result.geom.connectivity[t] = { id[3*t], id[3*t+1], id[3*t+2] };
    }
    else  /* assume uint32 */
    {
        const uint32_t* id = reinterpret_cast<const uint32_t*>(base);
        for (size_t t = 0; t < result.geom.connectivity.size(); ++t)
            result.geom.connectivity[t] = { id[3*t], id[3*t+1], id[3*t+2] };
    }

    // Fall-back if normals weren’t provided
    if (result.geom.normal.size() == 0)
        result.geom.normal_update();

    static std::unordered_map<int,cgp::opengl_texture_image_structure> cache; 

    int texIndex = -1;
    if (prim.material >= 0) {
        const auto& mat = model.materials[prim.material];
        texIndex = mat.pbrMetallicRoughness.baseColorTexture.index;
    }

    if (texIndex >= 0) {
        int imgIndex = model.textures[texIndex].source;
        if (!cache.count(imgIndex))
            upload_texture_from_gltf(model.images[imgIndex], cache[imgIndex]);
        result.tex = cache[imgIndex];  
    }

    if(!model.skins.empty()){
        const tinygltf::Skin& skin = model.skins[0];
    
        result.joint_node = skin.joints;
    
        // Inverse bind matrices
        const auto& accIBM   = model.accessors[skin.inverseBindMatrices];
        const auto& viewIBM  = model.bufferViews[accIBM.bufferView];
        const auto& bufIBM   = model.buffers[viewIBM.buffer];
        const float*  mdata  = reinterpret_cast<const float*>(
                bufIBM.data.data() + viewIBM.byteOffset + accIBM.byteOffset);
    
        size_t njoint = accIBM.count;
        result.inverse_bind.resize(njoint);
        result.inverse_bind.resize(njoint);
        for(size_t j = 0; j < njoint; ++j)
            result.inverse_bind[j] = make_mat4(mdata + 16*j);
    }
    
    result.geom.fill_empty_field();
    if (!model.skins.empty())
    {
        std::cout << "joint  node  name\n";
        for (size_t j = 0; j < result.joint_node.size(); ++j)
            std::cout << j << "     "
                    << result.joint_node[j] << "     "
                    << model.nodes[ result.joint_node[j] ].name << '\n';
}
    return result;
}

/* convert 16 consecutive floats (column-major) to a cgp::mat4 */
static cgp::mat4 make_mat4(const float* m)
{
    cgp::mat4 M;
    for(int col=0; col<4; ++col)
        for(int row=0; row<4; ++row)
            M(row,col) = m[4*col + row];
    return M;
}

