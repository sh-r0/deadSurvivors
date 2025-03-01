#include <iostream>
#include <format>
#include <istream>
#include <fstream>
#include <unordered_map>
#include <chrono>
#include <bitset>
#include "gameManager.hpp"
#include "renderer.hpp"

//#include <Windows.h>

gameManager_t* gameMngPtr_g;
levelManager_t* lvlMngPtr_g;

void key_callback(GLFWwindow* _window, int32_t _key, int32_t _scancode, int32_t _action, int32_t _mods) {
	static std::bitset<4> keyStates = {};
	
	if (_action == GLFW_PRESS || _action == GLFW_RELEASE) {
		switch (_key) {
			case GLFW_KEY_A: 
			case GLFW_KEY_LEFT: {
				keyStates[0] = _action;
			} break;
			case GLFW_KEY_D: 
			case GLFW_KEY_RIGHT: {
				keyStates[1] = _action;
			} break;
			case GLFW_KEY_S:
			case GLFW_KEY_DOWN: {
				keyStates[2] = _action;
			} break;
			case GLFW_KEY_W: 
			case GLFW_KEY_UP: {
				keyStates[3] = _action;
			} break;
			case GLFW_KEY_ESCAPE: {
				if (_action == GLFW_RELEASE) {
					if (gameMngPtr_g->currLayout_ == LAYOUT_TYPE_PAUSE_GAME)
						gameMngPtr_g->currLayout_ = gameMngPtr_g->unpausedLayout_;
					else {
						gameMngPtr_g->unpausedLayout_ = gameMngPtr_g->currLayout_;
						gameMngPtr_g->currLayout_ = LAYOUT_TYPE_PAUSE_GAME;
					}
				}
			} break;
		}

		lvlMngPtr_g->playerDir_[0] = keyStates[1] - keyStates[0];
		lvlMngPtr_g->playerDir_[1] = keyStates[2] - keyStates[3];		// inverted controls for the sake of sanity
	}
	return;
}

void mButton_callback(GLFWwindow* _window, int32_t _button, int32_t _action, int32_t _mods) {
	if (_button == GLFW_MOUSE_BUTTON_LEFT && _action == GLFW_RELEASE) {
		double x, y;
		glfwGetCursorPos(_window, &x, &y);
		gameMngPtr_g->processMouseInput({ float(x),float(y) });	
	}

	return;
}

int32_t main(void) {
	renderer_t r;
	r.initRenderer();
	gameManager_t manager {};
	manager.winSizeX = r.winSizeX;
	manager.winSizeY = r.winSizeY;

	manager.initGame();
    lvlMngPtr_g = &manager.levelManager_;
	gameMngPtr_g = &manager;

	auto& currLvlManager = manager.levelManager_;
	srand(time(NULL));

	glfwSetKeyCallback(r.window, key_callback);
	glfwSetMouseButtonCallback(r.window, mButton_callback);
	
	std::chrono::steady_clock::time_point lastFrameTime = std::chrono::steady_clock::now();
	while (!glfwWindowShouldClose(r.window) && !manager.closeGame_) {
		double dt = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - lastFrameTime).count()/1000.0;
		lastFrameTime = std::chrono::steady_clock::now();
		manager.updateGameState(dt);

		const auto& _playerPos = currLvlManager.level_.player.spritePosition;
		r.updateUBO({ currLvlManager.cameraPos_[0], currLvlManager.cameraPos_[1]});
		if (manager.currLayout_ == LAYOUT_TYPE_GAME_SCREEN)
			r.pushLevel(currLvlManager);

		r.pushGui(manager);
		r.drawFrame();

		glfwPollEvents();
	}
    
    //potentially make separate threads 
	r.cleanup();
    manager.saveGameData();

    return 0;
}
