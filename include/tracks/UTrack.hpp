#pragma once

#include "types.h"

namespace pugi {
    class xml_node;
}

namespace UTracks {
    class UTrackPoint;

    class UTrack {
        // XML info

        std::string mGameFilename;
        std::string mConfigName;
        bool bStopsAtStations;
        uint32_t mBrakingDist;

        // DAT info

        bool bLoops;
        uint32_t mCurvePointCount;

        void PreprocessNodes(shared_vector<UTrackPoint>& points);

    public:
        UTrack();
        ~UTrack();

        void Deserialize(pugi::xml_node& node);
        void Serialize(pugi::xml_node& node);

        shared_vector<UTracks::UTrackPoint> LoadNodePoints(std::filesystem::path dirName);
        void SaveNodePoints(std::filesystem::path dirName, shared_vector<UTrackPoint>& points);

        const std::string GetConfigName() const { return mConfigName; }

        std::string* GetConfigNameForEditor() { return &mConfigName; }
        bool* GetStopsAtStationsForEditor() { return &bStopsAtStations; }
        bool* GetLoopsForEditor() { return &bLoops; }
        uint32_t* GetBrakingDistForEditor() { return &mBrakingDist; }
    };
}