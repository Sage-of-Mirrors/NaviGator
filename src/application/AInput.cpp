#include "application/AInput.hpp"

#include <GLFW/glfw3.h>


namespace AInput {
	// Anonymous namespace within UInput allows us to have private-scope variables that are only accessible from this TU.
	// This allows us to hide direct access to the input state variables and only allow access via the Get functions.
	namespace {
		constexpr uint32_t KEY_MAX = 512;
		constexpr uint32_t MOUSE_BUTTON_MAX = 3;

		glm::vec2 mMousePosition;
		glm::vec2 mMouseDelta;
		int32_t mMouseScrollDelta;

		bool mKeysDown[KEY_MAX];
		bool mPrevKeysDown[KEY_MAX];

		bool mMouseButtonsDown[MOUSE_BUTTON_MAX];
		bool mPrevMouseButtonsDown[MOUSE_BUTTON_MAX];
		glm::vec2 mPrevMousePosition;

		void SetKeyboardState(uint32_t key, bool pressed) {
			mKeysDown[key] = pressed;
		}

		void SetMouseState(uint32_t button, bool pressed) {
			mMouseButtonsDown[button] = pressed;
		}

		void SetMousePosition(uint32_t x, uint32_t y) {
			mMousePosition = glm::vec2(x, y);
		}

		void SetMouseScrollDelta(uint32_t delta) {
			mMouseScrollDelta = delta;
		}
	}
}

bool AInput::GetKey(uint32_t key) {
	return mKeysDown[key];
}

bool AInput::GetKeyDown(uint32_t key) {
	return mKeysDown[key] && !mPrevKeysDown[key];
}

bool AInput::GetKeyUp(uint32_t key) {
	return mPrevKeysDown[key] && !mKeysDown[key];
}

bool AInput::GetMouseButton(uint32_t button) {
	return mMouseButtonsDown[button];
}

bool AInput::GetMouseButtonDown(uint32_t button) {
	return mMouseButtonsDown[button] && !mPrevMouseButtonsDown[button];
}

bool AInput::GetMouseButtonUp(uint32_t button) {
	return mPrevMouseButtonsDown[button] && !mMouseButtonsDown[button];
}

glm::vec2 AInput::GetMousePosition() {
	return mMousePosition;
}

glm::vec2 AInput::GetMouseDelta() {
	return mMouseDelta;
}

int32_t AInput::GetMouseScrollDelta() {
	return mMouseScrollDelta;
}

void AInput::UpdateInputState() {
 for (int i = 0; i < KEY_MAX; i++)
	 mPrevKeysDown[i] = mKeysDown[i];
 for (int i = 0; i < MOUSE_BUTTON_MAX; i++)
	 mPrevMouseButtonsDown[i] = mMouseButtonsDown[i];

 mMouseDelta = mMousePosition - mPrevMousePosition;
 mPrevMousePosition = mMousePosition;
 mMouseScrollDelta = 0;
}

void AInput::GLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key >= KEY_MAX)
		return;

	if (action == GLFW_PRESS)
		SetKeyboardState(key, true);
	else if (action == GLFW_RELEASE)
		SetKeyboardState(key, false);
}

void AInput::GLFWMousePositionCallback(GLFWwindow* window, double xpos, double ypos) {
	SetMousePosition(uint32_t(xpos), uint32_t(ypos));
}

void AInput::GLFWMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (button >= MOUSE_BUTTON_MAX)
		return;

	if (action == GLFW_PRESS)
		SetMouseState(button, true);
	else if (action == GLFW_RELEASE)
		SetMouseState(button, false);
}

void AInput::GLFWMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	SetMouseScrollDelta(uint32_t(yoffset));
}
