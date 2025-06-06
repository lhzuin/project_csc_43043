#include "angler_actor.hpp"
#include "cgp/cgp.hpp"
#include <random>

void angler_actor::initialize(cgp::opengl_shader_structure const& shader,
                std::string const& gltf_file,
                std::string const& texture_file) {
    load_from_gltf(gltf_file, shader);
    drawable.texture.load_and_initialize_texture_2d_on_gpu(
        texture_file, GL_REPEAT, GL_REPEAT);
    // define joint groups
    groups = {
        {"Tail0",  { 8 }},        
        {"Tail1",  { 9 }},   
        {"Tail2",  {10 }},  
        {"FinL",   {11,12}}, 
        {"FinR",   {13,14}},
        {"Jaw",    {2,5,7}},
        {"Lamp",   { 4 }}
    };

    float s = 1.0f / 40.0f;
    drawable.model.scaling = s;
    base_rotation = cgp::rotation_transform::from_axis_angle({0,0,1},  cgp::Pi);

    target_dist = 4.0f;
    std::srand(std::time(0));
}
auto phase = [](float w, float t, float lag){ return w*t - lag; };
void angler_actor::start_position(skinned_actor const& target_actor) {
    drawable.model.rotation = base_rotation;

    // Random engines & distributions
    static std::mt19937 engine{ std::random_device{}() };
    std::uniform_real_distribution<float> dist_xz(-5.0f, 5.0f);
    std::uniform_real_distribution<float> dist_target(-target_dist, target_dist);
    std::uniform_real_distribution<float> speed_real(2.0f, 8.0f);

    // Compute how far above the turtle we spawn this time:
    float current_spawn_dist = std::max(min_spawn_distance, spawn_distance);

    // Random X/Z jitter for origin & target‐bias
    float rnd_x_shark = dist_xz(engine);
    float rnd_z_shark = dist_xz(engine);
    float rnd_x_target = dist_target(engine);
    float rnd_z_target = dist_target(engine);

    // Turtle’s current position in world space:
    cgp::vec3 turtle_pos = target_actor.drawable.model.translation;

    // Set the shark’s origin “above” the turtle by current_spawn_dist
    origin = turtle_pos + cgp::vec3{ rnd_x_shark, current_spawn_dist, rnd_z_shark };

    // Build the swim‐to point (“in front of” turtle + some jitter)
    target = turtle_pos + cgp::vec3{ rnd_x_target, 0.0f, rnd_z_target };
    // Then reflect so that the shark actually moves downward toward that point:
    target = 2.0f * target - origin;
    speed = speed_real(engine);
    drawable.model.translation = origin;
    spawn_distance -= (float)(std::rand()) / (float)(std::rand())*spawn_decay_rate;
    if (spawn_distance < min_spawn_distance) {
        spawn_distance = min_spawn_distance;
    }


    target_dist -= (float)(std::rand()) / (float)(std::rand())*target_decay_rate;
    if (target_dist < min_target_dist) {
        target_dist = min_target_dist;
    }
}

void angler_actor::animate(float t)
{
    const float w = 2.0f * cgp::Pi * jaw_frequency;

    float jaw = jaw_amplitude * cgp::clamp( std::sin(w*t), 0.f, 1.f ); 
    rotate_group("Jaw", {0,1,0}, jaw);

    upload_pose_to_gpu(); 
}

bool angler_actor::check_for_collision(skinned_actor  const& actor){
    // Get angler‐local frame and world‐space centers of each actor’s bounding‐box center:
    cgp::mat4 M1     = drawable.model.matrix();
    cgp::vec3 C1     = (M1 * cgp::vec4(res->center_offset, 1)).xyz();
    cgp::mat4 M2     = actor.drawable.model.matrix();
    cgp::vec3 C2     = (M2 * cgp::vec4(actor.res->center_offset, 1)).xyz();

    // World‐space delta
    cgp::vec3 d_world = C2 - C1;

    // Transform delta into shark’s local rotated+scaled space:
    cgp::mat3 RS    = cgp::mat3(M1); 
    cgp::mat3 invRS = cgp::inverse(RS);
    cgp::vec3 d_loc = invRS * d_world;

    // Cylinder dimensions (shark) in its local space:
    constexpr float shrinkXY = 1.0f;
    constexpr float shrinkZ  = 0.6f;
    cgp::vec3   E1     = res->half_extents;
    float  radius = std::max(E1.x, E1.y) * shrinkXY;
    float  halfH  = E1.z * shrinkZ;

    cgp::vec3   E2     = actor.res->half_extents;         

    // Horizontal (X–Y) distance from cylinder axis to box:
    float dx = std::max(std::abs(d_loc.x) - E2.x, 0.0f);
    float dy = std::max(std::abs(d_loc.y) - E2.y, 0.0f);
    bool  overlapXY = (dx*dx + dy*dy) <= (radius*radius);

    // Vertical overlap (Z‐axis):
    bool  overlapZ  = std::abs(d_loc.z) <= (halfH + E2.z);

    return overlapXY && overlapZ;
}


/// Align the mesh forward (-Y) to given direction
void angler_actor::align_to(cgp::vec3 const& dir) {
    cgp::vec3 forward = {0,0,1};
    float cosang = dot(forward, dir);
    if (cgp::abs(cosang + 1.0f) < 1e-3f) {
        drawable.model.rotation = base_rotation*cgp::rotation_transform::from_axis_angle({0,0,1}, cgp::Pi);
    }
    else if (cgp::abs(cosang - 1.0f) < 1e-3f) {
        drawable.model.rotation = base_rotation*cgp::rotation_transform();
    }
    else {
        cgp::vec3 axis = normalize(cross(forward, dir));
        float    angle = acos(cosang);
        drawable.model.rotation = base_rotation*cgp::rotation_transform::from_axis_angle(axis, angle);
    }
}


