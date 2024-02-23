#pragma once

#include "types.h"

namespace ULitSimpleUniformBuffer {
    struct ULight {
        glm::vec4 mPos;
        glm::vec4 mColor;
        float mAtten;
        float mAmbCoeff;
        float d1;
        float d2;
    };

    void CreateUBO();
    void DestroyUBO();

    void SubmitUBO();
    void ClearUBO();

    void LinkShaderToUBO(const uint32_t program);

    void SetLight(const glm::vec4& pos, const glm::vec4 col, const float atten, const float ambCoeff);
    void SetAmbientColor(const glm::vec4& ambColor);
    void SetViewPos(const glm::vec4& viewPos);
}