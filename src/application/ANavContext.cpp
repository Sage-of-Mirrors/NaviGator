#include "application/ANavContext.hpp"
#include "ubo/common.hpp"
#include "ubo/litsimple.hpp"
#include "util/fileutil.hpp"

#include <glad/glad.h>

constexpr uint32_t VERTEX_ATTRIB_INDEX = 0;
constexpr uint32_t NORMAL_ATTRIB_INDEX = 1;

ANavmeshRenderData::ANavmeshRenderData() : mNavIndexCount(0), mNavVBO(0), mNavIBO(0), mNavVAO(0) {

}

ANavmeshRenderData::~ANavmeshRenderData() {
    DeleteNavResources();
}

void ANavmeshRenderData::CreateNavResources(librdr3::UNavmeshShared data) {
    uint32_t numVertices = 0;
    float* vertexData = nullptr;
    uint32_t* indexData = nullptr;
    data->GetVertices(vertexData, indexData, numVertices, mNavIndexCount);

    //data->GetIndicesForPolys(indexData, mNavIndexCount);

    DeleteNavResources();

    glCreateBuffers(1, &mNavVBO);
    glCreateBuffers(1, &mNavIBO);

    glNamedBufferStorage(mNavVBO, numVertices    * (sizeof(glm::vec3) + sizeof(glm::vec3)), vertexData, GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferStorage(mNavIBO, mNavIndexCount * sizeof(uint32_t),  indexData,  GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);

    delete[] vertexData;
    delete[] indexData;

    glCreateVertexArrays(1, &mNavVAO);
    glVertexArrayVertexBuffer(mNavVAO, 0, mNavVBO, 0, (sizeof(glm::vec3) + sizeof(glm::vec3)));
    glVertexArrayElementBuffer(mNavVAO, mNavIBO);

    glEnableVertexArrayAttrib(mNavVAO,  VERTEX_ATTRIB_INDEX);
    glVertexArrayAttribBinding(mNavVAO, VERTEX_ATTRIB_INDEX, 0);
    glVertexArrayAttribFormat(mNavVAO,  VERTEX_ATTRIB_INDEX, glm::vec3::length(), GL_FLOAT, GL_FALSE, 0);

    glEnableVertexArrayAttrib(mNavVAO, NORMAL_ATTRIB_INDEX);
    glVertexArrayAttribBinding(mNavVAO, NORMAL_ATTRIB_INDEX, 0);
    glVertexArrayAttribFormat(mNavVAO, NORMAL_ATTRIB_INDEX, glm::vec3::length(), GL_FLOAT, GL_FALSE, sizeof(glm::vec3));
}

void ANavmeshRenderData::DeleteNavResources() {
    uint32_t buffers[]{ mNavVBO, mNavIBO };

    glDeleteBuffers(2, buffers);
    glDeleteVertexArrays(1, &mNavVAO);

    mNavVBO = 0;
    mNavIBO = 0;
    mNavVAO = 0;
}

void ANavmeshRenderData::Render() {
    glBindVertexArray(mNavVAO);

    ULitSimpleUniformBuffer::SetAmbientColor(glm::vec4(0.15f, 0.15f, 0.15f, 1.0f));
    ULitSimpleUniformBuffer::SubmitUBO();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    //glPolygonOffset(-5.0f, 1.0f);

    glDrawElements(GL_TRIANGLES, mNavIndexCount, GL_UNSIGNED_INT, 0);

    //ULitSimpleUniformBuffer::SetAmbientColor(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    //ULitSimpleUniformBuffer::SubmitUBO();

    //glDisable(GL_BLEND);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //glPolygonOffset(-10.0f, 1.0f);

    //glDrawElements(GL_TRIANGLES, mNavIndexCount, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
}

ANavContext::ANavContext() : mLitSimpleProgram(0) {

}

ANavContext::~ANavContext() {

}

void ANavContext::LoadNavmesh(std::filesystem::path filePath) {
    std::shared_ptr<ANavmeshRenderData> newData = std::make_shared<ANavmeshRenderData>();
    newData->CreateNavResources(librdr3::ImportYnv(filePath.generic_string()));

    mLoadedNavmeshes.push_back(newData);
}

void ANavContext::Render(ASceneCamera& camera) {
    if (mLoadedNavmeshes.size() == 0) {
        return;
    }

    UCommonUniformBuffer::SetProjAndViewMatrices(camera.GetProjectionMatrix(), camera.GetViewMatrix());
    UCommonUniformBuffer::SetModelMatrix(glm::identity<glm::mat4>());
    UCommonUniformBuffer::SubmitUBO();

    f += 0.166666f;

    ULitSimpleUniformBuffer::SetLight(glm::vec4(0.0f, 10000.0f, 0.0f, 1.0f), glm::vec4(0.75f, 0.75f, 0.75f, 1.0f), 1.0f, 0.0f);
    ULitSimpleUniformBuffer::SetViewPos(glm::vec4(camera.GetPosition(), 1.0f));

    glUseProgram(mLitSimpleProgram);

    for (std::shared_ptr<ANavmeshRenderData> r : mLoadedNavmeshes) {
        r->Render();
    }

    glUseProgram(0);
}

void ANavContext::OnGLInitialized() {
    // Compile vertex shader
    std::string vertTxt = UFileUtil::LoadShaderText("lit_simple.vert");
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
    std::string fragTxt = UFileUtil::LoadShaderText("lit_simple.frag");
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
    mLitSimpleProgram = glCreateProgram();
    glAttachShader(mLitSimpleProgram, vertHandle);
    glAttachShader(mLitSimpleProgram, fragHandle);
    glLinkProgram(mLitSimpleProgram);

    // Clean up
    glDetachShader(mLitSimpleProgram, vertHandle);
    glDetachShader(mLitSimpleProgram, fragHandle);
    glDeleteShader(vertHandle);
    glDeleteShader(fragHandle);

    UCommonUniformBuffer::LinkShaderToUBO(mLitSimpleProgram);
    ULitSimpleUniformBuffer::LinkShaderToUBO(mLitSimpleProgram);
}
