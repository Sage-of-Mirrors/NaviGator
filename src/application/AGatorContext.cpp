#include "application/AGatorContext.hpp"
#include "application/AInput.hpp"

#include "application/ANavContext.hpp"
#include "application/ATrackContext.hpp"
#include "application/ADrawableContext.hpp"

#include "ui/UViewport.hpp"
#include "ui/UViewportPicker.hpp"

#include "util/rdr1util.hpp"

#include "application/AOptions.hpp"

#include <util/bstream.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuiFileDialog.h>
#include "util/ImGuizmo.hpp"

AGatorContext::AGatorContext() : bIsDockingConfigured(false), mMainDockSpaceID(UINT32_MAX), mDockNodeTopID(UINT32_MAX),
	mDockNodeRightID(UINT32_MAX), mDockNodeDownID(UINT32_MAX), mPropertiesDockNodeID(UINT32_MAX), mAppPosition({ 0, 0 }),
	mNavContext(std::make_shared<ANavContext>()), mTrackContext(std::make_shared<ATrackContext>()), mDrawableContext(std::make_shared<ADrawableContext>()),
	mPropertiesPanelTopID(UINT32_MAX), mPropertiesPanelBottomID(UINT32_MAX)
{
	OPTIONS.Load();
}

AGatorContext::~AGatorContext() {
	UViewportPicker::DestroyPicker();
	OPTIONS.Save();
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
			if (ImGui::BeginMenu("Railroad Data")) {
				if (!mTrackContext->IsLoaded()) {
					ImGui::BeginDisabled();
				}

				if (ImGui::MenuItem("Save")) {
					if (OPTIONS.mLastSavedRailroadDir.empty()) {
						SaveTracksAsCB();
					}
					else {
						mTrackContext->SaveTracks(OPTIONS.mLastSavedRailroadDir);
					}
				}
				if (ImGui::MenuItem("Save as...")) {
					SaveTracksAsCB();
				}

				if (!mTrackContext->IsLoaded()) {
					ImGui::EndDisabled();
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
	glm::vec2 viewportSize = mMainViewport->GetViewportSize();
	UViewportPicker::ResizePicker(uint32_t(viewportSize.x), uint32_t(viewportSize.y));

	glm::vec2 screenMousePos = mAppPosition + AInput::GetMousePosition();
	glm::vec2 bufferMousePos = screenMousePos - mMainViewport->GetViewportPosition();
	bufferMousePos.y = mMainViewport->GetViewportSize().y - bufferMousePos.y;

	if (bufferMousePos.x >= 0 && bufferMousePos.y >= 0) {
		mTrackContext->OnMouseHover(mMainViewport->GetCamera(), int32_t(bufferMousePos.x), int32_t(bufferMousePos.y));

		if (AInput::GetMouseButton(0)) {
			mTrackContext->OnMouseClick(mMainViewport->GetCamera(), int32_t(bufferMousePos.x), int32_t(bufferMousePos.y));
		}
	}
}

void AGatorContext::RenderPropertiesPanel() {
	ImGuiWindowClass window_class;
	window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;

	// Properties panel
	ImGui::SetNextWindowClass(&window_class);
	ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

	mTrackContext->RenderTreeView();
	
	ImGui::End();

	// Data editor panel
	ImGui::SetNextWindowClass(&window_class);
	ImGui::Begin("Data Editor", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
	
	mTrackContext->RenderDataEditor();

	ImGui::End();
}

void AGatorContext::Render(float deltaTime) {
	ZoneScoped;

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

	if (ImGuiFileDialog::Instance()->Display("saveTracksAsDialog", 32, {800, 600})) {
		if (ImGuiFileDialog::Instance()->IsOk()) {
			mTrackContext->SaveTracks(ImGuiFileDialog::Instance()->GetFilePathName());
			OPTIONS.mLastSavedRailroadDir = ImGuiFileDialog::Instance()->GetFilePathName();
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
		OPTIONS.mLastOpenedDir = filePath;
	}
	else if (filePath.extension() == ".ydr") {
		mDrawableContext->LoadDrawable(filePath);
	}
	else if (filePath.extension() == ".xml") {
		if (filePath.stem() == "traintracks") {
			mTrackContext->InitGLResources();
			mTrackContext->LoadTracks(filePath);

			OPTIONS.mLastOpenedDir = filePath;
			OPTIONS.mLastOpenedRailroadDir = filePath.parent_path();
		}
	}
	else if (filePath.filename() == "swrailroad.wsi") {
		RDR1Util::ExtractTrainPoints(filePath);
	}
}

void AGatorContext::LoadFileCB() {
	std::string startingDir = OPTIONS.mLastOpenedDir.empty() ? "." : OPTIONS.mLastOpenedDir.u8string();
	ImGuiFileDialog::Instance()->OpenDialog("loadFileDialog", "Open File", "traintracks.xml{.xml},Navmeshes (*.ynv){.ynv}", startingDir, 1, nullptr, ImGuiFileDialogFlags_Modal);
}

void AGatorContext::SaveTracksAsCB() {
	std::string startingDir = OPTIONS.mLastSavedRailroadDir.empty() ? "." : OPTIONS.mLastSavedRailroadDir.u8string();
	ImGuiFileDialog::Instance()->OpenDialog("saveTracksAsDialog", "Choose Directory", nullptr, startingDir, 1, nullptr, ImGuiFileDialogFlags_Modal);
}

void AGatorContext::OnFileDropped(std::filesystem::path filePath) {
	OpenFile(filePath);
}

void AGatorContext::OnGLInitialized() {
	mMainViewport = std::make_shared<UViewport>("Main Viewport");
	mNavContext->OnGLInitialized();

	glm::vec2 viewportSize = mMainViewport->GetViewportSize();
	UViewportPicker::CreatePicker(uint32_t(viewportSize.x), uint32_t(viewportSize.y));
}
