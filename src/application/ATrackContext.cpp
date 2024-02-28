#include "application/ATrackContext.hpp"
#include "tracks/UTrack.hpp"
#include "tracks/UTrackPoint.hpp"
#include "ubo/common.hpp"
#include "util/fileutil.hpp"
#include "util/uiutil.hpp"
#include "ui/UViewportPicker.hpp"
#include "application/AInput.hpp"

#include "primitives/USphere.hpp"

#include <pugixml.hpp>
#include <glad/glad.h>
#include <imgui.h>
#include "util/ImGuizmo.hpp"

#include <iostream>
#include <algorithm>
#include <format>
#include <limits>

constexpr const char* TRACKS_CHILD_NAME = "train_tracks";
constexpr const char* TRACKS_FILE_NAME = "traintracks.xml";
constexpr const char* NEW_TRACK_DIALOG_LABEL = "New Track";

constexpr uint32_t VERTEX_ATTRIB_INDEX = 0;

constexpr glm::vec4 NORMAL_COLOR = { 0.00f, 0.25f, 0.75f, 1.0f };
constexpr glm::vec4 HIGHLIGHT_COLOR = { 1.0f, 0.5f, 0.0f, 1.0f };
constexpr glm::vec4 SELECTED_COLOR = { 1.0f, 0.1f, 0.2f, 1.0f };
constexpr glm::vec4 HANDLE_COLOR = { 1.0f, 0.5f, 1.0f, 1.0f };

constexpr uint32_t HANDLE_A_MASK = 0x40000000;
constexpr uint32_t HANDLE_B_MASK = 0x80000000;

ATrackContext::ATrackContext() : mPntVBO(0), mPntIBO(0), mPntVAO(0), mSimpleProgram(0), bGLInitialized(false), mBaseColorUniform(0),
    mSelectedTrack(), mSelectedPickType(ETrackNodePickType::Position), bSelectingJunctionPartner(false), mPendingNewTrackName(""),
    bTrackDialogOpen(false), bCanDuplicatePoint(true)
{

}

ATrackContext::~ATrackContext() {
    DestroyGLResources();
}

void ATrackContext::InitGLResources() {
    if (bGLInitialized) {
        return;
    }

    glCreateBuffers(1, &mPntVBO);
    glCreateBuffers(1, &mPntIBO);

    glNamedBufferStorage(mPntVBO, USphere::VertexCount * sizeof(glm::vec3), USphere::Vertices, GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferStorage(mPntIBO, USphere::IndexCount  * sizeof(uint32_t),  USphere::Indices, GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);

    glCreateVertexArrays(1, &mPntVAO);
    glVertexArrayVertexBuffer(mPntVAO, 0, mPntVBO, 0, sizeof(glm::vec3));
    glVertexArrayElementBuffer(mPntVAO, mPntIBO);

    glEnableVertexArrayAttrib(mPntVAO, VERTEX_ATTRIB_INDEX);
    glVertexArrayAttribBinding(mPntVAO, VERTEX_ATTRIB_INDEX, 0);
    glVertexArrayAttribFormat(mPntVAO, VERTEX_ATTRIB_INDEX, glm::vec3::length(), GL_FLOAT, GL_FALSE, 0);

    InitSimpleShader();

    bGLInitialized = true;
}

void ATrackContext::InitSimpleShader() {
    // Compile vertex shader
    std::string vertTxt = UFileUtil::LoadShaderText("simple.vert");
    const char* vertTxtChars = vertTxt.data();

    uint32_t vertHandle = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertHandle, 1, &vertTxtChars, NULL);
    glCompileShader(vertHandle);

    int32_t success = 0;
    glGetShaderiv(vertHandle, GL_COMPILE_STATUS, &success);
    if (!success) {
        int32_t logSize = 0;
        glGetShaderiv(vertHandle, GL_INFO_LOG_LENGTH, &logSize);

        std::vector<char> log(logSize);
        glGetShaderInfoLog(vertHandle, logSize, nullptr, &log[0]);

        std::cout << std::string(log.data()) << std::endl;

        glDeleteShader(vertHandle);
    }

    // Compile fragment shader
    std::string fragTxt = UFileUtil::LoadShaderText("simple.frag");
    const char* fragTxtChars = fragTxt.data();

    uint32_t fragHandle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragHandle, 1, &fragTxtChars, NULL);
    glCompileShader(fragHandle);

    success = 0;
    glGetShaderiv(fragHandle, GL_COMPILE_STATUS, &success);
    if (!success) {
        int32_t logSize = 0;
        glGetShaderiv(fragHandle, GL_INFO_LOG_LENGTH, &logSize);

        std::vector<char> log(logSize);
        glGetShaderInfoLog(fragHandle, logSize, nullptr, &log[0]);

        std::cout << std::string(log.data()) << std::endl;

        glDeleteShader(fragHandle);
    }

    // Generate shader program
    mSimpleProgram = glCreateProgram();
    glAttachShader(mSimpleProgram, vertHandle);
    glAttachShader(mSimpleProgram, fragHandle);
    glLinkProgram(mSimpleProgram);

    // Clean up
    glDetachShader(mSimpleProgram, vertHandle);
    glDetachShader(mSimpleProgram, fragHandle);
    glDeleteShader(vertHandle);
    glDeleteShader(fragHandle);

    UCommonUniformBuffer::LinkShaderToUBO(mSimpleProgram);
    mBaseColorUniform = glGetUniformLocation(mSimpleProgram, "uBaseColor");
}

void ATrackContext::DestroyGLResources() {
    uint32_t buffers[]{ mPntVBO, mPntIBO };

    glDeleteBuffers(2, buffers);
    glDeleteVertexArrays(1, &mPntVAO);
    glDeleteProgram(mSimpleProgram);

    mPntVBO = 0;
    mPntIBO = 0;
    mPntVAO = 0;
    mSimpleProgram = 0;

    bGLInitialized = false;
}

void ATrackContext::LoadTracks(std::filesystem::path filePath) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(filePath.c_str());

    if (!result) {
        return;
    }

    std::filesystem::path configDir = filePath.parent_path();

    for (pugi::xml_node trackNode : doc.child(TRACKS_CHILD_NAME)) {
        std::shared_ptr<UTracks::UTrack> track = std::make_shared<UTracks::UTrack>();
        track->Deserialize(trackNode);
        mTracks.push_back(track);
    }

    std::sort(mTracks.begin(), mTracks.end(),
        [](std::shared_ptr<UTracks::UTrack> a, std::shared_ptr<UTracks::UTrack> b) {
            return a->GetConfigName() < b->GetConfigName();
        }
    );

    for (std::shared_ptr<UTracks::UTrack> track : mTracks) {
        mTrackPoints.push_back(track->LoadNodePoints(configDir));
    }

    PostprocessNodes();
}

void ATrackContext::PostprocessNodes() {
    for (shared_vector<UTracks::UTrackPoint> trackPoints : mTrackPoints) {
        for (const std::shared_ptr<UTracks::UTrackPoint> pnt : trackPoints) {
            if (!pnt->IsJunction() || !pnt->GetJunctionPartner().expired()) {
                continue;
            }

            for (uint32_t trackIdx = 0; trackIdx < mTracks.size(); trackIdx++) {
                if (mTracks[trackIdx]->GetConfigName() != pnt->GetArgument()) {
                    continue;
                }

                glm::vec3 curPntPos = pnt->GetPosition();

                for (std::shared_ptr<UTracks::UTrackPoint> otherPnt : mTrackPoints[trackIdx]) {
                    float dist = glm::distance(curPntPos, otherPnt->GetPosition());

                    if (dist <= 0.0001f) {
                        pnt->SetJunctionPartner(otherPnt);
                        otherPnt->SetJunctionPartner(pnt);
                    }
                }

                break;
            }
        }
    }
}

void ATrackContext::SaveTracks(std::filesystem::path dirPath) {
    std::filesystem::path fullConfigPath = dirPath / TRACKS_FILE_NAME;
    pugi::xml_document doc;
    doc.document_element().append_attribute("encoding").set_value("UTF-8");

    pugi::xml_node rootNode = doc.append_child(TRACKS_CHILD_NAME);
    rootNode.append_attribute("version").set_value("1");

    for (std::shared_ptr<UTracks::UTrack> track : mTracks) {
        pugi::xml_node trackNode = rootNode.append_child("train_track");
        track->Serialize(trackNode);
    }

    doc.save_file(fullConfigPath.c_str(), PUGIXML_TEXT("\t"), pugi::format_indent | pugi::format_indent_attributes | pugi::format_save_file_text, pugi::encoding_utf8);

    for (uint32_t trackIdx = 0; trackIdx < mTrackPoints.size(); trackIdx++) {
        mTracks[trackIdx]->SaveNodePoints(dirPath, mTrackPoints[trackIdx]);
    }
}

void ATrackContext::RenderTreeView() {
    if (mTracks.size() == 0) {
        ImGui::Text("Please load traintracks.xml.");
        return;
    }

    bool treeNodeOpen = ImGui::TreeNodeEx("Track Configs", ImGuiTreeNodeFlags_DefaultOpen);
    ImGui::PushID("Track Configs");
    
    if (ImGui::BeginPopupContextItem("Context Menu"))
    {
        if (ImGui::MenuItem("Add track...")) {
            bTrackDialogOpen = true;
        }

        ImGui::EndPopup();
    }
    
    ImGui::PopID();

    if (bTrackDialogOpen) {
        ImGui::OpenPopup(NEW_TRACK_DIALOG_LABEL);
        bTrackDialogOpen = false;
    }

    RenderNewTrackDialog();

    if (treeNodeOpen) {
        ImGui::Indent();

        for (uint32_t i = 0; i < mTracks.size(); i++) {
            std::shared_ptr<UTracks::UTrack> track = mTracks[i];

            bool isSelected = !mSelectedTrack.expired() && mSelectedTrack.lock() == track;
            std::string imguiId = track->GetConfigName(); // std::format("{}##{}", track->GetConfigName(), i);
            
            if (ImGui::Selectable(imguiId.c_str(), isSelected)) {
                mSelectedTrack = track;
            }
        }

        ImGui::TreePop();
    }
}

void ATrackContext::RenderDataEditor() {
    if (!mSelectedTrack.expired()) {
        RenderTrackDataEditor(mSelectedTrack.lock());
        ImGui::Spacing();
    }

    if (mSelectedPoints.size() == 1) {
        RenderPointDataEditorSingle(mSelectedPoints[0].lock());
    }
    else if (mSelectedPoints.size() > 1) {
        RenderPointDataEditorMulti();
    }
}

void ATrackContext::RenderTrackDataEditor(std::shared_ptr<UTracks::UTrack> track) {
    if (ImGui::CollapsingHeader("Selected Track Data", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        ImGui::Spacing();
        UIUtil::RenderTextInput("Name", track->GetConfigNameForEditor(), 0);

        ImGui::Spacing();
        ImGui::InputScalar("Braking Distance", ImGuiDataType_U32, track->GetBrakingDistForEditor());

        ImGui::Spacing();
        ImGui::Checkbox("Loops?", track->GetLoopsForEditor());

        ImGui::Spacing();
        ImGui::Checkbox("Stops at stations?", track->GetStopsAtStationsForEditor());

        ImGui::Unindent();
    }
}

void ATrackContext::RenderPointDataEditorSingle(std::shared_ptr<UTracks::UTrackPoint> point) {
    if (ImGui::CollapsingHeader("Selected Node Data", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        ImGui::Spacing();
        UIUtil::RenderComboEnum<UTracks::ENodeStationType>("Station Type", point->GetStationTypeForEditor());

        ImGui::Spacing();

        // Only nodes with station type None can be junctions, so show UI for one or the other.
        if (point->GetStationType() == UTracks::ENodeStationType::None) {
            if (!point->HasJunctionPartner()) {
                if (ImGui::Button("Dropper")) {
                    bSelectingJunctionPartner = true;
                }
            }
            else {
                ImGui::Text("Junction between:");
                ImGui::Text("%s", point->GetParentTrackName().data());
                ImGui::Text("and");
                ImGui::Text("%s", point->GetJunctionPartner().lock()->GetParentTrackName().data());

                if (ImGui::Button("Clear Junction")) {
                    point->SetJunctionPartner(nullptr);
                }
            }
        }
        else {
            UIUtil::RenderTextInput("Station Name", point->GetArgumentForEditor(), 0);
        }

        ImGui::Spacing();
        ImGui::InputFloat("Unknown Float", point->GetScalarForEditor());

        ImGui::Spacing();
        ImGui::Checkbox("Is in a tunnel?", point->GetIsTunnelForEditor());

        ImGui::Unindent();
    }
}

void ATrackContext::RenderPointDataEditorMulti() {
    if (ImGui::CollapsingHeader("Selected Node Data", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        ImGui::Text("Editing of multiple track nodes at once\nis currently not supported.");

        ImGui::Unindent();
    }
}

void ATrackContext::RenderNewTrackDialog() {
    bool open = true;
    ImGui::SetNextWindowSize({ 300, 0 });
    if (ImGui::BeginPopupModal(NEW_TRACK_DIALOG_LABEL, &open)) {
        ImGui::Text("New track name:");
        UIUtil::RenderTextInput("##pendingTrackName", &mPendingNewTrackName, 0);

        if (mPendingNewTrackName.empty()) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("OK", { 100, 0 })) {
            std::shared_ptr<UTracks::UTrack> newTrack = std::make_shared<UTracks::UTrack>(mPendingNewTrackName);
            mTracks.push_back(newTrack);

            shared_vector<UTracks::UTrackPoint> newTrackPoints;
            std::shared_ptr<UTracks::UTrackPoint> newPoint = std::make_shared<UTracks::UTrackPoint>(newTrack->GetConfigName());
            newTrackPoints.push_back(newPoint);

            mTrackPoints.push_back(newTrackPoints);
            ImGui::CloseCurrentPopup();
        }
        if (mPendingNewTrackName.empty()) {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", { 100, 0 })) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void ATrackContext::RenderUI(ASceneCamera& camera) {
    if (mSelectedPoints.size() == 0) {
        return;
    }

    if (!ImGuizmo::IsUsing()) {
        bCanDuplicatePoint = true;
    }

    glm::mat4 projMtx = camera.GetProjectionMatrix();
    glm::mat4 viewMtx = camera.GetViewMatrix();

    std::shared_ptr<UTracks::UTrackPoint> lockedPt = mSelectedPoints[0].lock();

    switch (mSelectedPickType) {
        case ETrackNodePickType::Position:
        {
            glm::mat4 modelMtx = glm::translate(glm::identity<glm::mat4>(), lockedPt->GetPosition());

            if (ImGuizmo::Manipulate(&viewMtx[0][0], &projMtx[0][0], ImGuizmo::OPERATION::TRANSLATE, ImGuizmo::WORLD, &modelMtx[0][0])) {
                if (ImGui::GetIO().KeyCtrl && bCanDuplicatePoint) {
                    bCanDuplicatePoint = false;

                    std::shared_ptr<UTracks::UTrackPoint> newPt = std::make_shared<UTracks::UTrackPoint>(*lockedPt);
                    for (uint32_t i = 0; i < mTracks.size(); i++) {
                        if (mTracks[i]->GetConfigName() == newPt->GetParentTrackName()) {
                            mTrackPoints[i].push_back(newPt);
                            break;
                        }
                    }

                    ClearSelectedPoints();
                    mSelectedPoints.push_back(newPt);
                    newPt->SetSelected(true);

                    lockedPt = newPt;
                }

                glm::vec3 diff = glm::vec3(modelMtx[3]) - lockedPt->GetPosition();
                lockedPt->GetPositionForEditor() = glm::vec3(modelMtx[3]);

                if (lockedPt->IsCurve()) {
                    lockedPt->GetHandleAForEditor() += diff;
                    lockedPt->GetHandleBForEditor() += diff;
                }

                if (lockedPt->HasJunctionPartner()) {
                    std::shared_ptr<UTracks::UTrackPoint> partner = lockedPt->GetJunctionPartner().lock();
                    partner->GetPositionForEditor() = glm::vec3(modelMtx[3]);

                    if (partner->IsCurve()) {
                        partner->GetHandleAForEditor() += diff;
                        partner->GetHandleBForEditor() += diff;
                    }
                }
            }

            break;
        }
        case ETrackNodePickType::Handle_A:
        {
            glm::mat4 modelMtx = glm::translate(glm::identity<glm::mat4>(), lockedPt->GetHandleA());

            if (ImGuizmo::Manipulate(&viewMtx[0][0], &projMtx[0][0], ImGuizmo::OPERATION::TRANSLATE, ImGuizmo::WORLD, &modelMtx[0][0])) {
                lockedPt->GetHandleAForEditor() = glm::vec3(modelMtx[3]);

                if (lockedPt->HasJunctionPartner()) {
                    std::shared_ptr<UTracks::UTrackPoint> partner = lockedPt->GetJunctionPartner().lock();
                    partner->GetHandleAForEditor() = glm::vec3(modelMtx[3]);
                }
            }

            break;
        }
        case ETrackNodePickType::Handle_B:
        {
            glm::mat4 modelMtx = glm::translate(glm::identity<glm::mat4>(), lockedPt->GetHandleB());

            if (ImGuizmo::Manipulate(&viewMtx[0][0], &projMtx[0][0], ImGuizmo::OPERATION::TRANSLATE, ImGuizmo::WORLD, &modelMtx[0][0])) {
                lockedPt->GetHandleBForEditor() = glm::vec3(modelMtx[3]);

                if (lockedPt->HasJunctionPartner()) {
                    std::shared_ptr<UTracks::UTrackPoint> partner = lockedPt->GetJunctionPartner().lock();
                    partner->GetHandleBForEditor() = glm::vec3(modelMtx[3]);
                }
            }

            break;
        }
    }
}

void ATrackContext::Render(ASceneCamera& camera) {
    if (mTracks.size() == 0) {
        return;
    }

    UCommonUniformBuffer::SetProjAndViewMatrices(camera.GetProjectionMatrix(), camera.GetViewMatrix());
    glBindVertexArray(mPntVAO);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glUseProgram(mSimpleProgram);

    for (shared_vector<UTracks::UTrackPoint> trackPoints : mTrackPoints) {
        for (const std::shared_ptr<UTracks::UTrackPoint> pnt : trackPoints) {
            if (pnt->IsSelected()) {
                glUniform4fv(mBaseColorUniform, 1, &SELECTED_COLOR.x);
            }
            else if (pnt->IsHighlighted()) {
                glUniform4fv(mBaseColorUniform, 1, &HIGHLIGHT_COLOR.x);
            }
            else {
                glUniform4fv(mBaseColorUniform, 1, &NORMAL_COLOR.x);
            }

            UCommonUniformBuffer::SetModelMatrix(glm::translate(glm::identity<glm::mat4>(), pnt->GetPosition()));
            UCommonUniformBuffer::SubmitUBO();

            glDrawElements(GL_TRIANGLES, USphere::IndexCount, GL_UNSIGNED_INT, 0);

            pnt->SetHighlighted(false);

            // Draw handles
            if (pnt->IsCurve() && pnt->IsSelected()) {
                glUniform4fv(mBaseColorUniform, 1, &HANDLE_COLOR.r);
                UCommonUniformBuffer::SetModelMatrix(glm::translate(glm::identity<glm::mat4>(), pnt->GetHandleA()));
                UCommonUniformBuffer::SubmitUBO();

                glDrawElements(GL_TRIANGLES, USphere::IndexCount, GL_UNSIGNED_INT, 0);

                UCommonUniformBuffer::SetModelMatrix(glm::translate(glm::identity<glm::mat4>(), pnt->GetHandleB()));
                UCommonUniformBuffer::SubmitUBO();

                glDrawElements(GL_TRIANGLES, USphere::IndexCount, GL_UNSIGNED_INT, 0);
            }
        }
    }

    glUseProgram(0);
    glBindVertexArray(0);
}

void ATrackContext::RenderPickingBuffer(ASceneCamera& camera) {
    UViewportPicker::BindBuffer();

    UCommonUniformBuffer::SetProjAndViewMatrices(camera.GetProjectionMatrix(), camera.GetViewMatrix());
    glBindVertexArray(mPntVAO);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    for (uint32_t trackIdx = 0; trackIdx < mTrackPoints.size(); trackIdx++) {
        for (uint32_t pointIdx = 0; pointIdx < mTrackPoints[trackIdx].size(); pointIdx++) {
            UCommonUniformBuffer::SetModelMatrix(glm::translate(glm::identity<glm::mat4>(), mTrackPoints[trackIdx][pointIdx]->GetPosition()));
            UCommonUniformBuffer::SubmitUBO();

            uint32_t trackId = ((trackIdx + 1) << 16) & 0x3FFF0000;
            uint32_t pointId = pointIdx + 1;
            UViewportPicker::SetIdUniform(trackId | pointId);

            glDrawElements(GL_TRIANGLES, USphere::IndexCount, GL_UNSIGNED_INT, 0);

            if (mTrackPoints[trackIdx][pointIdx]->IsSelected()) {
                UCommonUniformBuffer::SetModelMatrix(glm::translate(glm::identity<glm::mat4>(), mTrackPoints[trackIdx][pointIdx]->GetHandleA()));
                UCommonUniformBuffer::SubmitUBO();

                UViewportPicker::SetIdUniform(HANDLE_A_MASK | trackId | pointId);
                glDrawElements(GL_TRIANGLES, USphere::IndexCount, GL_UNSIGNED_INT, 0);

                UCommonUniformBuffer::SetModelMatrix(glm::translate(glm::identity<glm::mat4>(), mTrackPoints[trackIdx][pointIdx]->GetHandleB()));
                UCommonUniformBuffer::SubmitUBO();

                UViewportPicker::SetIdUniform(HANDLE_B_MASK | trackId | pointId);
                glDrawElements(GL_TRIANGLES, USphere::IndexCount, GL_UNSIGNED_INT, 0);
            }
        }
    }

    UViewportPicker::UnbindBuffer();
}

void ATrackContext::OnMouseHover(ASceneCamera& camera, int32_t pX, int32_t pY) {
    if (mTracks.size() == 0 || ImGuizmo::IsUsing()) {
        return;
    }

    RenderPickingBuffer(camera);

    uint32_t result = UViewportPicker::Query(pX, pY);
    if (result == 0) {
        return;
    }

    //mSelectedPickType = ETrackNodePickType((result & 0xC0000000) >> 30);
    uint16_t trackIdx = ((result & 0x3FFF0000) >> 16) - 1;
    uint16_t pointIdx = (result & 0xFFFF) - 1;

    mTrackPoints[trackIdx][pointIdx]->SetHighlighted(true);
}

void ATrackContext::OnMouseClick(ASceneCamera& camera, int32_t pX, int32_t pY) {
    if (mTracks.size() == 0 || ImGuizmo::IsUsing()) {
        return;
    }

    RenderPickingBuffer(camera);
    uint32_t result = UViewportPicker::Query(pX, pY);

    ETrackNodePickType pickType = ETrackNodePickType((result & 0xC0000000) >> 30);
    uint16_t trackIdx = ((result & 0x3FFF0000) >> 16) - 1;
    uint16_t pointIdx = (result & 0xFFFF) - 1;

    if (bSelectingJunctionPartner) {
        if (pickType != ETrackNodePickType::Position) {
            return;
        }

        std::shared_ptr<UTracks::UTrackPoint> junctionPartner = mTrackPoints[trackIdx][pointIdx];
        if (junctionPartner->HasJunctionPartner()) {
            junctionPartner->SetJunctionPartner(nullptr);
        }

        std::shared_ptr<UTracks::UTrackPoint> selectedNode = mSelectedPoints[0].lock();
        selectedNode->SetJunctionPartner(junctionPartner);
        junctionPartner->SetJunctionPartner(selectedNode);
        
        glm::vec3 middlePos = (junctionPartner->GetPosition() + selectedNode->GetPosition()) / 2.0f;
        selectedNode->GetPositionForEditor() = middlePos;
        junctionPartner->GetPositionForEditor() = middlePos;

        glm::vec3 middleHandleA = (junctionPartner->GetHandleA() + selectedNode->GetHandleA()) / 2.0f;
        selectedNode->GetHandleAForEditor() = middleHandleA;
        junctionPartner->GetHandleAForEditor() = middleHandleA;

        glm::vec3 middleHandleB = (junctionPartner->GetHandleB() + selectedNode->GetHandleB()) / 2.0f;
        selectedNode->GetHandleBForEditor() = middleHandleB;
        junctionPartner->GetHandleBForEditor() = middleHandleB;

        bSelectingJunctionPartner = false;
    }
    else {
        if (result == 0 || !AInput::GetKeyDown(341)) {
            ClearSelectedPoints();
        }

        if (result == 0) {
            mSelectedPickType = ETrackNodePickType::Position;
            return;
        }

        mSelectedPickType = pickType;

        std::shared_ptr<UTracks::UTrackPoint> selectedPoint = mTrackPoints[trackIdx][pointIdx];
        mSelectedPoints.push_back(selectedPoint);

        selectedPoint->SetSelected(true);
    }
}

void ATrackContext::ClearSelectedPoints() {
    for (std::weak_ptr<UTracks::UTrackPoint> pnt : mSelectedPoints) {
        pnt.lock()->SetSelected(false);
    }

    mSelectedPoints.clear();
}
