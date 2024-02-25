#include "tracks/UTrack.hpp"
#include "tracks/UTrackPoint.hpp"

#include <pugixml.hpp>
#include <imgui.h>

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>

UTracks::UTrack::UTrack() : mGameFilename(""), mConfigName(""), bStopsAtStations(false), mBrakingDist(10), mCurvePointCount(0)
{

}

UTracks::UTrack::~UTrack() {

}

void UTracks::UTrack::Deserialize(pugi::xml_node& node) {
    mGameFilename = node.attribute("filename").as_string();
    mConfigName = node.attribute("trainConfigName").as_string();
    bStopsAtStations = node.attribute("stopsAtStations").as_bool();
    mBrakingDist = node.attribute("brakingDist").as_int();
}

void UTracks::UTrack::Serialize(pugi::xml_node& node) {
    pugi::xml_attribute filenameAttribute = node.append_attribute("filename");
    filenameAttribute.set_value(mGameFilename.data());

    pugi::xml_attribute configNameAttribute = node.append_attribute("trainConfigName");
    configNameAttribute.set_value(mConfigName.data());

    pugi::xml_attribute stopsAtStationsAttribute = node.append_attribute("stopsAtStations");
    stopsAtStationsAttribute.set_value(bStopsAtStations);

    pugi::xml_attribute brakingDistAttribute = node.append_attribute("brakingDist");
    brakingDistAttribute.set_value(mBrakingDist);
}

shared_vector<UTracks::UTrackPoint> UTracks::UTrack::LoadNodePoints(std::filesystem::path dirName) {
    std::filesystem::path gamePath(mGameFilename);
    std::filesystem::path extPath = dirName / gamePath.filename();

    shared_vector<UTracks::UTrackPoint> points;
    if (!std::filesystem::exists(extPath)) {
        return points;
    }

    std::ifstream nodesFile(extPath.c_str());

    // From https://stackoverflow.com/a/2912614
    std::string streamSrc = std::string(std::istreambuf_iterator<char>(nodesFile), std::istreambuf_iterator<char>());
    std::stringstream stream;
    stream.str(streamSrc);

    std::string token = "";

    // Node count
    std::getline(stream, token, ' ');
    uint32_t totalNodeCount = std::stoi(token.data());

    // Curve node count
    std::getline(stream, token, ' ');
    uint32_t curveNodeCount = std::stoi(token.data());

    // Open or closed loop?
    std::getline(stream, token, '\n');
    bLoops = std::strcmp(token.data(), "close") == 0;

    for (uint32_t i = 0; i < totalNodeCount; i++) {
        std::shared_ptr<UTrackPoint> pt = std::make_shared<UTrackPoint>();
        pt->LoadPoint(stream);
        pt->SetParentTrackName(mConfigName);

        points.push_back(pt);
    }

    return points;
}

void UTracks::UTrack::PreprocessNodes() {
    for (std::shared_ptr<UTrackPoint> pnt : mPoints) {
        if (pnt->IsCurve()) {
            mCurvePointCount++;
        }

        pnt->TrySetJunctionArgument();
    }
}

void UTracks::UTrack::SaveNodePoints(std::filesystem::path dirName) {
    std::filesystem::path gamePath(mGameFilename);
    std::filesystem::path extPath = dirName / gamePath.filename();

    PreprocessNodes();

    std::stringstream stream;
    stream.precision(2);
    stream << std::fixed;

    stream << mPoints.size() << " " << mCurvePointCount << " " << (bLoops ? "close" : "open") << std::endl;

    for (std::shared_ptr<UTrackPoint> pnt : mPoints) {
        pnt->SavePoint(stream);
    }

    std::ofstream writer(extPath.c_str());
    writer << stream.str();
    writer.close();
}
