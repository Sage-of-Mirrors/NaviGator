#include "application/AGatorContext.hpp"

#include "application/AInput.hpp"

#include <bstream.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuiFileDialog.h>


AGatorContext::AGatorContext() : bIsDockingConfigured(false), mMainDockSpaceID(UINT32_MAX), mDockNodeTopID(UINT32_MAX),
	mDockNodeRightID(UINT32_MAX), mDockNodeDownID(UINT32_MAX), mDockNodeLeftID(UINT32_MAX), mAppPosition({ 0, 0 })
{

}

AGatorContext::~AGatorContext() {

}

void AGatorContext::SetUpDocking() {
	const ImGuiViewport* mainViewport = ImGui::GetMainViewport();

	ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_PassthruCentralNode;
	mMainDockSpaceID = ImGui::DockSpaceOverViewport(mainViewport, dockFlags);

	if (!bIsDockingConfigured) {
		ImGui::DockBuilderRemoveNode(mMainDockSpaceID); // clear any previous layout
		ImGui::DockBuilderAddNode(mMainDockSpaceID, dockFlags | ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(mMainDockSpaceID, mainViewport->Size);

		mDockNodeTopID = ImGui::DockBuilderSplitNode(mMainDockSpaceID, ImGuiDir_Up, 0.5f, nullptr, &mMainDockSpaceID);
		mDockNodeRightID = ImGui::DockBuilderSplitNode(mMainDockSpaceID, ImGuiDir_Right, 0.5f, nullptr, &mMainDockSpaceID);
		mDockNodeDownID = ImGui::DockBuilderSplitNode(mMainDockSpaceID, ImGuiDir_Down, 0.5f, nullptr, &mMainDockSpaceID);
		mDockNodeLeftID = ImGui::DockBuilderSplitNode(mMainDockSpaceID, ImGuiDir_Left, 0.5f, nullptr, &mMainDockSpaceID);

		ImGui::DockBuilderDockWindow("Main Viewport", mMainDockSpaceID);

		ImGui::DockBuilderFinish(mMainDockSpaceID);

		bIsDockingConfigured = true;
	}
}

void AGatorContext::RenderMenuBar() {
	ImGui::BeginMainMenuBar();

	if (ImGui::BeginMenu("File")) {
		if (ImGui::MenuItem("Open...")) {			
		}
		if (ImGui::MenuItem("Save...")) {
		}

		ImGui::Separator();
		ImGui::MenuItem("Close");

		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("About")) {
		ImGui::EndMenu();
	}

	ImGui::EndMainMenuBar();
}

void AGatorContext::Update(float deltaTime) {

}

void AGatorContext::Render(float deltaTime) {
	SetUpDocking();
	RenderMenuBar();
}

void AGatorContext::PostRender(float deltaTime) {

}

void AGatorContext::SetAppPosition(const int xPos, const int yPos) {
	mAppPosition = { xPos, yPos };
}

void AGatorContext::OnGLInitialized() {

}

void AGatorContext::OnFileDropped(std::filesystem::path filePath) {
	if (!filePath.has_extension()) {
		return;
	}
}
