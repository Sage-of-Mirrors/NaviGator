#pragma once

#include "types.h"
#include "application/ACamera.hpp"

#include <librdr3.hpp>

struct ANavmeshRenderData {
    uint32_t mNavIndexCount;
    uint32_t mNavVBO, mNavIBO, mNavVAO;

    void CreateNavResources(std::shared_ptr<CNavmeshData> data);
    void DeleteNavResources();

    ANavmeshRenderData();
    ~ANavmeshRenderData();

    void Render();
};

class ANavContext {
    std::vector<std::shared_ptr<ANavmeshRenderData>> mLoadedNavmeshes;
    uint32_t mLitSimpleProgram;
    float f = 0;

public:
    ANavContext();
    ~ANavContext();

    void LoadNavmesh(std::filesystem::path filePath);

    void Render(ASceneCamera& camera);
    
    void OnGLInitialized();
};