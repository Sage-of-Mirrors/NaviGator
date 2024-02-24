#include "application/ATrackContext.hpp"
#include "tracks/UTrack.hpp"
#include "tracks/UTrackPoint.hpp"
#include "ubo/common.hpp"
#include "util/fileutil.hpp"

#include "primitives/USphere.hpp"

#include <pugixml.hpp>
#include <glad/glad.h>
#include <iostream>

constexpr char* TRACKS_CHILD_NAME = "train_tracks";
constexpr char* TRACKS_FILE_NAME = "traintracks.xml";

constexpr uint32_t VERTEX_ATTRIB_INDEX = 0;

ATrackContext::ATrackContext() : mPntVBO(0), mPntIBO(0), mPntVAO(0), mSimpleProgram(0), bGLInitialized(false) {

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
        track->LoadNodePoints(configDir);

        mTracks.push_back(track);
    }
}

void ATrackContext::SaveTracks(std::filesystem::path dirPath) {
    std::filesystem::path fullConfigPath = dirPath / TRACKS_FILE_NAME;
    pugi::xml_document doc;

    pugi::xml_node rootNode = doc.append_child(TRACKS_CHILD_NAME);
    rootNode.append_attribute("version").set_value("1");

    for (std::shared_ptr<UTracks::UTrack> track : mTracks) {
        track->Serialize(rootNode.append_child("train_track"));
    }

    //pugi::xml_writer_file()
    //doc.s
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

    for (std::shared_ptr<UTracks::UTrack> track : mTracks) {
        for (const std::shared_ptr<UTracks::UTrackPoint> pnt : track->GetPoints()) {
            UCommonUniformBuffer::SetModelMatrix(glm::translate(glm::identity<glm::mat4>(), pnt->GetPosition()));
            UCommonUniformBuffer::SubmitUBO();

            glDrawElements(GL_TRIANGLES, USphere::IndexCount, GL_UNSIGNED_INT, 0);
        }
    }

    glUseProgram(0);
    glBindVertexArray(0);
}
