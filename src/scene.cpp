#include "scene.hpp"
#include "animated_texture.hpp"


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
	

	// Display general information
	display_info();
	// Create the global (x,y,z) frame
	global_frame.initialize_data_on_gpu(mesh_primitive_frame());


	// Create the shapes seen in the 3D scene
	// ********************************************** //

	float L = 5.0f;
	

	

	
	turtle_shader.load(
		project::path + "shaders/turtle/turtle.vert.glsl",
		project::path + "shaders/mesh/custom_mesh.frag.glsl");

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
	vec3 turtle_pos = {0.2f, 0.4f, 0.5f};
	turtle.drawable.model.translation = turtle_pos;


	shark.drawable.model.rotation = rotation_transform::from_axis_angle({1, 0, 0}, Pi / 2.0f);
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

// This function is called permanently at every new frame
// Note that you should avoid having costly computation and large allocation defined there. This function is mostly used to call the draw() functions on pre-existing data.
void scene_structure::display_frame()
{

	// Set the light to the current position of the camera
	environment.light = camera_control.camera_model.position();

	// advance clock
	timer.update();
	environment.uniform_generic.uniform_float["time"] = timer.t;

	/* ----- build skin matrices each frame -------------------- */
	float t = timer.t;
	
	/* ------------ flap angles -------------------------------------- */
    float aFront = 0.1f * std::sin( 2.0f * timer.t );        // front pair
    float aRear  = 0.1f * std::sin( 2.0f * timer.t + cgp::Pi ); // rear 180°

	turtle.reset_pose();
	turtle.rotate_group("RF", {0,0,1},  aFront);
	turtle.rotate_group("LF", {0,0,1},  aFront);
	turtle.rotate_group("RR", {0,0,1},  aRear );
	turtle.rotate_group("LR", {0,0,1},  aRear );
	turtle.upload_pose_to_gpu();        
	draw(turtle.drawable, environment);       

	/* --------- REQUIN ------------------------------------------------ */
	float f  = 0.2f;                            // Hz : battements / seconde
	float w  = 2.0f * cgp::Pi * f;              // pulsation
	float A  = 0.16f;                           // amplitude max au bout
	float amplitude_ratio = 0.5f;
	float lag= 0.40f;                           // retard (rad) entre vertèbres
	float jaw_A = 0.15f;
	float fin_A = 0.12f;

	/* ======== SHARK ======================================================= */
	shark.reset_pose();

	/* -------------- corps (onde qui se propage) ------------------- */
	std::array<std::string_view,4> seg = {
		"Body0","Body1","Body2", "Body3"
	};
	for (size_t i=0;i<seg.size();++i){
		float amp   = A * (amplitude_ratio + (1-amplitude_ratio)*i/seg.size());   // rampe 50 % →100 %
		float phase = w*t - i*lag;                      // onde vers l’arrière
		shark.rotate_group(seg[i], {0,0,1}, amp*std::sin(phase));
	}
	/* -------------- Tail (with the same mouvement as the last part of the body) ------------------- */
	size_t last = seg.size() - 1;           
	float amp_last   = A * ( amplitude_ratio
						+ (1 - amplitude_ratio) * last / (seg.size()) );
	float phase_last = w*t - last * lag;

	shark.rotate_group("Tail", {0,0,1}, amp_last* std::sin(phase_last));

	/* -------------- nageoires pectorales ---------------------------------- */
	/* petite battue anti-roulis décalée d’un quart de période                */
	float fin = fin_A * std::sin(w*t + cgp::Pi/2);
	shark.rotate_group("FinL",{0,1,0},  fin);   // gauche = +Z
	shark.rotate_group("FinR",{0,1,0}, -fin);   // droite = −Z

	/* -------------- mâchoire ---------------------------------------------- */
	float jaw = jaw_A * std::max(0.f, std::sin(w*t));   // ouvre 1× par cycle
	shark.rotate_group("Jaw",{1,0,0}, jaw);

	/* -------------- upload & draw ----------------------------------------- */
	shark.upload_pose_to_gpu();
	draw(shark.drawable, environment);

	// conditional display of the global frame (set via the GUI)
	if (gui.display_frame)
		draw(global_frame, environment);
	

	// Draw all the shapes
	

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