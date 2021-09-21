#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint coffee_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > coffee_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("coffee.pnct"));
	coffee_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > coffee_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("coffee.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = coffee_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = coffee_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;
	});
});

PlayMode::PlayMode() : scene(*coffee_scene), sugar_ref(new Scene::Transform()), mesh_buffer("") {

	mesh_buffer = (*coffee_meshes);

	//initialize mesh containers with cout = -1. will assert that mesh.count != -1
	//later after reading scene data
	spoon_m.count = -1;
	table_m.count = -1;
	mug_body_m.count = -1;
	mug_handle_m.count = -1;
	for (auto s : sugar_cubes_m) {
		s.count = -1;
	}

	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "Spoon") spoon = &transform;
		else if (transform.name == "Table") table = &transform;
		else if (transform.name == "MugBody") mug_body = &transform;
		else if (transform.name == "MugHandle") mug_handle = &transform;
		else if (transform.name.find("SugarCube") != std::string::npos) sugar_cubes.push_back(&transform);
	}

	for (auto it = coffee_meshes->meshes.begin(); it != coffee_meshes->meshes.end(); it++)
	{
		std::string name_m = it->first;
		Mesh m = it->second;
		if (name_m.compare("Spoon") == 0) spoon_m = m;
		else if (name_m.compare("Table") == 0) table_m = m;
		else if (name_m.compare("MugBody") == 0) mug_body_m = m;
		else if (name_m.compare("MugHandle") == 0) mug_handle_m = m;
		else if (name_m.compare("SugarRef") == 0) sugar_ref_m = m;
		else if (name_m.find("SugarCube") != std::string::npos) sugar_cubes_m.push_back(m);
	}

	if (spoon == nullptr) throw std::runtime_error("Spoon not found in blender.");
	if (table == nullptr) throw std::runtime_error("table not found in blender.");
	if (mug_body == nullptr) throw std::runtime_error("MugBody not found in blender.");
	if (mug_handle == nullptr) throw std::runtime_error("MugHandle not found in blender.");
	if (sugar_cubes.size() == 0) throw std::runtime_error("Sugar cubes not found in blender.");

	if (spoon_m.count == (GLuint)-1) throw std::runtime_error("Spoon not found in coffee_meshes->meshes.");
	if (table_m.count == (GLuint)-1) throw std::runtime_error("table not found in coffee_meshes->meshes.");
	if (mug_body_m.count == (GLuint)-1) throw std::runtime_error("MugBody not found in coffee_meshes->meshes.");
	if (mug_handle_m.count == (GLuint)-1) throw std::runtime_error("MugHandle not found in coffee_meshes->meshes.");
	for (auto s : sugar_cubes_m) {
		if (s.count == (GLuint)-1) throw std::runtime_error("Sugar cubes not found in coffee_meshes->meshes.");
	}

	for (Scene::Drawable const &drawable : scene.drawables)
	{
		if (drawable.pipeline.start == sugar_ref_m.start && drawable.pipeline.count == sugar_ref_m.count)
		{
			sugar_ref = drawable;
		}
	}

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			{ // rotate mug according to mouse movement
				mug_body->rotation = glm::normalize(
					mug_body->rotation
					* glm::angleAxis(-motion.x, glm::vec3(0.0f, 0.0f, 1.0f))
					//* glm::angleAxis(motion.y, glm::vec3(1.0f, 0.0f, 0.0f))
				);
			}
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//slowly rotates through [0,1):
	wobble += elapsed / 10.0f;
	wobble -= std::floor(wobble);

	if (curr_state == GAMEPLAY)
	{
		//move mug:
		{

			//combine inputs into a move:
			constexpr float PlayerSpeed = 10.0f;
			(void)PlayerSpeed;
			glm::vec2 move = glm::vec2(0.0f);
			if (left.pressed && !right.pressed) move.x =-1.0f;
			if (!left.pressed && right.pressed) move.x = 1.0f;
			if (down.pressed && !up.pressed) move.y =-1.0f;
			if (!down.pressed && up.pressed) move.y = 1.0f;

			//make it so that moving diagonally doesn't go faster:
			
			if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;
			mug_body->position += glm::vec3(move.x, 0, 0);
			
		}

		{ // sugarcubes raining down from the sky

			std::vector < Scene::Transform * > t_to_remove;
			std::vector < Scene::Transform * > t_to_add;
			t_to_remove.clear();
			t_to_add.clear();
			for (Scene::Transform *s : sugar_cubes)
			{
				s->position.z -= elapsed * 5;
				
				// move the cube back up if it touches the floor
				if (s->position.z < 0)
				{			
					// add points if the player collected the cube in the cup
					if (abs(s->position.x - mug_body->position.x) < 1)
					{
						score++;
						std::cout << score << std::endl;
						if (score > 25)
						{
							curr_state = WIN;
						}
					}
					s->position = glm::vec3(table->position.x + rand() % 8, mug_body->position.y, mug_body->position.z + rand() % 10 + 20);
				}
			}
		}
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); //print any errors produced by this setup code

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		if (curr_state == GAMEPLAY)
		{
			//draw game instructions
			{
				lines.draw_text("A and D move the cup left and right. Collect 25 sugar cubes to make your coffee and win!",
					glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
					glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
					glm::u8vec4(0x00, 0x00, 0x00, 0x00));
				float ofs = 2.0f / drawable_size.y;
				lines.draw_text("A and D move the cup left and right. Collect 25 sugar cubes to make your coffee and win!",
					glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
					glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
					glm::u8vec4(0xff, 0xff, 0xff, 0x00));
			}

			//draw score
			{
				lines.draw_text(std::to_string(score),
					glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H + 1.8f, 0.0),
					glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
					glm::u8vec4(0x00, 0x00, 0x00, 0x00));
				float ofs = 2.0f / drawable_size.y;
				lines.draw_text(std::to_string(score),
					glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs + 1.8f, 0.0),
					glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
					glm::u8vec4(0xff, 0xff, 0xff, 0x00));
			}
		} else {
			lines.draw_text("You win!",
					glm::vec3(-aspect + 0.1f * H + 1.0f, -1.0 + 0.1f * H + 1.0f, 0.0),
					glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
					glm::u8vec4(0x00, 0x00, 0x00, 0x00));
				float ofs = 2.0f / drawable_size.y;
				lines.draw_text("You win!",
					glm::vec3(-aspect + 0.1f * H + ofs + 1.0f, -1.0 + + 0.1f * H + ofs + 1.0f, 0.0),
					glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
					glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		}
	}
}
