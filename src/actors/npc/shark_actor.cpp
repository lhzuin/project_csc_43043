#include "include/actors/npc/shark_actor.hpp"
#include "cgp/cgp.hpp"
#include <random>

void shark_actor::initialize(cgp::opengl_shader_structure const& shader,
                std::string const& gltf_file,
                std::string const& texture_file) {
    load_from_gltf(gltf_file, shader);
    drawable.texture.load_and_initialize_texture_2d_on_gpu(
        texture_file, GL_REPEAT, GL_REPEAT);
    groups = {
        {"Tail",   {6,7,8,9,10}},
        {"Body0",  {2}},
        {"Body1",  {3,17,18,19}},
        {"Body2",  {4,15,16}},
        {"Body3",  {5,11,12,13,14}},
        {"FinL",   {20,21,22}},
        {"FinR",   {25,26,27}},
        {"Jaw",    {29,30}}
    };

    // Restart the initial spawn and target distance between a game over session and a new game session. 
    spawn_distance = 20.0f;
    target_dist = 5.0f;
    std::srand(std::time(0));
}


void shark_actor::start_position(skinned_actor const& target_actor) {
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

    cgp::vec3 turtle_pos = target_actor.drawable.model.translation;

    // Set the shark’s origin “above” the turtle by current_spawn_dist
    origin = turtle_pos + cgp::vec3{ rnd_x_shark, current_spawn_dist, rnd_z_shark };

    // Build the swim‐to point (“in front of” turtle + some jitter)
    target = turtle_pos + cgp::vec3{ rnd_x_target, 0.0f, rnd_z_target };
    // Reflect so that the shark actually moves downward toward that point:
    target = 2.0f * target - origin;

    speed = speed_real(engine);

    drawable.model.translation = origin;

    // 9) Decay spawn_distance and target_dist so next call is closer:
    spawn_distance -= (float)(std::rand()) / (float)(std::rand())*spawn_decay_rate;
    if (spawn_distance < min_spawn_distance) {
        spawn_distance = min_spawn_distance;
    }

    target_dist -= (float)(std::rand()) / (float)(std::rand())*target_decay_rate;
    if (target_dist < min_target_dist) {
        target_dist = min_target_dist;
    }
}

void shark_actor::animate(float t) {
    // Body wave
    float w = 2*cgp::Pi*body_frequency;
    std::array<std::string,4> seg = {"Body0","Body1","Body2","Body3"};
    for (int i=0;i<seg.size();++i) {
        float amp = body_amplitude * (amplitude_ratio + (1-amplitude_ratio)*(i/seg.size()));
        float phase = w*t - i*body_lag;
        rotate_group(seg[i], {0,0,1}, amp*std::sin(phase));
    }

    size_t last = seg.size() - 1;           
    // Tail
    float amp_last   = body_amplitude * (amplitude_ratio + (1-amplitude_ratio)*(last/seg.size()));
    float phase_last = w*t - last*body_lag;
    rotate_group("Tail", {0,0,1}, amp_last*std::sin(phase_last));
    // Fins
    float fin = fin_amplitude * std::sin(w*t + cgp::Pi/2);
    rotate_group("FinL",{0,1,0},  fin);
    rotate_group("FinR",{0,1,0}, -fin);
    // Jaw
    float jaw = jaw_amplitude * std::max(0.f, std::sin(w*t));
    rotate_group("Jaw",{1,0,0}, jaw);
    upload_pose_to_gpu();
}

bool shark_actor::check_for_collision(skinned_actor  const& actor){
    // Get shark‐local frame and world‐space centers of each actor’s bounding‐box center:
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
    constexpr float shrinkXY = 0.5f;
    constexpr float shrinkZ  = 0.6f;
    cgp::vec3   E1     = res->half_extents;
    float  radius = std::max(E1.x, E1.y) * shrinkXY; 
    float  halfH  = E1.z * shrinkZ;

    // Box dimensions (other actor) in *shark‐local* axes:
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
void shark_actor::align_to(cgp::vec3 const& dir) {
    cgp::vec3 forward = {0,0,1};
    float cosang = dot(forward, dir);
    if (cgp::abs(cosang + 1.0f) < 1e-3f) {
        drawable.model.rotation = cgp::rotation_transform::from_axis_angle({0,0,1}, cgp::Pi);
    }
    else if (cgp::abs(cosang - 1.0f) < 1e-3f) {
        drawable.model.rotation = cgp::rotation_transform();
    }
    else {
        cgp::vec3 axis = normalize(cross(forward, dir));
        float    angle = acos(cosang);
        drawable.model.rotation = cgp::rotation_transform::from_axis_angle(axis, angle);
    }
}


