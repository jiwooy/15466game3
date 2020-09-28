#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	enum dir {udir, ddir, ldir, rdir};

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;
	virtual void move_player(glm::vec2 move);
	virtual void init_maze(int setting);
	virtual void wall_bounds();
	virtual bool block_bounds(glm::vec2 move);
	virtual void obtain_generator();
	virtual void player_die();

	//----- game state -----
	Scene::Transform *player = nullptr;
	Scene::Transform *enemy = nullptr;
	Scene::Drawable *block = nullptr;
	Scene::Drawable *generator = nullptr;
	float sprint = 100.0f;
	float recovery_timer = 0.0f;
	bool penalty = false;
	bool die = false;

	std::vector<Scene::Drawable *> blocks;
	std::vector<Scene::Drawable *> generators;

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, space;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//music coming from the tip of the leg (as a demonstration):
	std::shared_ptr< Sound::PlayingSample > enemy_sound;
	std::shared_ptr< Sound::PlayingSample > tired_sound = nullptr;
	std::shared_ptr< Sound::PlayingSample > got_generator;
	std::vector<std::shared_ptr< Sound::PlayingSample >> gen_sounds;
	
	//camera:
	Scene::Camera *camera = nullptr;

};
