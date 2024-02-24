#pragma once

#include "types.h"
#include "application/ACamera.hpp"

namespace UTracks {
    class UTrack;
}

class ATrackContext {
    shared_vector<UTracks::UTrack> mTracks;

    bool bGLInitialized;
    uint32_t mPntVBO, mPntIBO, mPntVAO, mSimpleProgram;

    void InitSimpleShader();
    void DestroyGLResources();

public:
    ATrackContext();
    ~ATrackContext();

    void InitGLResources();
    void Render(ASceneCamera& camera);

    void LoadTracks(std::filesystem::path filePath);
    void SaveTracks(std::filesystem::path dirPath);
};
