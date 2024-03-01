#pragma once

#include "types.h"

#include <imgui.h>

struct GLFWwindow;

constexpr glm::vec3 ZERO = glm::vec3(0.0f, 0.0f, 0.0f);
constexpr glm::vec3 UNIT_X = glm::vec3(1.0f, 0.0f, 0.0f);
constexpr glm::vec3 UNIT_Y = glm::vec3(0.0f, 1.0f, 0.0f);
constexpr glm::vec3 UNIT_Z = glm::vec3(0.0f, 0.0f, 1.0f);

constexpr float LOOK_UP_MIN = -glm::half_pi<float>() + glm::epsilon<float>();
constexpr float LOOK_UP_MAX = glm::half_pi<float>() - glm::epsilon<float>();

enum {
	CAM_VIEW_PROJ,
	CAM_VIEW_ORTHO
};

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

	float mScreenWidth;
	float mScreenHeight;
	float mZoomLevel;

	ImVec2 mLastMouseDelta;

	uint8_t mViewMode = CAM_VIEW_PROJ;

	void ProcessInputProjection(float deltaTime);
	void ProcessInputOrthographic(float deltaTime);
	void Rotate(float deltaTime, float sensitivity, ImVec2 mouseDelta);

public:
	ASceneCamera();
	~ASceneCamera() {}

	void Update(float deltaTime, float screenWidth, float screenHeight);

	const glm::vec3& GetForwardVector() const { return mForward; }

	glm::vec3 GetPosition() { return mEye; }
	glm::vec3 GetLookat() { return mCenter; }
	uint8_t GetViewMode() const { return mViewMode; }

	glm::mat4 GetViewMatrix();
	glm::mat4 GetProjectionMatrix();

	void SetViewMode(uint8_t mode);
	void SetView(glm::vec3 eye, glm::vec3 center, glm::vec3 up);
	void SetViewportSize(float width, float height) { mScreenWidth = width; mScreenHeight = height; }
};
