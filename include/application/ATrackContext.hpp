#pragma once

#include "types.h"
#include "application/ACamera.hpp"

namespace UTracks {
    class UTrack;
    class UTrackPoint;
}

enum ETrackNodePickType : uint8_t {
    Position,
    Handle_A,
    Handle_B
};

class CPathRenderer;

struct APointSelection {
    uint16_t TrackIdx, PointIdx;

    void Get(uint16_t& trackIdx, uint16_t& pointIdx) {
        trackIdx = TrackIdx;
        pointIdx = PointIdx;
    }
};

class ATrackContext {
    shared_vector<UTracks::UTrack> mTracks;

    using points_vector = std::vector<shared_vector<UTracks::UTrackPoint>>;
    points_vector mTrackPoints;

    shared_vector<CPathRenderer> mPathRenderers;

    bool bGLInitialized;
    uint32_t mPntVBO, mPntIBO, mPntVAO, mSimpleProgram, mBaseColorUniform;

    std::weak_ptr<UTracks::UTrack> mSelectedTrack;
    std::vector<APointSelection> mSelectedPoints;

    ETrackNodePickType mSelectedPickType;

    std::string mPendingNewTrackName;
    bool bTrackDialogOpen;
    bool bCanDuplicatePoint;

    bool bSelectingJunctionPartner;

    void InitSimpleShader();
    void DestroyGLResources();

    void RenderTrackDataEditor(std::shared_ptr<UTracks::UTrack> track);
    void RenderPointDataEditorSingle(std::shared_ptr<UTracks::UTrackPoint> point);
    void RenderPointDataEditorMulti();

    void RenderNewTrackDialog();

    void RenderPickingBuffer(ASceneCamera& camera);

    void PostprocessNodes();

    void ClearSelectedPoints();

public:
    ATrackContext();
    ~ATrackContext();

    void InitGLResources();

    void RenderTreeView();
    void RenderDataEditor();
    void Render(ASceneCamera& camera);
    void RenderUI(ASceneCamera& camera);

    void OnMouseHover(ASceneCamera& camera, int32_t pX, int32_t pY);
    void OnMouseClick(ASceneCamera& camera, int32_t pX, int32_t pY);

    void LoadTracks(std::filesystem::path filePath);
    void SaveTracks(std::filesystem::path dirPath);
};
