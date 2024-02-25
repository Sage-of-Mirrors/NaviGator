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

    std::weak_ptr<UTracks::UTrack> mSelectedTrack;
    std::vector<std::weak_ptr<UTracks::UTrackPoint>> mSelectedPoints;

    void InitSimpleShader();
    void DestroyGLResources();

    void RenderTrackDataEditor(std::shared_ptr<UTracks::UTrack> track);
    void RenderPointDataEditorSingle(std::shared_ptr<UTracks::UTrackPoint> point);
    void RenderPointDataEditorMulti();

public:
    ATrackContext();
    ~ATrackContext();

    void InitGLResources();
    void RenderTreeView();
    void RenderDataEditor();
    void Render(ASceneCamera& camera);
    void RenderUI(ASceneCamera& camera);

    void LoadTracks(std::filesystem::path filePath);
    void SaveTracks(std::filesystem::path dirPath);
};
