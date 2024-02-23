#pragma once

#include "types.h"
#include "application/ACamera.hpp"

class UViewport {
    ASceneCamera mCamera;
    std::string mViewportName;

    uint32_t mFBO;
    uint32_t mTexIds[2];

    glm::vec2 mViewportPos;
    glm::vec2 mViewportSize;

    bool bIsOpen;

    void CreateFramebuffer();
    void ResizeViewport();
    void Clear();

public:
    UViewport() : UViewport("Viewport") { }
    UViewport(std::string name);
    ~UViewport();

    bool IsOpen() const { return bIsOpen; }

    glm::vec2 GetViewportSize() const { return mViewportSize; }
    glm::vec2 GetViewportPosition() const { return mViewportPos; }

    ASceneCamera& GetCamera() { return mCamera; }

    void BindViewport();
    void UnbindViewport();

    void RenderUI(float deltaTime);
    //void RenderScene(AJ3DContext* ctx, float deltaTime);
};