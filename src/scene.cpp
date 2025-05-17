#include "scene.hpp"
#include "animated_texture.hpp"
#include <GLFW/glfw3.h> 

using namespace cgp;

bool equals_exact(cgp::vec3 const& a, cgp::vec3 const& b) {
    return a.x == b.x
        && a.y == b.y
        && a.z == b.z;
}

void deform_terrain(mesh& m)
{
    // Set the terrain to have a gaussian shape
    for (int k = 0; k < m.position.size(); ++k)
    {
        vec3& p = m.position[k];
        float d2 = p.x * p.x + p.y * p.y;
        float z = exp(-d2 / 4) - 1;

        z = z + 0.05f * noise_perlin({ p.x,p.y });

        p = { p.x, p.y, z };
    }

    m.normal_update();
}

// This function is called only once at the beginning of the program
void scene_structure::initialize()
{
    std::cout << "Start function scene_structure::initialize()" << std::endl;

    // Set the behavior of the camera and its initial position
    // ********************************************** //
    camera_control.initialize(inputs, window);
    camera_control.set_rotation_axis_z(); // camera rotates around z-axis
    //   look_at(camera_position, targeted_point, up_direction)


    // Display general information
    display_info();
    // Create the global (x,y,z) frame
    global_frame.initialize_data_on_gpu(mesh_primitive_frame());


    // Create the shapes seen in the 3D scene
    // ********************************************** //

    float L = 5.0f;

    auto const turtle_data = mesh_load_file_gltf(project::path + "assets/sea_turtle/sea_turtle.gltf");
    
    turtle_shader.load(
        project::path + "shaders/turtle/turtle.vert.glsl",
        project::path + "shaders/mesh/custom_mesh.frag.glsl");

    turtle.load_from_gltf(
        project::path + "assets/sea_turtle/sea_turtle.gltf",
        turtle_shader);
    
    turtle.upload_pose_to_gpu();
    turtle.groups["RF"] = { 2,  3,  4,  5 };   // right-front flipper
    turtle.groups["RR"] = { 6,  7,  8,  9 };   // right-rear
    turtle.groups["LF"] = { 10, 11, 12, 13 };   // left-front
    turtle.groups["LR"] = { 14, 15, 16, 17 };   // left-rear

    shark.load_from_gltf(project::path + "assets/shark/scene.gltf", turtle_shader);
    shark.drawable.texture.load_and_initialize_texture_2d_on_gpu(project::path + "assets/shark/textures/SharkBody.png", GL_REPEAT, GL_REPEAT);

    shark.groups["Tail"] = {
            6,   // TailMid
            7, 8,  // TailTop
            9, 10,   // TailBottom_08  + end bones – add more if you like
    };
    shark.groups["Body0"] = { 2 };        // Spine01_02  (près des branchies)
    shark.groups["Body1"] = {
        3, // Spine02_03
        17,18,19 // First dorsal fin

    };
    shark.groups["Body2"] = {
        4, // Spine03_04
        15,16 // Pelvic fin
    };
    shark.groups["Body3"] = {
        5, // Spine04_05 (pédoncule/tail)
        11, 12, // Second dorsal fin
        13, 14, // Bone under dorsal fin
    };
    shark.groups["FinL"] = { 20, 21, 22 };   // Pectoral L (01, 02)
    shark.groups["FinR"] = { 25, 26, 27 };   // Pectoral R

    shark.groups["Jaw"] = { 29, 30 };
    turtle.drawable.model.rotation = rotation_transform::from_axis_angle({ 1, 0, 0 }, Pi / 2.0f);
    vec3 turtle_pos = { 0.2f, 0.4f, 0.5f };
    turtle.drawable.model.translation = turtle_pos;


    shark.drawable.model.rotation = rotation_transform::from_axis_angle({ 1, 0, 0 }, Pi / 2.0f);
    vec3 shark_pos = turtle_pos + vec3{ 0.0f, 20.0f, 0.0f };
    shark.drawable.model.translation = shark_pos;

    vec3 camera_pos = turtle_pos + vec3{ 0.0f, -0.5f, 0.3f };
    vec3 camera_target = turtle_pos + vec3{ 0.0f, 1.0f, 0.2f }; // small tilt down

    camera_control.look_at(
        camera_pos,
        camera_target,
        { 0.0f, 0.0f, 1.0f }   // 'up' is still Z
    );

    environment.caustic_array_tex = create_texture_array_from_sequence(
        project::path + "assets/caustics/02B_Caribbean_Caustics_Deep_FREE_SAMPLE_",
        240,
        4,
        image_format::jpg
    );
}


//------------------------------------------------------------------------------
// Move the turtle and immediately re-anchor the camera
void scene_structure::move_turtle(vec3 const& direction)
{
    // 1) shift the turtle’s position
    turtle.drawable.model.translation += direction;
    
    // 2) re-compute camera to keep the same offset
    vec3 pos = turtle.drawable.model.translation;
    
    ////camera_control.look_at(pos + camera_offset, pos, { 0,0,1 });
    //camera_control.look_at(
    //    turtle.drawable.model.translation + camera_offset,
    //    turtle.drawable.model.translation, { 0,0,1 }
    //);
}

// This function is called each frame to draw the scene
void scene_structure::display_frame()
{
    environment.light = camera_control.camera_model.position();
    timer.update();
    environment.uniform_generic.uniform_float["time"] = timer.t;

    // Turtle flapping
    float t = timer.t;
    float aFront = 0.1f * std::sin(2.0f * t);
    float aRear = 0.1f * std::sin(2.0f * t + cgp::Pi);

    turtle.reset_pose();
    turtle.rotate_group("RF", { 0,0,1 }, aFront);
    turtle.rotate_group("LF", { 0,0,1 }, aFront);
    turtle.rotate_group("RR", { 0,0,1 }, aRear);
    turtle.rotate_group("LR", { 0,0,1 }, aRear);
    turtle.upload_pose_to_gpu();
    draw(turtle.drawable, environment);

    // Shark animation
    std::array<std::string_view, 4> seg = { "Body0","Body1","Body2","Body3" };
    float f = 0.2f, w = 2.0f * cgp::Pi * f, A = 0.16f, lag = 0.40f;
    float jaw_A = 0.15f, fin_A = 0.12f;

    shark.reset_pose();
    for (size_t i = 0; i < seg.size(); ++i) {
        float amp = A * (0.5f + 0.5f * i / seg.size());
        float phase = w * t - i * lag;
        shark.rotate_group(seg[i], { 0,0,1 }, amp * std::sin(phase));
    }
    float amp_last = A * (0.5f + 0.5f * (seg.size() - 1) / seg.size());
    float phase_last = w * t - (seg.size() - 1) * lag;
    shark.rotate_group("Tail", { 0,0,1 }, amp_last * std::sin(phase_last));
    shark.rotate_group("FinL", { 0,1,0 }, fin_A * std::sin(w * t + cgp::Pi / 2));
    shark.rotate_group("FinR", { 0,1,0 }, -fin_A * std::sin(w * t + cgp::Pi / 2));
    shark.rotate_group("Jaw", { 1,0,0 }, jaw_A * std::max(0.f, std::sin(w * t)));
    shark.upload_pose_to_gpu();
    draw(shark.drawable, environment);

    if (gui.display_frame) {
        draw(global_frame, environment);
    }
    if (gui.display_wireframe) {
        draw_wireframe(terrain, environment);
        draw_wireframe(water, environment);
        draw_wireframe(tree, environment);
        draw_wireframe(cube1, environment);
        draw_wireframe(cube2, environment);
        draw_wireframe(shark.drawable, environment);
        draw_wireframe(turtle.drawable, environment);
    }
}

void scene_structure::display_gui()
{
    ImGui::Checkbox("Frame", &gui.display_frame);
    ImGui::Checkbox("Wireframe", &gui.display_wireframe);

    ImGui::Separator();
    ImGui::Text("Move Turtle");

    const float button_step = 0.2f;  // movement per click

    if (ImGui::ArrowButton("##Up", ImGuiDir_Up))
        move_turtle({ 0, +button_step, 0 });
    ImGui::SameLine();
    if (ImGui::ArrowButton("##Left", ImGuiDir_Left))
        move_turtle({ -button_step, 0, 0 });
    ImGui::SameLine();
    if (ImGui::ArrowButton("##Right", ImGuiDir_Right))
        move_turtle({ +button_step, 0, 0 });
    ImGui::SameLine();
    if (ImGui::ArrowButton("##Down", ImGuiDir_Down))
        move_turtle({ 0, -button_step, 0 });
}


void scene_structure::mouse_move_event()
{
    if (!inputs.keyboard.shift)
        camera_control.action_mouse_move(environment.camera_view);
}
void scene_structure::mouse_click_event()
{
    camera_control.action_mouse_click(environment.camera_view);
}
void scene_structure::keyboard_event()
{
    //handle_keyboard_movement();
    camera_control.action_keyboard(environment.camera_view);
}

//------------------------------------------------------------------------------
// Poll the arrow keys each frame and move the turtle
void scene_structure::handle_keyboard_movement()
{
    float speed = 2.0f * inputs.time_interval;

    GLFWwindow* win = window.glfw_window;

    cgp::vec3 delta{ 0, 0, 0 };
    cgp::vec3 origin{ 0, 0, 0 };

    if (glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS) delta += {  0, 0, +1 };
    if (glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS) delta += {  0, 0, -1 };
    if (glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS) delta += { -1, 0, 0 };
    if (glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS) delta += { +1, 0, 0 };

    if (!equals_exact(delta, origin)) {
        delta = normalize(delta) * speed;
        move_turtle(delta);
    }
}

void scene_structure::idle_frame()
{
    handle_keyboard_movement();
    camera_control.idle_frame(environment.camera_view);
}

void scene_structure::display_info()
{
    std::cout << "\nCAMERA CONTROL:\n"
        << "-----------------------------------------------\n"
        << camera_control.doc_usage()
        << "-----------------------------------------------\n\n"
        << "\nSCENE INFO:\n"
        << "-----------------------------------------------\n"
        << "Display here your startup info.\n"
        << "-----------------------------------------------\n\n";
}
