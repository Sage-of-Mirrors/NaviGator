#pragma once

#include "types.h"
#include "application/ACamera.hpp"

namespace UTracks {
    class UTrack;
    class UTrackPoint;
}

class ATrackContext {
    shared_vector<UTracks::UTrack> mTracks;

    using points_vector = std::vector<shared_vector<UTracks::UTrackPoint>>;
    points_vector mTrackPoints;

    bool bGLInitialized;
    uint32_t mPntVBO, mPntIBO, mPntVAO, mSimpleProgram, mBaseColorUniform;

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
