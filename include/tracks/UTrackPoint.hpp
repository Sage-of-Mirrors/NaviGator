#pragma once

#include "types.h"

#include <sstream>

namespace UTracks {
    enum ETrackPointType : uint8_t {
        Normal = 0,
        Stop_1 = 1,
        Stop_2 = 2,
        // No 3?
        Unk_4 = 4,
        // No 5, 6, 7?
        Switch = 8
    };

    class UTrackPoint {
        // Common properties

        // Primary location of the point in world space.
        glm::vec3 mPosition;
        uint32_t mType;
        // Purpose varies depending on mType.
        std::string mArgument;

        // Properties for curve points

        // Whether this node is a curve node. Internal use only.
        bool bIsCurve;
        glm::vec3 mHandleA;
        glm::vec3 mHandleB;
        float mSomeScalar;

        // Name of the track this point belongs to.
        std::string mParentTrackName;
        // Weak ref to the point this one is connected to on another track, assuming mType == Switch.
        std::weak_ptr<UTrackPoint> mSwitchPartner;

    public:
        UTrackPoint();
        ~UTrackPoint();

        void LoadPoint(std::stringstream& stream);
        void SavePoint(std::stringstream& stream);

        void UpdateArgument();

        const glm::vec3& GetPosition() const { return mPosition; }
        const std::string& GetParentTrackName() const { return mParentTrackName; }

        void SetParentTrackName(std::string name) { mParentTrackName = name; }

        bool IsCurve() const { return bIsCurve; }
    };
}
