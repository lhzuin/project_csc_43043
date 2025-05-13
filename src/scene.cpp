#include "scene.hpp"


using namespace cgp;

void deform_terrain(mesh& m)
{
	// Set the terrain to have a gaussian shape
	for (int k = 0; k < m.position.size(); ++k)
	{
		vec3& p = m.position[k];
		float d2 = p.x*p.x + p.y * p.y;
		float z = exp(-d2 / 4)-1;

		z = z + 0.05f*noise_perlin({ p.x,p.y });

		p = { p.x, p.y, z };
	}

	m.normal_update();
}

// This function is called only once at the beginning of the program
// This function can contain any complex operation that can be pre-computed once
void scene_structure::initialize()
{
	std::cout << "Start function scene_structure::initialize()" << std::endl;

	// Set the behavior of the camera and its initial position
	// ********************************************** //
	camera_control.initialize(inputs, window); 
	camera_control.set_rotation_axis_z(); // camera rotates around z-axis
	//   look_at(camera_position, targeted_point, up_direction)
	camera_control.look_at(
		{ 5.0f, -4.0f, 3.5f } /* position of the camera in the 3D scene */,
		{0,0,0} /* targeted point in 3D scene */,
		{0,0,1} /* direction of the "up" vector */);

	// Display general information
	display_info();
	// Create the global (x,y,z) frame
	global_frame.initialize_data_on_gpu(mesh_primitive_frame());


	// Create the shapes seen in the 3D scene
	// ********************************************** //

	float L = 5.0f;
	mesh terrain_mesh = mesh_primitive_grid({ -L,-L,0 }, { L,-L,0 }, { L,L,0 }, { -L,L,0 }, 100, 100);
	deform_terrain(terrain_mesh);
	terrain.initialize_data_on_gpu(terrain_mesh);
	terrain.texture.load_and_initialize_texture_2d_on_gpu(project::path + "assets/sand.jpg");

	float sea_w = 8.0;
	float sea_z = -0.8f;
	water.initialize_data_on_gpu(mesh_primitive_grid({ -sea_w,-sea_w,sea_z }, { sea_w,-sea_w,sea_z }, { sea_w,sea_w,sea_z }, { -sea_w,sea_w,sea_z }));
	water.texture.load_and_initialize_texture_2d_on_gpu(project::path + "assets/sea.png");

	tree.initialize_data_on_gpu(mesh_load_file_obj(project::path + "assets/palm_tree/palm_tree.obj"));
	tree.model.rotation = rotation_transform::from_axis_angle({ 1,0,0 }, Pi / 2.0f);
	tree.texture.load_and_initialize_texture_2d_on_gpu(project::path + "assets/palm_tree/palm_tree.jpg", GL_REPEAT, GL_REPEAT);
	turtle_shader.load(
		project::path + "shaders/turtle/turtle.vert.glsl",
		project::path + "shaders/mesh/mesh.frag.glsl");

	turtle.load_from_gltf(
        project::path+"assets/sea_turtle/sea_turtle.gltf",
        turtle_shader);

    turtle.groups["RF"] = {  2,  3,  4,  5 };   // right-front flipper
    turtle.groups["RR"] = {  6,  7,  8,  9 };   // right-rear
    turtle.groups["LF"] = { 10, 11, 12, 13 };   // left-front
    turtle.groups["LR"] = { 14, 15, 16, 17 };   // left-rear

	shark.load_from_gltf(project::path+"assets/shark/scene.gltf",turtle_shader);
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
	shark.groups["FinL"]  = { 20, 21, 22};   // Pectoral L (01, 02)
	shark.groups["FinR"]  = { 25, 26, 27 };   // Pectoral R

	shark.groups["Jaw"]  = { 29, 30};  
	turtle.drawable.model.rotation = rotation_transform::from_axis_angle({1, 0, 0}, Pi / 2.0f);
	vec3 position = {0.2f, 0.4f, 0.5f};
	turtle.drawable.model.translation = position;


	shark.drawable.model.rotation = rotation_transform::from_axis_angle({1, 0, 0}, Pi / 2.0f);
	position = {1.0f, 2.0f, 3.5f};
	shark.drawable.model.translation = position;
	
	

	cube1.initialize_data_on_gpu(mesh_primitive_cube({ 0,0,0 }, 0.5f));
	cube1.model.rotation = rotation_transform::from_axis_angle({ -1,1,0 }, Pi / 7.0f);
	cube1.model.translation = { 1.0f,1.0f,-0.1f };
	cube1.texture.load_and_initialize_texture_2d_on_gpu(project::path + "assets/wood.jpg");

	cube2 = cube1;


}

// This function is called permanently at every new frame
// Note that you should avoid having costly computation and large allocation defined there. This function is mostly used to call the draw() functions on pre-existing data.
void scene_structure::display_frame()
{

	// Set the light to the current position of the camera
	environment.light = camera_control.camera_model.position();

	// advance clock
	timer.update();

	/* ----- build skin matrices each frame -------------------- */
	float t = timer.t;
	/* ------------ flap angles -------------------------------------- */
    float aFront = 0.1f * std::sin( 2.0f * timer.t );        // front pair
    float aRear  = 0.1f * std::sin( 2.0f * timer.t + cgp::Pi ); // rear 180°
	float f  = 0.2f;                            // Hz : battements / seconde
	float w  = 2.0f * cgp::Pi * f; 

	turtle.reset_pose();                  // remet la bind-pose

	float aF = 0.5f * std::sin( 2*w*t );  // w = 2πf
	float aR = 0.5f * std::sin( 2*w*t + cgp::Pi );

	/* indices des racines de nageoires (voir le print) */
	turtle.rotate_joint( 2, {0,0,1},  aF);   // RF
	turtle.rotate_joint(10, {0,0,1},  aF);   // LF
	turtle.rotate_joint( 6, {0,0,1},  aR);   // RR
	turtle.rotate_joint(14, {0,0,1},  aR);   // LR

	turtle.update_pose();      // => remplit uBones
	turtle.upload_pose_to_gpu();
	draw(turtle.drawable, environment);    

	

	

	// conditional display of the global frame (set via the GUI)
	if (gui.display_frame)
		draw(global_frame, environment);
	

	// Draw all the shapes
	draw(terrain, environment);
	draw(water, environment);
	draw(tree, environment);
	draw(cube1, environment);

	// Animate the second cube in the water
	cube2.model.translation = { -1.0f, 6.0f+0.1*sin(0.5f*timer.t), -0.8f + 0.1f * cos(0.5f * timer.t)};
	cube2.model.rotation = rotation_transform::from_axis_angle({1,-0.2,0},Pi/12.0f*sin(0.5f*timer.t));
	draw(cube2, environment);

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
void scene_structure::idle_frame()
{
	camera_control.idle_frame(environment.camera_view);
}


void scene_structure::display_info()
{
	std::cout << "\nCAMERA CONTROL:" << std::endl;
	std::cout << "-----------------------------------------------" << std::endl;
	std::cout << camera_control.doc_usage() << std::endl;
	std::cout << "-----------------------------------------------\n" << std::endl;


	std::cout << "\nSCENE INFO:" << std::endl;
	std::cout << "-----------------------------------------------" << std::endl;
	std::cout << "Display here the information you would like to appear at the start of the program on the command line (file scene.cpp, function display_info())." << std::endl;
	std::cout << "-----------------------------------------------\n" << std::endl;
}