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

    // Reset timer to zero and immediately update so dt starts fresh
    timer.update();
    start_time = timer.t;
    

    // Create the shapes seen in the 3D scene
    // ********************************************** //

    turtle.start_position();

    nemo.start_position();
    
    fish.start_position();

    // ── NEW: initialize bubble center & clear timers/warnings ──
    bubble_center = turtle.base_translation;  // center the sphere at turtle’s start
    warning_issued = false;
    outside_timer = 0.0f;
    died_by_drowning = false;   //NEW: clear drowning‐flag each time we start
    // bubble_radius and outside_time_limit remain whatever they are in the header
    
    // ───────────────────────────────────────────────────────────────────
    // Compute initial camera position based on gui.first_player_view
    vec3 base = turtle.base_translation;
    vec3 offset = gui.first_player_view
            ? vec3{ 0.0f, -0.5f, 0.3f }
        : vec3{ 0.0f, -1.8f, 1.0f };
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
    npcs.clear();

    spawn_npc();

    
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

    shark_proto.initialize(actor_shader,
        project::path + "assets/shark/scene.gltf",
        project::path + "assets/shark/textures/SharkBody.png");
    angler_proto.initialize(actor_shader,
        project::path + "assets/anglerfish/scene.gltf",
        project::path + "assets/anglerfish/textures/unshaded_angler_baseColor.png");

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

    particle_system.initialize(environment, vert_path, frag_path, max_particles, bubble_center);

    // Optionally, immediately set your desired parameters
    particle_parameters.fog_col_in = cgp::vec3(environment.background_color);
    particle_parameters.fog_dist_in = environment.fog_d_max;

    particle_system.set_parameters(particle_parameters);

    //-----------------------------------------------------------------
    //  load the initial image
    //-----------------------------------------------------------------
    image_structure im =
        cgp::image_load_file(project::path + "assets/ui/turtle_rider.png");

    splash_size = ImVec2((float)im.width, (float)im.height);

    glGenTextures(1, &splash_tex);
    glBindTexture(GL_TEXTURE_2D, splash_tex);
    glTexImage2D(GL_TEXTURE_2D,
             0,                 // mip-level
             GL_RGBA8,          // internal format
             im.width,
             im.height,
             0,                 // border
             GL_RGBA,           // incoming format
             GL_UNSIGNED_BYTE,  // incoming type
             &im.data[0]);   // ← pointer, **not** a function call
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D,0);

    
}


void scene_structure::spawn_npc()
{
    // Decide randomly: 50% chance shark, 50% chance angler
    float r = (float)rand() / (float)RAND_MAX;
    if (r < 0.5f) {
        // Copy from shark_proto
        auto new_shark = std::make_unique<shark_actor>(shark_proto);
        new_shark->start_position(turtle);
        // Immediately ramp speed by difficulty:
        new_shark->speed += speed_increase_rate * gameplay_time;
        npcs.push_back(std::move(new_shark));
    }
    else {
        // Copy from angler_proto
        auto new_angler = std::make_unique<angler_actor>(angler_proto);
        new_angler->start_position(turtle);
        // We could also apply some difficulty‐based speed change if needed:
        new_angler->speed += speed_increase_rate * gameplay_time;
        npcs.push_back(std::move(new_angler));
    }
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

        float W = (float)window.width;
        float H = (float)window.height;

        /*======================================================================
        1.  Splash picture : scale to fit *inside* window while keeping aspect
            scale = min( W / imgW , H / imgH )
        ======================================================================*/
        if (splash_tex != 0)
        {
            float sf = std::min(W / splash_size.x, H / splash_size.y);
            ImVec2 imgSz = { splash_size.x * sf, splash_size.y * sf };

            // center the picture
            ImVec2 imgPos = { (W - imgSz.x)*0.5f, (H - imgSz.y)*0.5f };
            ImGui::SetCursorPos(imgPos);
            ImGui::Image((ImTextureID)(intptr_t)splash_tex, imgSz);
        }

        /*======================================================================
        2.  "Play" button in the *bottom–right* corner (with a margin)
        ======================================================================*/
        ImVec2 butSz = {160.0f, 60.0f};
        float margin = 40.0f;

        ImVec2 butPos = { W - butSz.x - margin,  H - butSz.y - margin };
        ImGui::SetCursorPos(butPos);

        if (ImGui::Button("Play", butSz))
        {
            game_started = true;
            game_over    = false;
            initialize();
        }

        ImGui::End();

        // ─────────────────────────────────────────────────────────────────
        // Because we’re still on the menu, we skip all gameplay updates/draws.
        // ─────────────────────────────────────────────────────────────────
        return;
    }


    // 3) If the game HAS started but not yet over ⇒ normal gameplay:
    if (game_started && !game_over) {
        ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f));
        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoBackground
            | ImGuiWindowFlags_NoInputs;
        ImGui::Begin("ScoreOverlay", nullptr, flags);
        ImGui::Text("Time alive: %.1f", timer.t-start_time);
        ImGui::End();

        // Advance your internal clock & animate uniforms
        float t_prev = timer.t;
        timer.update();
        dt = timer.t - t_prev;
        environment.uniform_generic.uniform_float["time"] = timer.t;


        // Accumulate total gameplay time
        gameplay_time += dt;

        /* ======== TURTLE AND NEMO ======== */
        turtle.animate(timer.t);
        draw(turtle.drawable, environment);

        nemo.follow(turtle);
        nemo.animate(timer.t);

        draw(nemo.drawable, environment);

        /* ======== FISHES ======== */
        fish.animate(timer.t);
        fish.draw(environment, camera_projection);

        // Update & draw all NPCs in the single vector:
        for (size_t i = 0; i < npcs.size(); ++i) {
            auto & actor = npcs[i];

            // Move (only sharks have update_position, but angler inherits from npc_actor too)
            actor->update_position(dt);
            actor->animate(timer.t);
            draw(actor->drawable, environment);

            // Collision check against turtle:
            if (actor->check_for_collision(turtle)) {
                // Game over:
                game_over = true;
                float final_score = timer.t - start_time;
                if (final_score > high_score)
                    high_score = final_score;
                break;
            }

            // If this NPC is “out of bounds” (i.e. end_of_life), remove & respawn
            if (actor->check_for_end_of_life()) {
                // Erase this NPC, then spawn a new random one:
                npcs.erase(npcs.begin() + i);
                spawn_npc();
                // Don’t increment i (we removed current), but continue loop:
                --i;
            }
        }

        // Handle turtle movement from keyboard arrows
        handle_keyboard_movement();
        // ─────────────────────────────────────────
        
        // Accumulate “outside” time; possibly warn or kill
        check_turtle_in_current(dt);

        if (warning_issued && !game_over) {
            ImGui::SetNextWindowPos(ImVec2(window.width * 0.5f - 160.0f, 20.0f));
            ImGuiWindowFlags warn_flags =
                ImGuiWindowFlags_NoTitleBar
                | ImGuiWindowFlags_NoResize
                | ImGuiWindowFlags_NoMove
                | ImGuiWindowFlags_NoBackground
                | ImGuiWindowFlags_NoInputs;
            ImGui::Begin("WarningWindow", nullptr, warn_flags);

            float remaining = std::max(0.0f, outside_time_limit - outside_timer);
            ImGui::TextColored(
                ImVec4(1, 0, 0, 1),
                "Warning: Return within %.1f s!", remaining
            );
            ImGui::End();
        }
        // ─────────────────────────────────────────
        
        // Re‐anchor / update the camera **every frame** based on current turtle position
        vec3 base = turtle.drawable.model.translation;
        vec3 offset = gui.first_player_view
                ? vec3{ 0.0f, -0.5f, 0.3f }
            : vec3{ 0.0f, -1.8f, 1.0f };
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
        // Draw the turtle in its final position:
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

        // 1) Final message
        std::string over_text;
        if (died_by_drowning) {
            over_text = "Oh no, you strayed too far and lost your way!";
        }
        else {
            over_text = "Oh no, we have been caught!";
        }

        ImVec2 text_size = ImGui::CalcTextSize(over_text.c_str());
        float text_x = (window_w - text_size.x) * 0.5f;
        float text_y = window_h * 0.40f;

        ImGui::SetCursorPosX(text_x);
        ImGui::SetCursorPosY(text_y);
        ImGui::Text("%s", over_text.c_str());

        // 2) Display “Your Score” (timer.t) and “High Score” (high_score) below:
        float line_y = text_y + text_size.y + 20.0f;
        ImGui::SetCursorPosX((window_w - 200.0f) * 0.5f);
        ImGui::SetCursorPosY(line_y);
        ImGui::Text("You survived %.2f seconds", timer.t-start_time);

        ImGui::SetCursorPosX((window_w - 200.0f) * 0.5f);
        ImGui::SetCursorPosY(line_y + ImGui::GetTextLineHeight() + 10.0f);
        ImGui::Text("Highest Score: %.2f seconds", high_score);

        // 3) “Play Again” button under the scores:
        ImVec2 button_size = ImVec2(160.0f, 60.0f);
        float button_x = (window_w - button_size.x) * 0.5f;
        float button_y = line_y + 2 * (ImGui::GetTextLineHeight() + 10.0f) + 30.0f;

        ImGui::SetCursorPosX(button_x);
        ImGui::SetCursorPosY(button_y);
        if (ImGui::Button("Play Again", button_size)) {
            // Reset everything for a new playthrough:
            game_over    = false;
            game_started = true;   // already true, but keep for clarity
            loop_initialize();
        }

        ImGui::End();
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 5) Draw any optional debug overlays (wireframe, global frame) as before
    // ─────────────────────────────────────────────────────────────────────────
    if (gui.display_frame)
        draw(global_frame, environment);
    if (gui.display_wireframe) {
        draw_wireframe(turtle.drawable, environment);
    }
}

void scene_structure::check_turtle_in_current(float dt)
{
    // 1) Get the turtle’s current position in world‐space:
    cgp::vec3 turtle_pos = turtle.drawable.model.translation;

    // 2) Distance to bubble_center:
    float dist = cgp::norm(turtle_pos - bubble_center);

    // 3) If she’s outside the sphere:
    if (dist > bubble_radius) {
        // 3a) If this is the first frame outside, raise a one‐time warning:
        if (outside_timer == 0.0f) {
            warning_issued = true;
        }

        // 3b) Accumulate how long she’s been outside:
        outside_timer += dt;

        // 3c) If she’s exceeded the allowed outside time, kill the turtle:
        if (outside_timer >= outside_time_limit) {
            game_over = true;
            // ── NEW: record final survival time and update high_score ──
            float final_score = timer.t - start_time;
            if (final_score > high_score) {
                high_score = final_score;
            }
            died_by_drowning = true;
            // ──────────────────────────────────────────────────────────
        }
    }
    // 4) If she’s back inside, clear everything so we can warn again next time:
    else {
        outside_timer = 0.0f;
        warning_issued = false;
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
        turtle.move({ 0, +button_step, 0 }, dt);
    ImGui::SameLine();
    if (ImGui::ArrowButton("##Left", ImGuiDir_Left))
        turtle.move({ -button_step, 0, 0 }, dt);
    ImGui::SameLine();
    if (ImGui::ArrowButton("##Right", ImGuiDir_Right))
        turtle.move({ +button_step, 0, 0 }, dt);
    ImGui::SameLine();
    if (ImGui::ArrowButton("##Down", ImGuiDir_Down))
        turtle.move({ 0, -button_step, 0 }, dt);
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
        turtle.move(delta,dt);
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
