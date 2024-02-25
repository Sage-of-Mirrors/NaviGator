#include "application/AGatorContext.hpp"
#include "application/AInput.hpp"

#include "application/ANavContext.hpp"
#include "application/ATrackContext.hpp"

#include "ui/UViewport.hpp"

#include <bstream.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuiFileDialog.h>
#include "util/ImGuizmo.hpp"


AGatorContext::AGatorContext() : bIsDockingConfigured(false), mMainDockSpaceID(UINT32_MAX), mDockNodeTopID(UINT32_MAX),
	mDockNodeRightID(UINT32_MAX), mDockNodeDownID(UINT32_MAX), mPropertiesDockNodeID(UINT32_MAX), mAppPosition({ 0, 0 }),
	mNavContext(std::make_shared<ANavContext>()), mTrackContext(std::make_shared<ATrackContext>()), mPropertiesPanelTopID(UINT32_MAX),
	mPropertiesPanelBottomID(UINT32_MAX)
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
		mPropertiesDockNodeID = ImGui::DockBuilderSplitNode(mMainDockSpaceID, ImGuiDir_Left, 0.5f, nullptr, &mMainDockSpaceID);

		mPropertiesPanelTopID = ImGui::DockBuilderSplitNode(mPropertiesDockNodeID, ImGuiDir_Up, 0.5f, nullptr, &mPropertiesPanelBottomID);

		ImGui::DockBuilderDockWindow("Properties", mPropertiesPanelTopID);
		ImGui::DockBuilderDockWindow("Data Editor", mPropertiesPanelBottomID);
		ImGui::DockBuilderDockWindow("Main Viewport", mMainDockSpaceID);
		ImGui::DockBuilderDockWindow("gizmo", mMainDockSpaceID);

		ImGui::DockBuilderFinish(mMainDockSpaceID);

		bIsDockingConfigured = true;
	}
}

void AGatorContext::RenderMenuBar() {
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Open...")) {
				LoadFileCB();
			}
			if (ImGui::BeginMenu("Save")) {
				if (ImGui::MenuItem("Track Data")) {

				}

				ImGui::EndMenu();
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
}

void AGatorContext::Update(float deltaTime) {

}

void AGatorContext::RenderPropertiesPanel() {
	ImGuiWindowClass window_class;
	window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;

	ImGui::SetNextWindowClass(&window_class);
	ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

	mTrackContext->RenderTreeView();
	
	ImGui::End();

	ImGui::SetNextWindowClass(&window_class);
	ImGui::Begin("Data Editor", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
	
	mTrackContext->RenderDataEditor();

	ImGui::End();
}

void AGatorContext::Render(float deltaTime) {
	SetUpDocking();

	RenderMenuBar();
	RenderPropertiesPanel();

	glm::vec2 viewportSize = mMainViewport->GetViewportSize();
	glm::vec2 viewportPos = mMainViewport->GetViewportPosition() + mAppPosition;
	ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);

	mMainViewport->RenderUI(deltaTime);
	mTrackContext->RenderUI(mMainViewport->GetCamera());

	// Render open file dialog
	if (ImGuiFileDialog::Instance()->Display("loadFileDialog", 32, { 800, 600 })) {
		if (ImGuiFileDialog::Instance()->IsOk()) {
			OpenFile(ImGuiFileDialog::Instance()->GetFilePathName());
		}

		ImGuiFileDialog::Instance()->Close();
	}
}

void AGatorContext::PostRender(float deltaTime) {
	mMainViewport->BindViewport();

	mNavContext->Render(mMainViewport->GetCamera());
	mTrackContext->Render(mMainViewport->GetCamera());

	mMainViewport->UnbindViewport();
}

void AGatorContext::SetAppPosition(const int xPos, const int yPos) {
	mAppPosition = { xPos, yPos };
}

void AGatorContext::OpenFile(std::filesystem::path filePath) {
	if (!std::filesystem::exists(filePath) || !filePath.has_extension()) {
		return;
	}

	if (filePath.extension() == ".ynv") {
		mNavContext->LoadNavmesh(filePath);
	}
	else if (filePath.extension() == ".xml") {
		if (filePath.stem() == "traintracks") {
			mTrackContext->InitGLResources();
			mTrackContext->LoadTracks(filePath);
		}
	}
}

void AGatorContext::LoadFileCB() {
	ImGuiFileDialog::Instance()->OpenDialog("loadFileDialog", "Open File", "Navmeshes{.ynv},GLTF{.gltf}", ".", 1, nullptr, ImGuiFileDialogFlags_Modal);
}

void AGatorContext::OnFileDropped(std::filesystem::path filePath) {
	OpenFile(filePath);
}

void AGatorContext::OnGLInitialized() {
	mMainViewport = std::make_shared<UViewport>("Main Viewport");
	mNavContext->OnGLInitialized();
}
