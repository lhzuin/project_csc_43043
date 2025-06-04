#include "scene.hpp"
#include "loader/animated_texture.hpp"
#include "actors/shark_actor.hpp"

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

void scene_structure::loop_initialize()
{



    // Create the shapes seen in the 3D scene
    // ********************************************** //

    
    turtle.start_position();

    nemo.start_position();
    
    fish.start_position();
    // ───────────────────────────────────────────────────────────────────
    // Compute initial camera position based on gui.first_player_view
    vec3 base = turtle.base_translation;
    vec3 offset;
    if (gui.first_player_view) {
        // First‐person offset:
        offset = vec3{ 0.0f, -0.5f, 0.3f };
    }
    else {
        // Third‐person offset:
        offset = vec3{ 0.0f, -1.8f, 1.0f };
    }
    vec3 camera_pos = base + offset;
    vec3 camera_target = base + vec3{ 0.0f, 1.0f, 0.2f };

    camera_control.look_at(
        camera_pos,
        camera_target,
        { 0.0f, 0.0f, 1.0f }   // 'up' is still Z
    );
    // ───────────────────────────────────────────────────────────────────
    // Reset the “difficulty timer” so shark‐speed starts fresh
    gameplay_time = 0.0f;
    spawn_shark();

    
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

    
    actor_shader.load(
        project::path + "shaders/actor/actor.vert.glsl",
        project::path + "shaders/mesh/custom_mesh.frag.glsl");

    turtle.initialize(actor_shader,
            project::path + "assets/sea_turtle/sea_turtle.gltf",
            project::path + "assets/sea_turtle/textures/Tortue_PBRMaterial_baseColor.png");

    

    nemo.initialize(actor_shader, project::path + "assets/nemo_finding_nemo/scene.gltf", project::path + "assets/nemo_finding_nemo/textures/nemo_diff_png_baseColor.png");
    
    fish_instanced_shader.load(
        project::path + "shaders/actor/instanced_fish.vert.glsl",
        project::path + "shaders/mesh/custom_mesh.frag.glsl"  // reuse your existing fragment
    );

    fish.initialize(fish_instanced_shader,
                    project::path + "assets/blue_powder_tang/scene.gltf",
                    project::path + "assets/blue_powder_tang/textures/Material.002_baseColor.png");

    // ───────────────────────────────────────────────────────────────────
    environment.caustic_array_tex = create_texture_array_from_sequence(
        project::path + "assets/caustics/02B_Caribbean_Caustics_Deep_FREE_SAMPLE_",
        240,
        4,
        image_format::jpg
    );

    const std::string vert_path = project::path + "shaders/particle/particle.vert.glsl";
    const std::string frag_path = project::path + "shaders/particle/particle.frag.glsl";
    int max_particles = 2000000;

    loop_initialize();

    particle_system.initialize(environment, vert_path, frag_path, max_particles);

    // Optionally, immediately set your desired parameters
    particle_parameters.fog_col_in = cgp::vec3(environment.background_color);
    particle_parameters.fog_dist_in = environment.fog_d_max;

    particle_system.set_parameters(particle_parameters);

    
}


void scene_structure::spawn_shark()
{
    if (sharks.size() == 0){
        shark_actor s;
        s.initialize(actor_shader,
            project::path + "assets/shark/scene.gltf",
            project::path + "assets/shark/textures/SharkBody.png");
        sharks.push_back(std::move(s));
    }
    sharks[0].start_position(turtle);

    // Immediately ramp that new shark’s speed up by (speed_increase_rate * gameplay_time)
    // so that it’s already “as fast as” any shark would be at this point
    sharks[0].speed += speed_increase_rate * gameplay_time;
}

//------------------------------------------------------------------------------
// Move the turtle and immediately re-anchor the camera

void scene_structure::display_frame()
{
    // 1) Set the light position to follow the camera
    environment.light = camera_control.camera_model.position();

    // 2) If the game has NOT started yet, show the full‐screen "Welcome" menu
    if (!game_started) {

        // ─────────────────────────────────────────────────────────────────
        // 2a) Force a full‐screen, borderless ImGui window:
        // ─────────────────────────────────────────────────────────────────
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2((float)window.width, (float)window.height));

        ImGuiWindowFlags menu_flags = 
            ImGuiWindowFlags_NoTitleBar
          | ImGuiWindowFlags_NoResize
          | ImGuiWindowFlags_NoMove
          | ImGuiWindowFlags_NoScrollbar
          | ImGuiWindowFlags_NoSavedSettings
          | ImGuiWindowFlags_NoFocusOnAppearing;

        ImGui::Begin("StartMenu", nullptr, menu_flags);

        // ─────────────────────────────────────────────────────────────────
        // 2b) Get window dimensions, measure text size, and center both:
        // ─────────────────────────────────────────────────────────────────
        float window_w = (float)window.width;
        float window_h = (float)window.height;

        // The welcome text and button label:
        std::string menu_text    = "Welcome to Turtle Ride!";
        std::string button_label = "Play";

        // Measure the width/height of the text in pixels
        ImVec2 text_size = ImGui::CalcTextSize(menu_text.c_str());

        // Place the text at ~45% of window height, horizontally centered
        float text_x = (window_w - text_size.x) * 0.5f;
        float text_y = window_h * 0.45f;

        ImGui::SetCursorPosX(text_x);
        ImGui::SetCursorPosY(text_y);
        ImGui::Text("%s", menu_text.c_str());

        // ─────────────────────────────────────────────────────────────────
        // 2c) Place a button just below the text, centered
        // ─────────────────────────────────────────────────────────────────
        ImVec2 button_size = ImVec2(160.0f, 60.0f);
        float   button_x    = (window_w - button_size.x) * 0.5f;
        float   button_y    = text_y + text_size.y + 40.0f;

        ImGui::SetCursorPosX(button_x);
        ImGui::SetCursorPosY(button_y);
        if (ImGui::Button(button_label.c_str(), button_size)) {
            // When “Play” is clicked:
            game_started = true;

            // Reset game_over in case it was true
            game_over = false;

            // Re‐initialize anything needed for a new playthrough:
            // Here, we’ll just call initialize() to reset turtle, sharks, timer, etc.
            // If you only want to reset positions & scores (and not re‐upload GPU data),
            // move that logic into a separate helper, e.g. reset_for_new_playthrough().
            initialize();

            // Ensure the timer does not jump. Immediately update it so dt calculations
            // start fresh from zero:
            timer.update();
        }

        ImGui::End();

        // ─────────────────────────────────────────────────────────────────
        // Because we’re still on the menu, we skip all gameplay updates/draws.
        // ─────────────────────────────────────────────────────────────────
        return;
    }


    // 3) If the game HAS started but not yet over ⇒ normal gameplay:
    if (game_started && !game_over) {

        // Advance your internal clock & animate uniforms
        float t_prev = timer.t;
        timer.update();
        float dt = timer.t - t_prev;
        environment.uniform_generic.uniform_float["time"] = timer.t;


        // Accumulate total gameplay time
        gameplay_time += dt;

        /* ======== TURTLE AND NEMO ======== */
        turtle.animate(timer.t);

        nemo.follow(turtle);
        nemo.animate(timer.t);

        draw(turtle.drawable, environment);
        draw(nemo.drawable, environment);

        /* ======== FISHES ======== */
        fish.animate(timer.t);
        fish.draw(environment, camera_projection);
        //draw(fish.drawable, environment);

        /* ======== SHARK ======== */
        shark_actor& sh = sharks[0];
        // Increase shark.speed by a bit each second
        sh.speed += speed_increase_rate * dt;
        sh.update_position(dt);
        sh.animate(timer.t);
        draw(sh.drawable, environment);

        // Collision check: if not eaten, allow respawn; otherwise set game_over = true
        if (!sh.check_for_collision(turtle)) {
            if (sh.check_for_end_of_life()) {
                spawn_shark();
            }
        }
        else {
            // Turtle was caught ⇒ switch to game‐over state
            game_over = true;
        }

        // Handle turtle movement from keyboard arrows
        handle_keyboard_movement();
        // ───────────────────────────────────────────────────────────────
        // Re‐anchor / update the camera **every frame** based on current turtle position
        vec3 base = turtle.drawable.model.translation;
        vec3 offset;
        if (gui.first_player_view) {
            // First‐player offset:
            offset = vec3{ 0.0f, -0.5f, 0.3f };
        }
        else {
            // Third‐person offset:
            offset = vec3{ 0.0f, -1.8f, 1.0f };
        }
        vec3 camera_pos = base + offset;
        vec3 camera_target = base + vec3{ 0.0f, 1.0f, 0.2f };

        camera_control.look_at(
            camera_pos,
            camera_target,
            { 0.0f, 0.0f, 1.0f }
        );
        // ─────────────────────────────────────────────────────────────────────────────
        //  WATER‐PARTICLES: upload per-frame uniforms (view, proj, light), then draw:
        // ─────────────────────────────────────────────────────────────────────────────
        {
            // view/proj from CGP
            cgp::mat4 V = environment.camera_view;
            cgp::mat4 P = camera_projection.matrix();
            cgp::vec3 L = environment.light;

            particle_system.upload_frame_uniforms(V, P, L, timer.t);
            particle_system.draw();
        }
    }
    // 4) If the game HAS started and is OVER ⇒ show “Game Over” screen
    else if (game_started && game_over) {
        // We still draw the turtle (so the user sees it in its final position)
        draw(turtle.drawable, environment);
        // Full‐screen modal window for “Game Over”
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2((float)window.width, (float)window.height));

        ImGuiWindowFlags over_flags =
            ImGuiWindowFlags_NoTitleBar
          | ImGuiWindowFlags_NoResize
          | ImGuiWindowFlags_NoMove
          | ImGuiWindowFlags_NoScrollbar
          | ImGuiWindowFlags_NoSavedSettings
          | ImGuiWindowFlags_NoFocusOnAppearing;

        ImGui::Begin("GameOverMenu", nullptr, over_flags);

        // Center the “caught” message & Restart button
        float window_w = (float)window.width;
        float window_h = (float)window.height;

        std::string over_text    = "Oh no, we have been caught!";
        std::string restart_text = "Play Again";

        ImVec2 text_size = ImGui::CalcTextSize(over_text.c_str());
        float   text_x    = (window_w - text_size.x) * 0.5f;
        float   text_y    = window_h * 0.45f;

        ImGui::SetCursorPosX(text_x);
        ImGui::SetCursorPosY(text_y);
        ImGui::Text("%s", over_text.c_str());

        ImVec2 button_size = ImVec2(160.0f, 60.0f);
        float   button_x    = (window_w - button_size.x) * 0.5f;
        float   button_y    = text_y + text_size.y + 40.0f;

        ImGui::SetCursorPosX(button_x);
        ImGui::SetCursorPosY(button_y);
        if (ImGui::Button(restart_text.c_str(), button_size)) {
            // Reset everything for a new playthrough:
            game_over    = false;
            game_started = true;   // already true, but keep for clarity

            loop_initialize();
            timer.update();
        }

        ImGui::End();
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 5) Draw any optional debug overlays (wireframe, global frame) as before
    // ─────────────────────────────────────────────────────────────────────────
    if (gui.display_frame)
        draw(global_frame, environment);
    if (gui.display_wireframe) {
        draw_wireframe(shark.drawable, environment);
        draw_wireframe(turtle.drawable, environment);
    }
}

void scene_structure::display_gui()
{
    ImGui::Checkbox("Frame", &gui.display_frame);
    ImGui::Separator();

    ImGui::Checkbox("Wireframe", &gui.display_wireframe);
    ImGui::Separator();

    ImGui::Checkbox("First Player View", &gui.first_player_view);
    ImGui::Separator();

    ImGui::Text("Move Turtle");

    const float button_step = 0.2f;  // movement per click

    if (ImGui::ArrowButton("##Up", ImGuiDir_Up))
        turtle.move({ 0, +button_step, 0 }, timer.t);
    ImGui::SameLine();
    if (ImGui::ArrowButton("##Left", ImGuiDir_Left))
        turtle.move({ -button_step, 0, 0 }, timer.t);
    ImGui::SameLine();
    if (ImGui::ArrowButton("##Right", ImGuiDir_Right))
        turtle.move({ +button_step, 0, 0 }, timer.t);
    ImGui::SameLine();
    if (ImGui::ArrowButton("##Down", ImGuiDir_Down))
        turtle.move({ 0, -button_step, 0 }, timer.t);
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
        turtle.move(delta,timer.t);
    }
}

void scene_structure::idle_frame()
{
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
