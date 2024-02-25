#include "application/ATrackContext.hpp"
#include "tracks/UTrack.hpp"
#include "tracks/UTrackPoint.hpp"
#include "ubo/common.hpp"
#include "util/fileutil.hpp"
#include "util/uiutil.hpp"

#include "primitives/USphere.hpp"

#include <pugixml.hpp>
#include <glad/glad.h>
#include <imgui.h>

#include <iostream>
#include <algorithm>
#include <format>

constexpr const char* TRACKS_CHILD_NAME = "train_tracks";
constexpr const char* TRACKS_FILE_NAME = "traintracks.xml";

constexpr uint32_t VERTEX_ATTRIB_INDEX = 0;

constexpr glm::vec4 NORMAL_COLOR = { 1.0f, 0.5f, 0.5f, 1.0f };
constexpr glm::vec4 CURVE_COLOR = { 0.5f, 1.0f, 0.5f, 1.0f };
constexpr glm::vec4 HANDLE_COLOR = { 0.5f, 0.5f, 1.0f, 1.0f };
constexpr glm::vec4 JUNCTION_COLOR = { 1.0f, 0.5f, 1.0f, 1.0f };

ATrackContext::ATrackContext() : mPntVBO(0), mPntIBO(0), mPntVAO(0), mSimpleProgram(0), bGLInitialized(false), mBaseColorUniform(0),
    mSelectedTrack()
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
        track->SaveNodePoints(dirPath);
    }

    doc.save_file(fullConfigPath.c_str(), PUGIXML_TEXT("\t"), pugi::format_indent | pugi::format_indent_attributes | pugi::format_save_file_text, pugi::encoding_utf8);
}

void ATrackContext::RenderTreeView() {
    if (mTracks.size() == 0) {
        ImGui::Text("Please load traintracks.xml.");
        return;
    }

    ImGui::SetNextItemOpen(true);
    if (ImGui::TreeNode("Track Configs")) {
        ImGui::Indent();

        for (uint32_t i = 0; i < mTracks.size(); i++) {
            std::shared_ptr<UTracks::UTrack> track = mTracks[i];

            bool isSelected = !mSelectedTrack.expired() && mSelectedTrack.lock() == track;
            std::string imguiId = std::format("{}##{}", track->GetConfigName(), i);
            
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
            // TODO: junction selection UI
            ImGui::Text("TODO: Junction selection UI");
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
            if (pnt->IsJunction()) {
                glUniform4fv(mBaseColorUniform, 1, &CURVE_COLOR.x);
            }
            else {
                glUniform4fv(mBaseColorUniform, 1, &NORMAL_COLOR.x);
            }

            UCommonUniformBuffer::SetModelMatrix(glm::translate(glm::identity<glm::mat4>(), pnt->GetPosition()));
            UCommonUniformBuffer::SubmitUBO();

            glDrawElements(GL_TRIANGLES, USphere::IndexCount, GL_UNSIGNED_INT, 0);

            if (pnt->IsCurve()) {
                glUniform4fv(mBaseColorUniform, 1, &HANDLE_COLOR.x);

                UCommonUniformBuffer::SetModelMatrix(glm::scale(glm::translate(glm::identity<glm::mat4>(), pnt->GetHandleA()), glm::vec3(0.5f, 0.5f, 0.5f)));
                UCommonUniformBuffer::SubmitUBO();

                glDrawElements(GL_TRIANGLES, USphere::IndexCount, GL_UNSIGNED_INT, 0);

                UCommonUniformBuffer::SetModelMatrix(glm::scale(glm::translate(glm::identity<glm::mat4>(), pnt->GetHandleB()), glm::vec3(0.5f, 0.5f, 0.5f)));
                UCommonUniformBuffer::SubmitUBO();

                glDrawElements(GL_TRIANGLES, USphere::IndexCount, GL_UNSIGNED_INT, 0);
            }
        }
    }

    glUseProgram(0);
    glBindVertexArray(0);
}
