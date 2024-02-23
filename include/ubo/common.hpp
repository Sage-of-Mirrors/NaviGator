#pragma once

#include "types.h"

namespace UCommonUniformBuffer {
    void CreateUBO();
    void DestroyUBO();

    void SubmitUBO();
    void ClearUBO();

    void LinkShaderToUBO(const uint32_t program);

    void SetProjAndViewMatrices(const glm::mat4& proj, const glm::mat4& view);
    void SetModelMatrix(const glm::mat4& model);
}
