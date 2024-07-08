#include <iostream>
#include <format>
#include <istream>
#include <fstream>
#include <unordered_map>
#include <chrono>
#include <bitset>
#include "gameManager.hpp"
#include "renderer.hpp"

#include <Windows.h>

levelManager_t* mngPtr_g;

void key_callback(GLFWwindow* _window, int32_t _key, int32_t _scancode, int32_t _action, int32_t _mods) {
	static std::bitset<4> keyStates = {};
	if (_action == GLFW_PRESS || _action == GLFW_RELEASE) {
		switch (_key) {
			case GLFW_KEY_LEFT: {
				keyStates[0] = _action;
			} break;
			case GLFW_KEY_RIGHT: {
				keyStates[1] = _action;
			} break;
			case GLFW_KEY_DOWN: {
				keyStates[2] = _action;
			} break;
			case GLFW_KEY_UP: {
				keyStates[3] = _action;
			} break;
		}

		mngPtr_g->playerDir_[0] = keyStates[1] - keyStates[0];
		mngPtr_g->playerDir_[1] = keyStates[2] - keyStates[3];		// inverted controls for the sake of sanity
	}

	return;
}

int32_t main(void) {
	renderer_t r;
	r.initRenderer();
	levelManager_t manager;
	mngPtr_g = &manager;
	manager.initLevel();
	srand(time(NULL));

	//glfwSetInputMode(r.window, GLFW_LOCK_KEY_MODS, GLFW_TRUE);
	glfwSetKeyCallback(r.window, key_callback);
	
	std::chrono::steady_clock::time_point lastFrameTime = std::chrono::steady_clock::now();
	while (!glfwWindowShouldClose(r.window)) {
		double dt = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - lastFrameTime).count()/1000.0;
		lastFrameTime = std::chrono::steady_clock::now();
		manager.updateGameState(dt);

		const auto& _playerPos = manager.currLevel_.player.spritePosition;
		r.updateUBO({ manager.cameraPos_[0], manager.cameraPos_[1]});
		r.pushLevel(manager);
		r.pushGui(manager);
		r.drawFrame();

		//Sleep(10);
		//std::cout << std::format("player position: x={}; y={}\n", _playerPos[0], _playerPos[1]);
		glfwPollEvents();
	}
	
	r.cleanup();
	return 0;
}