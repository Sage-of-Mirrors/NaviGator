#include "application/ATrackContext.hpp"
#include "tracks/UTrack.hpp"
#include "tracks/UTrackPoint.hpp"
#include "ubo/common.hpp"
#include "util/fileutil.hpp"
#include "util/uiutil.hpp"
#include "ui/UViewportPicker.hpp"
#include "application/AInput.hpp"
#include "ui/UPathRenderer.hpp"

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

    for (uint32_t i = 0; i < mTracks.size(); i++) {
        mTrackPoints.push_back(mTracks[i]->LoadNodePoints(configDir));

        std::shared_ptr<CPathRenderer> pathRenderer = std::make_shared<CPathRenderer>();
        pathRenderer->Init();
        for (std::shared_ptr<UTracks::UTrackPoint> pnt : mTrackPoints[i]) {
            pathRenderer->mPath.push_back({ pnt->GetPosition(), {1, 0, 0, 1,}, pnt->GetHandleA(), pnt->GetHandleB() });
        }
        pathRenderer->UpdateData();

        mPathRenderers.push_back(pathRenderer);
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
            ImGui::PushID(i);

            if (track->IsHidden()) {
                if (ImGui::Button("Show", { 40, 0 })) {
                    track->SetHidden(false);
                }
            }
            else {
                if (ImGui::Button("Hide", { 40, 0 })) {
                    track->SetHidden(true);
                }
            }

            ImGui::SameLine();

            bool isSelected = !mSelectedTrack.expired() && mSelectedTrack.lock() == track;
            if (ImGui::Selectable(track->GetConfigName().c_str(), isSelected)) {
                mSelectedTrack = track;
            }

            ImGui::PopID();
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
        uint16_t trackIdx, pointIdx;
        mSelectedPoints[0].Get(trackIdx, pointIdx);

        RenderPointDataEditorSingle(mTrackPoints[trackIdx][pointIdx]);
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
        ImGui::Checkbox("Is in a tunnel?", point->GetIsTunnelForEditor());

        ImGui::Spacing();
        if (ImGui::Checkbox("Is curve?", point->GetIsCurveForEditor())) {

        }

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
            // Create new track
            std::shared_ptr<UTracks::UTrack> newTrack = std::make_shared<UTracks::UTrack>(mPendingNewTrackName);
            mTracks.push_back(newTrack);

            // Create track points vector
            shared_vector<UTracks::UTrackPoint> newTrackPoints;
            std::shared_ptr<UTracks::UTrackPoint> newPoint = std::make_shared<UTracks::UTrackPoint>(newTrack->GetConfigName());
            newTrackPoints.push_back(newPoint);
            mTrackPoints.push_back(newTrackPoints);

            // Create path renderer
            std::shared_ptr<CPathRenderer> pathRenderer = std::make_shared<CPathRenderer>();
            pathRenderer->Init();
            for (std::shared_ptr<UTracks::UTrackPoint> pnt : newTrackPoints) {
                pathRenderer->mPath.push_back({ pnt->GetPosition(), {1, 0, 0, 1,}, pnt->GetHandleA(), pnt->GetHandleB() });
            }
            pathRenderer->UpdateData();
            mPathRenderers.push_back(pathRenderer);

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

    uint16_t trackIdx, pointIdx;
    mSelectedPoints[0].Get(trackIdx, pointIdx);

    std::shared_ptr<UTracks::UTrackPoint> selPoint = mTrackPoints[trackIdx][pointIdx];

    bool bUpdated = false;
    switch (mSelectedPickType) {
        case ETrackNodePickType::Position:
        {
            glm::vec3 avgPosition = glm::zero<glm::vec3>();
            for (const APointSelection& s : mSelectedPoints) {
                avgPosition += mTrackPoints[s.TrackIdx][s.PointIdx]->GetPosition();
            }

            avgPosition /= mSelectedPoints.size();
            glm::mat4 modelMtx = glm::translate(glm::identity<glm::mat4>(), avgPosition);

            if (ImGuizmo::Manipulate(&viewMtx[0][0], &projMtx[0][0], ImGuizmo::OPERATION::TRANSLATE, ImGuizmo::WORLD, &modelMtx[0][0])) {
                if (mSelectedPoints.size() == 1 && ImGui::GetIO().KeyCtrl && bCanDuplicatePoint) {
                    bCanDuplicatePoint = false;
                    uint16_t insertIdx = pointIdx + 1;

                    std::shared_ptr<UTracks::UTrackPoint> newPt = std::make_shared<UTracks::UTrackPoint>(*selPoint);
                    for (uint32_t i = 0; i < mTracks.size(); i++) {
                        if (mTracks[i]->GetConfigName() == newPt->GetParentTrackName()) {
                            mTrackPoints[i].insert(mTrackPoints[i].begin() + insertIdx, newPt);

                            auto f = mPathRenderers[i]->mPath.begin() + insertIdx;
                            mPathRenderers[i]->mPath.insert(f, { newPt->GetPosition(), {1, 0, 0, 1}, newPt->GetHandleA(), newPt->GetHandleB() });
                            mPathRenderers[i]->UpdateData();

                            break;
                        }
                    }

                    ClearSelectedPoints();
                    mSelectedPoints.push_back({ trackIdx, insertIdx });

                    newPt->SetSelected(true);
                    selPoint = newPt;
                }

                glm::vec3 diff = glm::vec3(modelMtx[3]) - avgPosition;
                for (const APointSelection& s : mSelectedPoints) {
                    std::shared_ptr<UTracks::UTrackPoint> pnt = mTrackPoints[s.TrackIdx][s.PointIdx];

                    pnt->GetPositionForEditor() += diff;

                    pnt->GetHandleAForEditor() += diff;
                    pnt->GetHandleBForEditor() += diff;

                    if (pnt->HasJunctionPartner()) {
                        std::shared_ptr<UTracks::UTrackPoint> partner = pnt->GetJunctionPartner().lock();
                        partner->GetPositionForEditor() += diff;

                        partner->GetHandleAForEditor() += diff;
                        partner->GetHandleBForEditor() += diff;
                    }
                }

                bUpdated = true;
            }

            break;
        }
        case ETrackNodePickType::Handle_A:
        {
            glm::mat4 modelMtx = glm::translate(glm::identity<glm::mat4>(), selPoint->GetHandleA());

            if (ImGuizmo::Manipulate(&viewMtx[0][0], &projMtx[0][0], ImGuizmo::OPERATION::TRANSLATE, ImGuizmo::WORLD, &modelMtx[0][0])) {
                selPoint->GetHandleAForEditor() = glm::vec3(modelMtx[3]);

                if (ImGui::GetIO().KeyShift) {
                    glm::vec3 pointSpaceDelta = selPoint->GetHandleA() - selPoint->GetPosition();
                    selPoint->GetHandleBForEditor() = -pointSpaceDelta + selPoint->GetPosition();
                }

                if (selPoint->HasJunctionPartner()) {
                    std::shared_ptr<UTracks::UTrackPoint> partner = selPoint->GetJunctionPartner().lock();
                    partner->GetHandleAForEditor() = glm::vec3(modelMtx[3]);
                }
            }

            bUpdated = true;
            break;
        }
        case ETrackNodePickType::Handle_B:
        {
            glm::mat4 modelMtx = glm::translate(glm::identity<glm::mat4>(), selPoint->GetHandleB());

            if (ImGuizmo::Manipulate(&viewMtx[0][0], &projMtx[0][0], ImGuizmo::OPERATION::TRANSLATE, ImGuizmo::WORLD, &modelMtx[0][0])) {
                selPoint->GetHandleBForEditor() = glm::vec3(modelMtx[3]);

                if (ImGui::GetIO().KeyShift) {
                    glm::vec3 pointSpaceDelta = selPoint->GetHandleB() - selPoint->GetPosition();
                    selPoint->GetHandleAForEditor() = -pointSpaceDelta + selPoint->GetPosition();
                }

                if (selPoint->HasJunctionPartner()) {
                    std::shared_ptr<UTracks::UTrackPoint> partner = selPoint->GetJunctionPartner().lock();
                    partner->GetHandleBForEditor() = glm::vec3(modelMtx[3]);
                }
            }

            bUpdated = true;
            break;
        }
    }

    if (bUpdated) {
        for (const APointSelection& s : mSelectedPoints) {
            CPathPoint& p = mPathRenderers[s.TrackIdx]->mPath[s.PointIdx];
            p.Position = mTrackPoints[s.TrackIdx][s.PointIdx]->GetPosition();
            p.LeftHandle = mTrackPoints[s.TrackIdx][s.PointIdx]->GetHandleA();
            p.RightHandle = mTrackPoints[s.TrackIdx][s.PointIdx]->GetHandleB();

            mPathRenderers[s.TrackIdx]->UpdateData();
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

    for (uint32_t trackIdx = 0; trackIdx < mTracks.size(); trackIdx++) {
        if (mTracks[trackIdx]->IsHidden()) {
            continue;
        }

        for (const std::shared_ptr<UTracks::UTrackPoint> pnt : mTrackPoints[trackIdx]) {
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

    for (uint32_t trackIdx = 0; trackIdx < mTracks.size(); trackIdx++) {
        if (mTracks[trackIdx]->IsHidden()) {
            continue;
        }

        mPathRenderers[trackIdx]->Draw(camera, glm::identity<glm::mat4>());
    }
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

    for (uint32_t trackIdx = 0; trackIdx < mTracks.size(); trackIdx++) {
        if (mTracks[trackIdx]->IsHidden()) {
            continue;
        }

        for (uint32_t pointIdx = 0; pointIdx < mTrackPoints[trackIdx].size(); pointIdx++) {
            UCommonUniformBuffer::SetModelMatrix(glm::translate(glm::identity<glm::mat4>(), mTrackPoints[trackIdx][pointIdx]->GetPosition()));
            UCommonUniformBuffer::SubmitUBO();

            uint32_t trackId = ((trackIdx + 1) << 16) & 0x3FFF0000;
            uint32_t pointId = pointIdx + 1;
            UViewportPicker::SetIdUniform(trackId | pointId);

            glDrawElements(GL_TRIANGLES, USphere::IndexCount, GL_UNSIGNED_INT, 0);

            if (mTrackPoints[trackIdx][pointIdx]->IsSelected() && mTrackPoints[trackIdx][pointIdx]->IsCurve()) {
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
        if (pickType != ETrackNodePickType::Position || trackIdx == UINT16_MAX) {
            bSelectingJunctionPartner = false;
            return;
        }

        std::shared_ptr<UTracks::UTrackPoint> junctionPartner = mTrackPoints[trackIdx][pointIdx];
        if (junctionPartner->HasJunctionPartner()) {
            junctionPartner->SetJunctionPartner(nullptr);
        }

        uint16_t trackIdx, pointIdx;
        mSelectedPoints[0].Get(trackIdx, pointIdx);

        std::shared_ptr<UTracks::UTrackPoint> selPoint = mTrackPoints[trackIdx][pointIdx];
        selPoint->SetJunctionPartner(junctionPartner);
        junctionPartner->SetJunctionPartner(selPoint);
        
        glm::vec3 middlePos = (junctionPartner->GetPosition() + selPoint->GetPosition()) / 2.0f;
        selPoint->GetPositionForEditor() = middlePos;
        junctionPartner->GetPositionForEditor() = middlePos;

        glm::vec3 middleHandleA = (junctionPartner->GetHandleA() + selPoint->GetHandleA()) / 2.0f;
        selPoint->GetHandleAForEditor() = middleHandleA;
        junctionPartner->GetHandleAForEditor() = middleHandleA;

        glm::vec3 middleHandleB = (junctionPartner->GetHandleB() + selPoint->GetHandleB()) / 2.0f;
        selPoint->GetHandleBForEditor() = middleHandleB;
        junctionPartner->GetHandleBForEditor() = middleHandleB;

        bSelectingJunctionPartner = false;
    }
    else {
        if (result == 0 || !ImGui::GetIO().KeyCtrl) {
            ClearSelectedPoints();
        }

        if (result == 0) {
            mSelectedPickType = ETrackNodePickType::Position;
            return;
        }

        for (const APointSelection s : mSelectedPoints) {
            if (s.TrackIdx == trackIdx && s.PointIdx == pointIdx) {
                return;
            }
        }

        mSelectedPickType = pickType;

        mSelectedPoints.push_back({ trackIdx, pointIdx });
        mTrackPoints[trackIdx][pointIdx]->SetSelected(true);
    }
}

void ATrackContext::ClearSelectedPoints() {
    for (APointSelection pnt : mSelectedPoints) {
        uint16_t trackIdx, pointIdx;
        pnt.Get(trackIdx, pointIdx);

        mTrackPoints[trackIdx][pointIdx]->SetSelected(false);
    }

    mSelectedPoints.clear();
}
