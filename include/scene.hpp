// scene.hpp
#pragma once

#include "cgp/cgp.hpp"
#include "environment.hpp"
#include "loader/gltf_loader.hpp"
#include "loader/gpu_skin_helper.hpp" 
#include "actors/skinned_actor.hpp"
#include "actors/npc/shark_actor.hpp"
#include "actors/turtle_actor.hpp"
#include "particles/particle_system.hpp"
#include "actors/nemo_actor.hpp"
#include "actors/fish_actor.hpp"
#include "actors/npc/angler_actor.hpp"

// Variables associated to the GUI (buttons, etc)
struct gui_parameters {
    bool display_frame = false;
    bool display_wireframe = false;
    bool first_player_view = false;
};

// The structure of the custom scene
struct scene_structure : cgp::scene_inputs_generic {

    camera_controller_orbit_euler camera_control;
    camera_projection_perspective camera_projection;
    window_structure               window;


    std::vector<std::unique_ptr<npc_actor>> npcs;

    shark_actor  shark_proto;
    angler_actor angler_proto;

    void spawn_npc();

	// Collision mechanism
	bool game_over = false;
    bool game_started = false;     

    float speed_increase_rate = 0.09f;

    float gameplay_time = 0.0f;
    mesh_drawable  global_frame; // The standard global frame
    environment_structure environment; // Standard environment controller
    input_devices inputs; // Mouse, keyboard, window size…
    gui_parameters gui;

    turtle_actor turtle;
    opengl_shader_structure actor_shader;
    opengl_shader_structure fish_instanced_shader;

    nemo_actor nemo;
    fish_actor fish;

    timer_basic timer;
    float high_score = 0.0f;
    float start_time;
    float dt;
    
    cgp::vec3 camera_offset;
    ParticleParameters particle_parameters;
    ParticleSystem   particle_system;

    ImVec2  splash_size  = {0,0};
    GLuint  splash_tex   = 0;


    float outside_timer = 0.0f;  
    float outside_time_limit = 4.0f;

    cgp::vec3 bubble_center;
    float     bubble_radius = particle_parameters.spread_in;
    bool      warning_issued = false;
    bool      died_by_drowning = false; 


    void handle_keyboard_movement();  // poll arrows each frame
    void loop_initialize();  // called once before the loop
    void initialize();    
    void display_frame(); // called every frame to draw
    void display_gui();   // ImGui widgets

    void mouse_move_event();
    void mouse_click_event();
    void keyboard_event();
    void idle_frame(); 
    void check_turtle_in_current(float dt);
    void display_info();
};
