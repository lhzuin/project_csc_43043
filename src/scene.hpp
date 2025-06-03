// scene.hpp
#pragma once

#include "cgp/cgp.hpp"
#include "environment.hpp"
#include "loader/gltf_loader.hpp"
#include "loader/gpu_skin_helper.hpp" 
#include "actors/skinned_actor.hpp"
#include "actors/shark_actor.hpp"
#include "actors/turtle_actor.hpp"
#include "particle_system.hpp"
#include "actors/nemo_actor.hpp"

// Variables associated to the GUI (buttons, etc)
struct gui_parameters {
    bool display_frame = false;
    bool display_wireframe = false;
    bool first_player_view = false;
};

// The structure of the custom scene
struct scene_structure : cgp::scene_inputs_generic {

    // ****************************** //
    // Elements and shapes of the scene
    // ****************************** //
    camera_controller_orbit_euler camera_control;
    camera_projection_perspective camera_projection;
    window_structure               window;


    std::vector<shark_actor> sharks;   // we’ll keep only one shark alive at a time

    // helper to spawn one shark
    void spawn_shark();

	shark_actor shark;

	// Collision mechanism
	bool   game_over   = false;
    bool   game_started = false;                //to control the main game menu
    mesh_drawable          global_frame;        // The standard global frame
    environment_structure  environment;         // Standard environment controller
    input_devices          inputs;              // Mouse, keyboard, window size…
    gui_parameters         gui;                 // GUI state

    turtle_actor          turtle;
    opengl_shader_structure actor_shader;

    nemo_actor nemo;

    timer_basic timer;

    cgp::vec3 camera_offset;

    ParticleSystem   particle_system;

    void handle_keyboard_movement();               // poll arrows each frame

    void initialize();    // called once before the loop
    void display_frame(); // called every frame to draw
    void display_gui();   // ImGui widgets

    void mouse_move_event();
    void mouse_click_event();
    void keyboard_event();
    void idle_frame();    // called every frame before display_frame()

    void display_info();
};
