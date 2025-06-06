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
#include "actors/fish_actor.hpp"

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
    // How quickly the shark’s speed goes up over time (units/sec^2)
    float speed_increase_rate = 0.09f;
    // track gameplay time so that respawning doesn’t reset difficulty
    float gameplay_time = 0.0f;
    mesh_drawable          global_frame;        // The standard global frame
    environment_structure  environment;         // Standard environment controller
    input_devices          inputs;              // Mouse, keyboard, window size…
    gui_parameters         gui;                 // GUI state

    turtle_actor          turtle;
    opengl_shader_structure actor_shader;
    opengl_shader_structure fish_instanced_shader;

    nemo_actor nemo;
    fish_actor fish;

    timer_basic timer;
    //Track the high score over the entire game session:
    float high_score = 0.0f;
    float start_time;
    float dt;
    
    cgp::vec3 camera_offset;
    ParticleParameters particle_parameters;
    ParticleSystem   particle_system;

    ImVec2  splash_size  = {0,0}; // original size, kept for later
    GLuint  splash_tex   = 0;


    float outside_timer = 0.0f;   // how long (in seconds) the turtle has been continuously outside
    float outside_time_limit = 4.0f;   // “predetermined amount of time” allowed outside before game over

    cgp::vec3 bubble_center = { 0.2f, 0.4f, 0.5f };
    float     bubble_radius = particle_parameters.spread_in;
    bool      warning_issued = false;
    bool      died_by_drowning = false;   // new: true if turtle ran out of “outside time.”


    void handle_keyboard_movement();               // poll arrows each frame
    void loop_initialize();  // called once before the loop
    void initialize();    
    void display_frame(); // called every frame to draw
    void display_gui();   // ImGui widgets

    void mouse_move_event();
    void mouse_click_event();
    void keyboard_event();
    void idle_frame();    // called every frame before display_frame()
    void check_turtle_in_current(float dt);
    void display_info();
};
