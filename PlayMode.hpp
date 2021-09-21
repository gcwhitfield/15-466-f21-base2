#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>
#include "Mesh.hpp"
#include <vector>
#include <deque>
#include <random>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----
	enum GameState
	{
		GAMEPLAY,
		WIN
	};

	GameState curr_state = GAMEPLAY;

	unsigned int score = 0; //incrased when the player collets a sugar cube
	

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//transforms for game objects
	Scene::Transform *spoon = nullptr;
	Scene::Transform *mug_body = nullptr;
	Scene::Transform *mug_handle = nullptr;
	Scene::Transform *table = nullptr;
	Scene::Transform *sugar_container = nullptr;

	Scene::Drawable sugar_ref;
	std::list < Scene::Transform *> sugar_cubes;

	//mesh data for game objects
	Mesh spoon_m;
	Mesh mug_body_m;
	Mesh mug_handle_m;
	Mesh table_m;
	Mesh sugar_container_m;
	Mesh sugar_ref_m;
	std::list< Mesh > sugar_cubes_m;
	MeshBuffer mesh_buffer;

	glm::quat hip_base_rotation;
	glm::quat upper_leg_base_rotation;
	glm::quat lower_leg_base_rotation;
	float wobble = 0.0f;
	
	//camera:
	Scene::Camera *camera = nullptr;

};
