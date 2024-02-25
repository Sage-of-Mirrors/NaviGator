#include "ubo/litsimple.hpp"

#include <glad/glad.h>

namespace ULitSimpleUniformBuffer {
    namespace {
        constexpr const char* UBO_NAME = "uLitSimpleData";

        struct ULitSimpleUniformBufferObject {
            ULight mLight;
            glm::vec4 mAmbientColor;
            glm::vec4 mViewPos;
        };

        static ULitSimpleUniformBufferObject mInst;
        uint32_t mHandle;
    }
}

void ULitSimpleUniformBuffer::CreateUBO() {
    glCreateBuffers(1, &mHandle);

    glNamedBufferStorage(mHandle, sizeof(ULitSimpleUniformBufferObject), nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferRange(GL_UNIFORM_BUFFER, 1, mHandle, 0, sizeof(ULitSimpleUniformBufferObject));
}

void ULitSimpleUniformBuffer::DestroyUBO() {
    glDeleteBuffers(1, &mHandle);
    mHandle = 0;
}

void ULitSimpleUniformBuffer::SubmitUBO() {
    if (mHandle == 0)
        return;

    glNamedBufferSubData(mHandle, NULL, sizeof(ULitSimpleUniformBufferObject), &mInst);
}

void ULitSimpleUniformBuffer::ClearUBO() {
    mInst.mLight.mPos = glm::zero<glm::vec4>();
    mInst.mLight.mColor = glm::zero<glm::vec4>();
    mInst.mLight.mAtten = 0.0f;
    mInst.mLight.mAmbCoeff = 0.0f;

    mInst.mAmbientColor = glm::zero<glm::vec4>();
}

void ULitSimpleUniformBuffer::LinkShaderToUBO(const uint32_t program) {
    if (mHandle == 0)
        CreateUBO();

    uint32_t uniformIndex = glGetUniformBlockIndex(program, UBO_NAME);
    glUniformBlockBinding(program, uniformIndex, 1);
}

void ULitSimpleUniformBuffer::SetLight(const glm::vec4& pos, const glm::vec4 col, const float atten, const float ambCoeff) {
    mInst.mLight.mPos = pos;
    mInst.mLight.mColor = col;
    mInst.mLight.mAtten = atten;
    mInst.mLight.mAmbCoeff = ambCoeff;
}

void ULitSimpleUniformBuffer::SetAmbientColor(const glm::vec4& ambColor) {
    mInst.mAmbientColor = ambColor;
}

void ULitSimpleUniformBuffer::SetViewPos(const glm::vec4& viewPos) {
    mInst.mViewPos = viewPos;
}
