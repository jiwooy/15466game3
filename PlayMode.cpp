#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("maze.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("maze.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

Load< Sound::Sample > wind_chimes(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("wind_chimes.wav"));
});

Load< Sound::Sample > tired(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("tired.wav"));
});

Load< Sound::Sample > gen_sound(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("generator.wav"));
});

Load< Sound::Sample > got_item(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("got_item.wav"));
});

Load< Sound::Sample > hit1(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("damage1.wav"));
});

Load< Sound::Sample > hit2(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("damage2.wav"));
});

void PlayMode::init_maze(int setting) {
	uint8_t arr[10][10] {
	{0, 2, 1, 0, 1, 1, 2, 1, 1, 1},
	{0, 1, 1, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 1, 0, 1, 0, 1, 0},
	{1, 1, 0, 1, 1, 1, 1, 1, 1, 0},
	{2, 1, 0, 1, 0, 2, 1, 2, 0, 0},
	{0, 1, 0, 0, 0, 1, 1, 1, 1, 0},
	{0, 1, 0, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 1, 0, 1, 1, 1, 0, 1},
	{0, 1, 1, 1, 0, 1, 2, 1, 0, 1},
	{0, 1, 0, 0, 0, 1, 0, 0, 0, 0},
	};
	uint8_t size = 0;
	uint8_t size2 = 0;
	for (uint8_t i = 0; i < 10; i++) {
		for (uint8_t j = 0; j < 10; j++) {
			float xpos = 10.0f * j - 45.0f;
			float ypos = -10.0f * i + 45.0f;
			if (arr[i][j] == 1 && setting == 1) {
				//printf("iter %d %d %s", i, j, block->transform->name.c_str());
				size++;
				Scene::Drawable *new_block = new Scene::Drawable(*block);
				Scene::Transform *t = new Scene::Transform;
				t->rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
				t->position = glm::vec3(xpos, ypos, 6.0f);
				t->scale = block->transform->scale;
				t->name = "Block" + std::to_string(size);
				t->parent = block->transform->parent;
				scene.transforms.emplace_back(*t);
				new_block->transform = t;
				scene.drawables.emplace_back(*new_block);
				blocks.push_back(new_block);
			}
			if (arr[i][j] == 2 && setting == 2) {
				size2++;
				Scene::Drawable *new_gen = new Scene::Drawable(*generator);
				Scene::Transform *t = new Scene::Transform;
				t->rotation = generator->transform->rotation;
				t->position = glm::vec3(xpos, ypos, 3.0f);
				t->scale = generator->transform->scale;
				t->name = "Generator" + std::to_string(size);
				t->parent = generator->transform->parent;
				scene.transforms.emplace_back(*t);
				new_gen->transform = t;
				scene.drawables.emplace_back(*new_gen);
				generators.push_back(new_gen);
				std::shared_ptr< Sound::PlayingSample > gen = Sound::loop_3D(*gen_sound, 10.0f, t->position, 0.05f);
				gen_sounds.push_back(gen);
			}
		}
	}
}

PlayMode::PlayMode() : scene(*hexapod_scene) {
	//get pointers to leg for convenience:
	for (auto drawable : scene.drawables) {
		if (drawable.transform->name == "Sphere") {
			player = drawable.transform;
			player_chase = player->position;
		} else if (drawable.transform->name == "Block") {
			block = &drawable;
			init_maze(1);
		} else if (drawable.transform->name == "Generator") {
			generator = &drawable;
			init_maze(2);
		} else if (drawable.transform->name == "Cylinder") {
			enemy = drawable.transform;
			enemy->position[2] = 10.0f;
		}
	}

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	//start music loop playing:
	// (note: position will be over-ridden in update())
	enemy_sound = Sound::loop_3D(*wind_chimes, 15.0f, enemy->position, 0.05f);
	glm::quat rot = rotate(camera->transform->rotation, -3.18f ,glm::vec3(0,0,1));
	//printf("%f %f %f %f\n", camera->transform->rotation[0], camera->transform->rotation[1], camera->transform->rotation[2], camera->transform->rotation[3]);
	camera->transform->rotation = rot;
	rot = rotate(camera->transform->rotation, -3.3f ,glm::vec3(0,1,0));
	camera->transform->rotation = rot;

	camera->transform->rotation[0] = -0.002011f;
	camera->transform->rotation[1] = -0.003080f;
	camera->transform->rotation[2] = 0.001323f; 
	camera->transform->rotation[3] = 0.999992f;
	camera->transform->position = glm::vec3(player->position[0], player->position[1], 70.0f);
	
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
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.downs += 1;
			space.pressed = true;
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
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.pressed = false;
			return true;
		}
	}
	// } else if (evt.type == SDL_MOUSEBUTTONDOWN) {
	// 	if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
	// 		SDL_SetRelativeMouseMode(SDL_TRUE);
	// 		return true;
	// 	}
	// } else if (evt.type == SDL_MOUSEMOTION) {
	// 	if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
	// 		glm::vec2 motion = glm::vec2(
	// 			evt.motion.xrel / float(window_size.y),
	// 			-evt.motion.yrel / float(window_size.y)
	// 		);
	// 		camera->transform->rotation = glm::normalize(
	// 			camera->transform->rotation
	// 			* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
	// 			* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
	// 		);
	// 		return true;
	// 	}
	// }

	return false;
}

void PlayMode::update(float elapsed) {
	if (die || generators.size() == 0) {
		for (size_t i = 0; i < generators.size(); i++) {
			gen_sounds[i].get()->stop();
		}
		enemy_sound.get()->stop();
		return;
	}

	//move camera:
	{

		//combine inputs into a move:
		float PlayerSpeed = 8.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-0.5f;
		if (!left.pressed && right.pressed) move.x = 0.5f;
		if (down.pressed && !up.pressed) move.y =-0.5f;
		if (!down.pressed && up.pressed) move.y = 0.5f;
		if (space.pressed) {
			if (sprint > 0.0f) {
				PlayerSpeed *= 2.0f;
				sprint -= 0.75f;
				if (sprint <= 0.0f) {
					penalty = true;
				}
			}
		} else {
			if (!penalty) {
				sprint = std::min(100.0f, sprint + 0.5f);
			}
			else {
				recovery_timer += elapsed;
				PlayerSpeed /= 1.5f;
				if (recovery_timer >= 5.0f) {
					recovery_timer = 0.0f;
					penalty = false;
					sprint = 50.0f;
				}
			}
		}
		if (sprint <= 30.0f) {
			if (tired_sound != nullptr && tired_sound.get()->stopped) tired_sound = Sound::play(*tired, 0.7f, 0.0f);
			if (tired_sound == nullptr) tired_sound = Sound::play(*tired, 0.7f, 0.0f);
		}

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;
		
		if (block_bounds(move)) move_player(move);
		wall_bounds();
		camera->transform->position[0] = player->position[0];
		camera->transform->position[1] = player->position[1];
		obtain_generator();

		update_timer += elapsed;
		damage_timer += elapsed;
		if (update_timer >= update_interval) {
			update_interval += 1.5f;
			player_chase = player->position;
		}
		bad_ai(elapsed);

		if (damage_timer > damage_interval) {
			player_die();
		}
	}

	{ //update listener to camera position:
		glm::mat4x3 frame = player->make_local_to_parent();
		glm::vec3 right = frame[0];
		//printf("right %f %f %f\n", right[0], right[1], right[2]);
		Sound::listener.set_position_right(player->position, right, 1.0f / 60.0f);

		enemy_sound.get()->set_position(enemy->position);
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::move_player(glm::vec2 move) {
	player->position[0] += move.x;
	player->position[1] += move.y;
}

void PlayMode::wall_bounds() {
	if (player->position[0] + 1.5f > 50.0f) player->position[0] = 48.5;
	if (player->position[0] - 1.5f < -50.0f) player->position[0] = -48.5;
	if (player->position[1] + 1.5f > 50.0f) player->position[1] = 48.5;
	if (player->position[1] - 1.5f < -50.0f) player->position[1] = -48.5;
}

bool PlayMode::block_bounds(glm::vec2 move) {
	//printf("size %zd\n", blocks.size());
	for (size_t i = 0; i < blocks.size(); i++) {
		//printf("%zd %p\n", i, blocks[i]->transform);
		if (std::max(player->position[0] - 1.5f + move.x, blocks[i]->transform->position[0] - 5.0f) <= 
			std::min(player->position[0] + 1.5f + move.x, blocks[i]->transform->position[0] + 5.0f) &&
			std::max(player->position[1] - 1.5f + move.y, blocks[i]->transform->position[1] - 5.0f) <= 
			std::min(player->position[1] + 1.5f + move.y, blocks[i]->transform->position[1] + 5.0f)) {
			return false;
		}
	}
	return true;
}

void PlayMode::obtain_generator() {
	for (size_t i = 0; i < generators.size(); i++) {
		if (std::max(player->position[0] - 1.5f, generators[i]->transform->position[0] - 1.5f) <= 
			std::min(player->position[0] + 1.5f, generators[i]->transform->position[0] + 1.5f) &&
			std::max(player->position[1] - 1.5f, generators[i]->transform->position[1] - 1.5f) <= 
			std::min(player->position[1] + 1.5f, generators[i]->transform->position[1] + 1.5f)) {
			generators[i]->transform->position[2] = -10;
			generators.erase(generators.begin() + i);
			gen_sounds[i].get()->stop();
			gen_sounds.erase(gen_sounds.begin() + i);
			got_generator = Sound::play(*got_item, 0.7f, 0.0f);
		}
	}
}

void PlayMode::player_die() {
	if (std::max(player->position[0] - 1.5f, enemy->position[0] - 1.5f) <= 
		std::min(player->position[0] + 1.5f, enemy->position[0] + 1.5f) &&
		std::max(player->position[1] - 1.5f, enemy->position[1] - 1.5f) <= 
		std::min(player->position[1] + 1.5f, enemy->position[1] + 1.5f)) {
		if (health > 2) {
			damaged_sound = Sound::play(*hit1, 0.7f, 0.0f);
		} else {
			damaged_sound = Sound::play(*hit2, 0.7f, 0.0f);
		}
		health -= 1;
		damage_interval = damage_timer + 2.0f;
		if (health <= 0) die = true;
	}
}

void PlayMode::bad_ai(float elapsed) {
	float speed = 10.0f;
	glm::vec2 move = glm::vec2(0.0f);
	if (enemy->position[0] > player_chase[0]) {
		move.x -= 0.5f;
	} else if (enemy->position[0] < player_chase[0]) {
		move.x += 0.5f;
	}

	if (enemy->position[1] > player_chase[1]) {
		move.y -= 0.5f;
	} else if (enemy->position[1] < player_chase[1]) {
		move.y += 0.5f;
	}
	if (move != glm::vec2(0.0f)) move = glm::normalize(move) * speed * elapsed;
	enemy->position[0] += move.x;
	enemy->position[1] += move.y;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);
	constexpr float H = 0.09f;
	float ofs = 2.0f / drawable_size.y;

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	if (generators.size() > 0) {
		glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 2);
		glUniform3fv(lit_color_texture_program->LIGHT_LOCATION_vec3, 1,
		glm::value_ptr(glm::vec3(player->position[0], player->position[1], 20.0f)));
		glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1,
		glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
		glUniform1f(lit_color_texture_program->LIGHT_CUTOFF_float,
		std::cos(3.1415926f * 0.125f));
		glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1,
		glm::value_ptr(200.0f * glm::vec3(1.0f, 1.0f, 0.95f)));
	}
	else {
		glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
		glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
		glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	}
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

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
		if (generators.size() == 0) {
			lines.draw_text("You turned on the lights!",
				glm::vec3(-aspect / 5.5f, 0.0f, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			lines.draw_text("You turned on the lights!",
				glm::vec3(-aspect / 5.5f + ofs, ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		}

		if (!die) {
			std::string txt = "Generators found: " + std::to_string(6 - generators.size()) + "/" + std::to_string(6);
			
			lines.draw_text(txt,
				glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));	
			lines.draw_text(txt,
				glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + 0.1f * H + ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		} else {
			lines.draw_text("Game Over!",
				glm::vec3(-aspect / 5.5f, 0.0f, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			lines.draw_text("Game Over!",
				glm::vec3(-aspect / 5.5f + ofs, ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		}
	}
}