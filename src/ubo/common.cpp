#include "ubo/common.hpp"

#include <glad/glad.h>

namespace UCommonUniformBuffer {
    namespace {
        constexpr char* UBO_NAME = "uSharedData";

        struct UCommonUniformBufferObject {
            glm::mat4 mProj;
            glm::mat4 mView;
            glm::mat4 mModel;
        };

        static UCommonUniformBufferObject mInst;
        uint32_t mHandle;
    }
}

void UCommonUniformBuffer::CreateUBO() {
    glCreateBuffers(1, &mHandle);

    glNamedBufferStorage(mHandle, sizeof(UCommonUniformBufferObject), nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, mHandle, 0, sizeof(UCommonUniformBufferObject));
}

void UCommonUniformBuffer::DestroyUBO() {
    glDeleteBuffers(1, &mHandle);
    mHandle = 0;
}

void UCommonUniformBuffer::SubmitUBO() {
    if (mHandle == 0)
        return;

    glNamedBufferSubData(mHandle, NULL, sizeof(UCommonUniformBufferObject), &mInst);
}

void UCommonUniformBuffer::ClearUBO() {
    mInst.mProj = glm::identity<glm::mat4>();
    mInst.mView = glm::identity<glm::mat4>();
    mInst.mModel = glm::identity<glm::mat4>();
}

void UCommonUniformBuffer::LinkShaderToUBO(const uint32_t program) {
    if (mHandle == 0)
        CreateUBO();

    uint32_t uniformIndex = glGetUniformBlockIndex(program, UBO_NAME);
    glUniformBlockBinding(program, uniformIndex, 0);
}

void UCommonUniformBuffer::SetProjAndViewMatrices(const glm::mat4& proj, const glm::mat4& view) {
    mInst.mProj = proj;
    mInst.mView = view;
}

void UCommonUniformBuffer::SetModelMatrix(const glm::mat4& model) {
    mInst.mModel = model;
}