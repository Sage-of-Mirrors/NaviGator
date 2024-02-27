#include "application/ACamera.hpp"
#include "application/AInput.hpp"

#include <imgui.h>

#include <algorithm>


ASceneCamera::ASceneCamera() : mNearPlane(1.0f), mFarPlane(1000000.f), mFovy(glm::radians(60.f)),
    mCenter(ZERO), mEye(ZERO), mPitch(0.f), mYaw(glm::half_pi<float>()), mUp(UNIT_Y), mRight(-UNIT_X), mForward(UNIT_Z),
    mAspectRatio(16.f / 9.f), mMoveSpeed(1000.f), mMouseSensitivity(0.25f)
{
	mCenter = mEye - mForward;
}

void ASceneCamera::Update(float deltaTime, float screenWidth, float screenHeight) {
	mScreenWidth = screenWidth;
	mScreenHeight = screenHeight;

	glm::vec3 moveDir = glm::zero<glm::vec3>();

	if (ImGui::IsKeyDown(ImGuiKey_W)) // Forward
		moveDir -= mForward;
	if (ImGui::IsKeyDown(ImGuiKey_S)) // Backward
		moveDir += mForward;
	if (ImGui::IsKeyDown(ImGuiKey_D)) // Right
		moveDir -= mRight;
	if (ImGui::IsKeyDown(ImGuiKey_A)) // Left
		moveDir += mRight;

	if (ImGui::IsKeyDown(ImGuiKey_Q)) // Up
		moveDir -= UNIT_Y;
	if (ImGui::IsKeyDown(ImGuiKey_E)) // Down
		moveDir += UNIT_Y;

	mMoveSpeed += ImGui::GetIO().MouseWheel * 10000 * deltaTime;
	mMoveSpeed = std::clamp(mMoveSpeed, 100.f, 50000.f);
	float actualMoveSpeed = ImGui::IsKeyDown(ImGuiKey_LeftShift) ? mMoveSpeed * 10.f : mMoveSpeed;

	// Step-wise rotation
	if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
		Rotate(deltaTime, 0.25f, ImGui::GetMouseDragDelta(ImGuiMouseButton_Right));
		ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
	}
	// Smooth rotation
	else if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
		Rotate(deltaTime, 0.05f, ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle));
	}

	if (glm::length(moveDir) != 0.f)
		moveDir = glm::normalize(moveDir);

	mEye += moveDir * (actualMoveSpeed * deltaTime);
	mCenter = mEye - mForward;
}

void ASceneCamera::Rotate(float deltaTime, float sensitivity, ImVec2 mouseDelta) {
	if (mouseDelta.x == 0.f && mouseDelta.y == 0.f)
		return;

	mPitch += mouseDelta.y * deltaTime * sensitivity;
	mYaw += mouseDelta.x * deltaTime * sensitivity;

	mPitch = std::clamp(mPitch, LOOK_UP_MIN, LOOK_UP_MAX);

	mForward.x = cos(mYaw) * cos(mPitch);
	mForward.y = sin(mPitch);
	mForward.z = sin(mYaw) * cos(mPitch);

	mForward = glm::normalize(mForward);

	mRight = glm::normalize(glm::cross(mForward, UNIT_Y));
	mUp = glm::normalize(glm::cross(mRight, mForward));
}

glm::mat4 ASceneCamera::GetViewMatrix() {
	return glm::lookAt(mEye, mCenter, mUp);
}

glm::mat4 ASceneCamera::GetProjectionMatrix() {
	switch (mViewMode) {
		case CAM_VIEW_PROJ:
			return glm::perspective(mFovy, mAspectRatio, mNearPlane, mFarPlane);
		case CAM_VIEW_ORTHO:
			return glm::ortho(0.0f, mScreenWidth, 0.0f, mScreenHeight, -1000.0f, 1000.0f);
		default:
			return glm::identity<glm::mat4>();
	}
}

void ASceneCamera::SetViewMode(uint8_t mode) {
	mViewMode = mode;
}