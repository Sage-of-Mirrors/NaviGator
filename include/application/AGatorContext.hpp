#pragma once

#include "types.h"

class UViewport;
class ANavContext;

class AGatorContext {
    glm::vec2 mAppPosition;
    bool bIsDockingConfigured;

    uint32_t mMainDockSpaceID;
    uint32_t mDockNodeTopID;
    uint32_t mDockNodeRightID;
    uint32_t mDockNodeDownID;
    uint32_t mDockNodeLeftID;

    std::shared_ptr<UViewport> mMainViewport;

    std::shared_ptr<ANavContext> mNavContext;

    void LoadFileCB();

    void OpenFile(std::filesystem::path filePath);

public:
    AGatorContext();
    ~AGatorContext();
    
    void SetUpDocking();
    void RenderMenuBar();

    void SetAppPosition(const int xPos, const int yPos);

    void Update(float deltaTime);
    void Render(float deltaTime);
    void PostRender(float deltaTime);

    void OnGLInitialized();
    void OnFileDropped(std::filesystem::path filePath);
};
