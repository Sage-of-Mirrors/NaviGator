#include "tracks/UTrack.hpp"
#include "tracks/UTrackPoint.hpp"

#include <pugixml.hpp>

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>

UTracks::UTrack::UTrack() : mGameFilename(""), mConfigName(""), bStopsAtStations(false), mBrakingDist(10)
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

bool UTracks::UTrack::LoadNodePoints(std::filesystem::path dirName) {
    std::filesystem::path gamePath(mGameFilename);
    std::filesystem::path extPath = dirName / gamePath.filename();

    if (!std::filesystem::exists(extPath)) {
        return false;
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
    bOpen = std::strcmp(token.data(), "open") == 0;

    for (uint32_t i = 0; i < totalNodeCount; i++) {
        std::shared_ptr<UTrackPoint> pt = std::make_shared<UTrackPoint>();
        pt->LoadPoint(stream);

        mPoints.push_back(pt);
    }
}

void UTracks::UTrack::SaveNodePoints(std::filesystem::path dirName) {

}
