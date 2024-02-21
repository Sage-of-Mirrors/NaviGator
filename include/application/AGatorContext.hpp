#pragma once

#include <glm/vec2.hpp>

#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>


class MScenegraph;
class MJointData;
class MShapeData;
class MTextureData;
class MMaterialData;

class AJ3DContext;

namespace bStream {
    class CStream;
}

class AGatorContext {
    glm::vec2 mAppPosition;
    bool bIsDockingConfigured;

    uint32_t mMainDockSpaceID;
    uint32_t mDockNodeTopID;
    uint32_t mDockNodeRightID;
    uint32_t mDockNodeDownID;
    uint32_t mDockNodeLeftID;

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
