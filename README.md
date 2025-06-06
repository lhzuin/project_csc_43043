Turtle Rider

A C++/OpenGL survival game where you control a turtle carrying Nemo through an underwater environment, avoiding sharks and anglerfish. The goal is to survive as long as possible while difficulty ramps up over time.

Features
	•	Skinned 3D Characters
	•	Turtle (player) with limb animations and smooth banking/tilting
	•	Nemo follows the turtle’s movements
	•	Sharks and anglerfish (NPCs) with skeletal animation and procedural spawning
	•	School of instanced fish for additional ambiance
	•	Procedural Bubble Particle System
	•	Up to ~2 000 000 instanced bubble sprites
	•	GPU‐driven “swirl” and “falling” effects in the vertex shader
	•	Underwater caustics (animated texture array) and volumetric fog
	•	Dynamic NPC Behavior
	•	Randomized, distance‐based spawning over the player
	•	Sharks and anglerfish alternate randomly as threats
	•	NPCs interpolate linearly toward a “target point” around the turtle and orient themselves toward movement direction
	•	Spawn distance and horizontal jitter decrease over time to increase challenge
	•	Zone Boundary Mechanic
	•	A spherical “bubble radius” around the turtle defines a safe zone
	•	If the player leaves the zone for more than a few seconds, a warning appears and then game over by drowning
	•	Two Camera Modes
	•	First‐person (just behind the turtle)
	•	Third‐person (elevated, trailing view)
	•	ImGui Overlays
	•	Start menu with splash screen and “Play” button
	•	In‐game timer overlay (time alive)
	•	Warning banner when exiting the safe zone
	•	Game Over screen showing final and high scores

Dependencies
	•	A C++17‐compatible compiler (e.g. g++, clang++)
	•	GLFW (for window/context/input)
	•	GLAD or another OpenGL loader
	•	GLM (math library)
	•	ImGui (for the UI overlays)
	•	tinygltf (for loading .glTF / .glb models)
	•	stb_image (for PNG/JPG texture loading)
	•	cgp (the Custom Graphics Package for mesh management, shader wrappers, etc.—included as a submodule or local folder)
	•	OpenGL 3.3+ core profile

Build Instructions
	1.	Clone the repository (including submodules):

git clone --recursive https://github.com/your‐username/turtle_rider.git
cd turtle_rider


	2.	Create a build directory and run CMake:

mkdir build
cd build
cmake .. 
make

	•	Adjust CMAKE_PREFIX_PATH or PKG_CONFIG_PATH if your libraries (GLFW, GLM, ImGui) are installed in non‐standard locations.

	3.	Run the executable:

./TurtleRider

The game binary will open a window titled “Turtle Rider” and display the start menu.

Controls
	•	Arrow Keys: Move the turtle on the X–Z plane
	•	Mouse (with no modifier): Rotate the camera around the scene
	•	Shift + Mouse: Temporarily disable camera orbit (free look disabled)
	•	ImGui “Play” Button on the start screen to begin
	•	ImGui “Play Again” Button on the Game Over screen to restart

Project Structure

├── assets/          
│   ├── shark/          # .gltf model + textures for the shark
│   ├── anglerfish/     # .gltf model + textures for the anglerfish
│   ├── sea_turtle/     # .gltf + textures for the turtle
│   ├── nemo/           # .gltf + textures for Nemo
│   ├── blue_powder_tang/ # .gltf + textures for instanced fish
│   ├── caustics/       # 240‐frame sequence for animated caustics
│   └── ui/             # splash screen PNGs
│
├── include/            # Header files
│   ├── actors/         # Actor classes (turtle_actor, shark_actor, angler_actor, fish_actor, npc_actor, etc.)
│   ├── loader/         # glTF loader, GPU skin helper
│   ├── particle_system/ # Particle system code for bubbles
│   ├── environment.hpp # Fog, lighting, caustics parameters
│   └── scene.hpp       # Main scene structure (camera, actors, game logic)
│
├── src/                # Source files
│   ├── actors/         # Implementation of actor behaviors and animation
│   ├── loader/         # glTF import logic (tinygltf + stb_image)
│   ├── particle_system/ # GPU‐driven bubble particle code
│   └── scene.cpp       # Main application loop, ImGui setup, game state
│
├── shaders/            # GLSL shaders
│   ├── actor.vert.glsl
│   ├── custom_mesh.frag.glsl
│   ├── instanced_fish.vert.glsl
│   ├── particle.vert.glsl
│   └── particle.frag.glsl
│
├── CMakeLists.txt      
└── README.md           

	•	scene.cpp / scene.hpp
	•	Controls the main game loop, camera re‐anchoring, UI overlays, NPC spawning, and zone checks.
	•	actors/
	•	skinned_actor.hpp – base class to handle GPU skinning (uploading uBones[64] matrices).
	•	turtle_actor.cpp/hpp – player logic and animation.
	•	nemo_actor.cpp/hpp – follows the turtle with a fixed offset.
	•	npc_actor.hpp – generic interface for moving toward a target and aligning.
	•	shark_actor.cpp/hpp, angler_actor.cpp/hpp – specialized NPCs with skeletal animation groups and dynamic spawn decay.
	•	fish_actor.cpp/hpp – instanced fish school (no per‐joint animation beyond uploading a bind pose).
	•	loader/
	•	gltf_loader.cpp/hpp – minimal TinyGLTF wrapper extracting POSITION, NORMAL, TEXCOORD, JOINTS/WEIGHTS, inverseBindMatrices, and textures.
	•	gpu_skin_helper.hpp – utility to add JOINTS/WEIGHTS attributes into a VAO.
	•	particle_system/
	•	Implements a single‐buffer GPU instancing of bubble sprites, with per‐instance seed3 that drives a falling + swirl effect entirely in the vertex shader.
	•	environment.hpp
	•	Stores global uniforms (camera matrices, light position, fog parameters, caustic parameters).

How It Works
	1.	Initialization
	•	Load all skinned models (turtle, shark_proto, angler_proto, fish_instanced_shader, nemo) using TinyGLTF.
	•	Initialize the bubble particle system (texture array from 240 caustic images).
	•	Set up OpenGL state (depth test, blending, face culling).
	•	Position the turtle at the origin and compute the initial “bubble_center” and “bubble_radius.”
	2.	Game Loop (display_frame)
	•	Update a high‐precision timer (dt).
	•	If not started: show the ImGui full‐screen menu with a “Play” button.
	•	If playing:
	•	Animate and draw the turtle and Nemo.
	•	Animate and draw instanced fish.
	•	For each NPC in scene.npcs (mixed sharks & anglerfish):
	1.	update_position(dt) → move toward its current target.
	2.	animate(t) → update joint rotations (jaw, body wave, etc.).
	3.	draw(...) → render with the actor shader.
	4.	check_for_collision(turtle) → if true, set game_over.
	5.	check_for_end_of_life() → if true, remove that NPC from the vector and call spawn_npc() to append a new one.
	•	Check whether the turtle has left the spherical “bubble zone” via check_turtle_in_current(dt). If outside for more than 4 s, issue a warning, then kill the player.
	•	Display ImGui overlays: time alive, zone warning (if any).
	•	Re‐anchor the camera based on the turtle’s position + chosen offset (first/third person).
	•	Render bubble particles (upload uniforms + draw).
	•	If game over: show an ImGui full‐screen “Game Over” window with final/high scores and a “Play Again” button.
	3.	NPC Spawning (spawn_npc)
	•	Randomly choose between a copy-of-shark_proto or a copy-of-angler_proto.
	•	Call start_position(turtle):
	•	Compute a random horizontal jitter around the turtle.
	•	Place the NPC at height current_spawn_dist = max(min_spawn_distance, spawn_distance).
	•	Set a random swim-to target in front of the turtle, then reflect to ensure downward movement.
	•	Assign a random speed in [2, 8] units/s, and decrement spawn_distance and target_dist by their respective decay rates (clamped to minimums).
	•	Each subsequent spawn is closer/more “on top of” the player to increase difficulty.
	4.	Collision Detection
	•	Each NPC uses a cylinder‐vs‐AABB test in its local (R·S)^{-1} space around the turtle:
	•	Transform the world‐space delta between bounding‐box centers into NPC‐local space using (R·S)⁻¹ = S⁻¹·Rᵀ.
	•	Compare horizontal distance (dx, dy) to shrunk radius and vertical distance (dz) to half‐height.
	5.	Zone Boundary (“Drowning”)
	•	At game start, store turtle’s initial position as bubble_center.
	•	Each frame, measure distance between turtle and bubble_center.
	•	If turtle goes outside bubble_radius for more than outside_time_limit (4 s):
	•	Issue a one‐time ImGui warning (“Return in X.Y s!”).
	•	After timeout, game_over due to drowning, and the final Game Over message changes accordingly.

Controls
	•	Arrow Keys
	•	Up / Down / Left / Right → Move the turtle on the X–Z plane
	•	Mouse
	•	Move to orbit / look around (unless Shift is held)
	•	Shift + Mouse
	•	Temporarily disable camera orbit
	•	ImGui
	•	“Play” on the start screen
	•	“Play Again” on the Game Over screen

Future Improvements
	•	More fluid anglerfish animations (full‐body wave, lantern pulsation, fin movement)
	•	More advanced NPC AI (Bezier‐curve paths, predictive steering, group tactics)
	•	Additional enemies (octopi, barracudas) using the same skinned pipeline
	•	Enhanced visuals (sunlight shafts, underwater light caustics, bloom on angler lantern)
	•	Performance optimizations (frustum culling, LODs, instanced environmental details)

License & Credits

This project relies on several third‐party libraries: GLFW, GLAD, GLM, ImGui, tinygltf, stb_image, and the custom cgp framework for mesh/shader management. All assets (models, textures, shaders) are included under their respective licenses (see /assets/*/LICENSE where applicable).

⸻

Authors:
Ariel Silva Claudino & Luis Henrique Zuin Ruiz
CSC 43043 – Computer Graphics (Spring 2025)