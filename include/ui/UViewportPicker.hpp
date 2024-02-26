#pragma once

#include "types.h"

namespace UViewportPicker {
    void CreatePicker(uint32_t width, uint32_t height);
    void ResizePicker(uint32_t width, uint32_t height);
    void DestroyPicker();

    void BindBuffer();
    void UnbindBuffer();

    void SetIdUniform(uint32_t id);

    uint32_t Query(uint32_t x, uint32_t y);
}