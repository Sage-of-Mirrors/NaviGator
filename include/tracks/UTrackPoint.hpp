#pragma once

#include "types.h"

#include <sstream>

namespace UTracks {
    enum ENodeInfoBits {
        BITS_STATION_TYPE   = 0x03,
        BITS_IS_TUNNEL      = 0x04,
        BITS_IS_JUNCTION    = 0x08
    };

    enum ENodeStationType : uint8_t {
        None,
        Left_Side,
        Right_Side
    };

    class UTrackPoint {
        // Common properties

        // Primary location of the point in world space.
        glm::vec3 mPosition;
        // What kind of station this node represents - can be None, a station on the train's left side,
        // or a station on the train's right side.
        ENodeStationType mStationType;
        // Whether this node is inside of a tunnel.
        bool bIsTunnel;
        // Whether this node can act as a junction to another track config.
        bool bIsJunction;

        // If bIsSwitch is false and mStationType is not None, this is the name of the station the node represents.
        // If bIsSwitch is true, this is the name of the track config this node can switch to.
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

        void TrySetJunctionArgument();

        const glm::vec3& GetPosition() const { return mPosition; }
        const glm::vec3& GetHandleA() const { return mHandleA; }
        const glm::vec3& GetHandleB() const { return mHandleB; }
        const std::string& GetParentTrackName() const { return mParentTrackName; }
        const ENodeStationType& GetStationType() const { return mStationType; }

        void SetParentTrackName(std::string name) { mParentTrackName = name; }

        bool IsCurve() const { return bIsCurve; }
        bool IsJunction() const { return bIsJunction; }

        bool* GetIsTunnelForEditor() { return &bIsTunnel; }
        bool* GetIsJunctionForEditor() { return &bIsJunction; }
        std::string* GetArgumentForEditor() { return &mArgument; }
        float* GetScalarForEditor() { return &mSomeScalar; }
        ENodeStationType& GetStationTypeForEditor() { return mStationType; }
        glm::vec3& GetPositionForEditor() { return mPosition; }
    };
}
