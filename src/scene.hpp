﻿// scene.hpp
#pragma once

#include "cgp/cgp.hpp"
#include "environment.hpp"
#include "loader/gltf_loader.hpp"
#include "loader/gpu_skin_helper.hpp" 
#include "actors/skinned_actor.hpp"
#include "actors/shark_actor.hpp"
#include "actors/turtle_actor.hpp"

// Variables associated to the GUI (buttons, etc)
struct gui_parameters {
    bool display_frame = true;
    bool display_wireframe = false;
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
    mesh_drawable          global_frame;        // The standard global frame
    environment_structure  environment;         // Standard environment controller
    input_devices          inputs;              // Mouse, keyboard, window size…
    gui_parameters         gui;                 // GUI state

    turtle_actor          turtle;
    opengl_shader_structure turtle_shader;

    std::vector<cgp::mat4> shark_inverse_bind;
    std::vector<int>       shark_joint_node;    // skin → node
    std::vector<cgp::mat4> shark_uBones;

    timer_basic            timer;

    mesh_drawable          terrain, water, tree;
    mesh_drawable          cube1, cube2;

    cgp::vec3 camera_offset;

    void handle_keyboard_movement();               // poll arrows each frame

    // ****************************** //
    // Functions
    // ****************************** //

    void initialize();    // called once before the loop
    void display_frame(); // called every frame to draw
    void display_gui();   // ImGui widgets

    void mouse_move_event();
    void mouse_click_event();
    void keyboard_event();
    void idle_frame();    // called every frame before display_frame()

    void display_info();
};
