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

        bool bOpen;
        uint32_t mCurvePointCount;
        shared_vector<UTrackPoint> mPoints;

        void PreprocessNodes();

    public:
        UTrack();
        ~UTrack();

        void Deserialize(pugi::xml_node& node);
        void Serialize(pugi::xml_node& node);

        bool LoadNodePoints(std::filesystem::path dirName);
        void SaveNodePoints(std::filesystem::path dirName);

        const shared_vector<UTrackPoint> const GetPoints() { return mPoints; }
    };
}