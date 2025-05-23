#include "environment.hpp"

// Change these global values to modify the default behavior
// ************************************************************* //
// The initial zoom factor on the GUI
float project::gui_scale = 1.0f;
// Is FPS limited automatically
bool project::fps_limiting = true;
// Maximal default FPS (used only of fps_max is true)
float project::fps_max=60.0f;
// Automatic synchronization of GLFW with the vertical-monitor refresh
bool project::vsync=true;     
// Initial dimension of the OpenGL window (ratio if in [0,1], and absolute pixel size if > 1)
float project::initial_window_size_width  = 0.5f; 
float project::initial_window_size_height = 0.5f;
// ************************************************************* //


// This path will be automatically filled when the program starts
std::string project::path = ""; 




void environment_structure::send_opengl_uniform(opengl_shader_structure const& shader, bool expected) const
{
	opengl_uniform(shader, "projection", camera_projection, expected);
	opengl_uniform(shader, "view", camera_view, expected);
	opengl_uniform(shader, "light", light, false);
	opengl_uniform(shader, "fog_d_max", fog_d_max, false);
	opengl_uniform(shader, "fog_color", fog_color, false);


	// bind it to texture unit 1
	if (caustic_array_tex) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D_ARRAY, caustic_array_tex);
		opengl_uniform(shader, "causticMapArray", 1, false);
	}

	// playback parameters
	opengl_uniform(shader, "caustic_frame_count", caustic_frame_count, false);
	opengl_uniform(shader, "caustic_fps",         caustic_fps,         false);
	opengl_uniform(shader, "caustic_intensity", caustic_intensity, false);
	opengl_uniform(shader, "caustic_scale",         caustic_scale,         false);


	uniform_generic.send_opengl_uniform(shader, expected);

}