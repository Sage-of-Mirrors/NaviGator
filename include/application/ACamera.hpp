#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>

struct GLFWwindow;

constexpr glm::vec3 ZERO = glm::vec3(0.0f, 0.0f, 0.0f);
constexpr glm::vec3 UNIT_X = glm::vec3(1.0f, 0.0f, 0.0f);
constexpr glm::vec3 UNIT_Y = glm::vec3(0.0f, 1.0f, 0.0f);
constexpr glm::vec3 UNIT_Z = glm::vec3(0.0f, 0.0f, 1.0f);

constexpr float LOOK_UP_MIN = -glm::half_pi<float>() + glm::epsilon<float>();
constexpr float LOOK_UP_MAX = glm::half_pi<float>() - glm::epsilon<float>();

class ASceneCamera {
	glm::vec3 mEye;
	glm::vec3 mCenter;

	float mPitch;
	float mYaw;
	glm::vec3 mForward;
	glm::vec3 mRight;
	glm::vec3 mUp;

	float mNearPlane;
	float mFarPlane;
	float mFovy;
	float mAspectRatio;

	float mMoveSpeed;
	float mMouseSensitivity;

	ImVec2 mLastMouseDelta;

	void Rotate(float deltaTime, float sensitivity, ImVec2 mouseDelta);

public:
	ASceneCamera();
	~ASceneCamera() {}

	void Update(float deltaTime);

	const glm::vec3& GetForwardVector() const { return mForward; }

	glm::vec3 GetPosition() { return mEye; }
	glm::vec3 GetLookat() { return mCenter; }
	glm::mat4 GetViewMatrix() { return glm::lookAt(mEye, mCenter, mUp); }
	glm::mat4 GetProjectionMatrix() { return glm::perspective(mFovy, mAspectRatio, mNearPlane, mFarPlane); }
};
