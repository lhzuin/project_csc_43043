#include "scene.hpp"
#include "animated_texture.hpp"
#include "shark_actor.hpp"

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

    turtle.drawable.model.rotation = rotation_transform::from_axis_angle({ 1, 0, 0 }, Pi / 2.0f);
    vec3 turtle_pos = { 0.2f, 0.4f, 0.5f };
    turtle.drawable.model.translation = turtle_pos;

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

    spawn_shark();
    
}


void scene_structure::spawn_shark()
{
    // clear out old shark(s)
    sharks.clear();

    // create a brand‐new shark and give it a random start
    shark_actor s;
    s.initialize(turtle_shader,
        project::path + "assets/shark/scene.gltf",
        project::path + "assets/shark/textures/SharkBody.png");
    s.start_position(turtle);
    sharks.push_back(std::move(s));
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
    // Set the light to the current position of the camera
	environment.light = camera_control.camera_model.position();

	if (!game_over) {
		float t_prev = timer.t;

		// advance clock
		timer.update();
		float dt = timer.t - t_prev;
		environment.uniform_generic.uniform_float["time"] = timer.t;

		
		/* ------------ Turtle -------------------------------------- */
		float aFront = 0.1f * std::sin( 2.0f * timer.t );        // front pair
		float aRear  = 0.1f * std::sin( 2.0f * timer.t + cgp::Pi ); // rear 180°

		turtle.reset_pose();
		turtle.rotate_group("RF", {0,0,1},  aFront);
		turtle.rotate_group("LF", {0,0,1},  aFront);
		turtle.rotate_group("RR", {0,0,1},  aRear );
		turtle.rotate_group("LR", {0,0,1},  aRear );
		turtle.upload_pose_to_gpu();
		draw(turtle.drawable, environment);          
			

		/* ======== SHARK ======================================================= */
        // only one shark in the vector:
        shark_actor& sh = sharks[0];
        sh.update_position(dt);
        sh.animate(timer.t);
        sh.upload_pose_to_gpu();
        draw(sh.drawable, environment);

        // only retire & respawn if *not* eaten:
        if (!sh.check_for_collision(turtle)) {
            if(sh.check_for_end_of_life()){
                spawn_shark();
            }
        }
        else {
            // collision happened → game over logic remains as you had it
            game_over = true;
        }
        handle_keyboard_movement();
	}
	else {
		draw(turtle.drawable, environment);
		ImGui::Begin("Game"); 
		ImGui::Text("💥 Turtle got eaten!");
		if (ImGui::Button("Restart")) {
			game_over = false;
			timer.update();
            initialize();      // reset everything
		}
		ImGui::End();
	}



	// conditional display of the global frame (set via the GUI)
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
