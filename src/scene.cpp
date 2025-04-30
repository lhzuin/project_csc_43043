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
		

	gltf_geometry_and_texture turtle_data = mesh_load_file_gltf(
        project::path + "assets/sea_turtle/sea_turtle.gltf");

	opengl_shader_structure turtle_shader;
	turtle_shader.load(
		project::path + "shaders/turtle/turtle.vert.glsl",
		project::path + "shaders/mesh/mesh.frag.glsl");
	
	/* Send the geometry to the GPU, set shader and assign texture */
	turtle.initialize_data_on_gpu(
        turtle_data.geom,          // geometry
        turtle_shader,             // <-- keep custom shader!
        turtle_data.tex); 
	
	turtle.model.rotation = rotation_transform::from_axis_angle({1, 0, 0}, Pi / 2.0f);
	//turtle.model.rotation = rotation_transform::from_axis_angle({0, 1, 0}, Pi);
	vec3 position = {0.2f, 0.4f, 0.5f};
	turtle.model.translation = position;
	
	/* ---- add JOINTS_0 / WEIGHTS_0 to the VAO ----------------- */
	add_skin_attributes(turtle,
		turtle_data.joint_index,
		turtle_data.joint_weight);

	/* ---- keep inverse-bind & joint-node for later ------------ */
	inverse_bind  = std::move(turtle_data.inverse_bind);   // store as scene members
	joint_node    = std::move(turtle_data.joint_node);

	// glTF files are +Y-up, right-handed.  
	// rotate −90 ° around X to lie the turtle flat in X-Z.
	

	uBones.resize(inverse_bind.size());
	

	cube1.initialize_data_on_gpu(mesh_primitive_cube({ 0,0,0 }, 0.5f));
	cube1.model.rotation = rotation_transform::from_axis_angle({ -1,1,0 }, Pi / 7.0f);
	cube1.model.translation = { 1.0f,1.0f,-0.1f };
	cube1.texture.load_and_initialize_texture_2d_on_gpu(project::path + "assets/wood.jpg");

	cube2 = cube1;


}

const int RF[] = {  2,  3,  4,  5};          // right-front
const int RR[] = {  6,  7,  8,  9};          // right-rear
const int LF[] = { 10, 11, 12, 13};          // left-front
const int LR[] = { 14, 15, 16, 17};          // left-rear
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

    /* ------------ build the 24 skin matrices ----------------------- */
    for (size_t j = 0; j < uBones.size(); ++j)
    {
        mat4 bind = inverse( inverse_bind[j] );          // rest-pose
        mat4 M    = bind;                                // default

        /* ---- does this joint belong to a flipper? ----------------- */
        auto rotate_if_in = [&](const int* list, float ang)
        {
            for(int k=0;k<4;++k) if(j==list[k])
            {
                /* rotate around local +Z (up/out of the shell plane) */
                mat4 R = affine_rt(
                           rotation_transform::from_axis_angle({0,0,1}, ang),
                           {0,0,0}).matrix();
                M = bind * R;                            // hinge at joint
            }
        };

        rotate_if_in(RF, aFront);
        rotate_if_in(LF, aFront);
        rotate_if_in(RR, aRear );
        rotate_if_in(LR, aRear );

        uBones[j] = M * inverse_bind[j];
    }

    /* ------------ upload to the *turtle* shader only --------------- */
    glUseProgram( turtle.shader.id );
    for (size_t j = 0; j < uBones.size(); ++j)
    {
        std::string n = "uBones[" + std::to_string(j) + "]";
        GLint loc = glGetUniformLocation(turtle.shader.id, n.c_str());
        if (loc >= 0)
            glUniformMatrix4fv(loc,1,GL_FALSE,&uBones[j](0,0));
    }
    glUseProgram( 0 );


	// conditional display of the global frame (set via the GUI)
	if (gui.display_frame)
		draw(global_frame, environment);
	

	// Draw all the shapes
	draw(terrain, environment);
	draw(water, environment);
	draw(tree, environment);
	draw(cube1, environment);
	draw(turtle, environment);

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