#pragma once

#include "types.h"

class UViewport;
class ANavContext;
class ATrackContext;

class AGatorContext {
    glm::vec2 mAppPosition;
    bool bIsDockingConfigured;

    uint32_t mMainDockSpaceID;
    uint32_t mDockNodeTopID;
    uint32_t mDockNodeRightID;
    uint32_t mDockNodeDownID;
    uint32_t mPropertiesDockNodeID;

    uint32_t mPropertiesPanelTopID;
    uint32_t mPropertiesPanelBottomID;

    std::shared_ptr<UViewport> mMainViewport;

    std::shared_ptr<ANavContext> mNavContext;
    std::shared_ptr<ATrackContext> mTrackContext;

    void RenderPropertiesPanel();

    void LoadFileCB();

    void SaveTracksAsCB();

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
