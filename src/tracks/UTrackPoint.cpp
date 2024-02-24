#include "tracks/UTrackPoint.hpp"

#include <iostream>

UTracks::UTrackPoint::UTrackPoint() : mPosition(glm::zero<glm::vec3>()), mHandleA(glm::zero<glm::vec3>()),
    mHandleB(glm::zero<glm::vec3>()), mSomeScalar(0.0f), mType(ETrackPointType::Normal), bIsCurve(false) {

}

UTracks::UTrackPoint::~UTrackPoint() {

}

void UTracks::UTrackPoint::LoadPoint(std::stringstream& stream) {
    std::string token = "";

    std::getline(stream, token, ' ');
    if (token[0] == 'c') {
        bIsCurve = true;

        std::getline(stream, token, ' ');
        mHandleA.x = std::stof(token.data());
        std::getline(stream, token, ' ');
        mHandleA.y = std::stof(token.data());
        std::getline(stream, token, ' ');
        mHandleA.z = std::stof(token.data());

        std::getline(stream, token, ' ');
        mPosition.x = std::stof(token.data());
        std::getline(stream, token, ' ');
        mPosition.y = std::stof(token.data());
        std::getline(stream, token, ' ');
        mPosition.z = std::stof(token.data());

        std::getline(stream, token, ' ');
        mHandleB.x = std::stof(token.data());
        std::getline(stream, token, ' ');
        mHandleB.y = std::stof(token.data());
        std::getline(stream, token, ' ');
        mHandleB.z = std::stof(token.data());

        std::getline(stream, token, ' ');
        mSomeScalar = std::stof(token.data());
    }
    else {
        std::getline(stream, token, ' ');
        mPosition.x = std::stof(token.data());
        std::getline(stream, token, ' ');
        mPosition.y = std::stof(token.data());
        std::getline(stream, token, ' ');
        mPosition.z = std::stof(token.data());
    }

    float yTmp = mPosition.y;
    mPosition.y = mPosition.z;
    mPosition.z = -yTmp;

    mType = stream.get() - 0x30; // Subtract the value of the char '0' to get the actual value.

    if (mType == ETrackPointType::Stop_1 || mType == ETrackPointType::Stop_2 || mType == ETrackPointType::Switch) {
        std::getline(stream, token, '\n');
        mArgument = std::string(token);
    }
    else {
        std::getline(stream, token, '\n');
    }
}

void UTracks::UTrackPoint::SavePoint(std::stringstream& stream) {
    if (bIsCurve) {
        stream << "c ";
        stream << mHandleA.x  << " " << mHandleA.y  << " " << mHandleA.z  << " ";
        stream << mPosition.x << " " << mPosition.y << " " << mPosition.z << " ";
        stream << mHandleB.x  << " " << mHandleB.y  << " " << mHandleB.z  << " ";
        stream << mSomeScalar;
    }
    else {
        stream << mPosition.x << " " << mPosition.y << " " << mPosition.z << " ";
    }

    stream << mType << " ";

    if (mType == ETrackPointType::Stop_1 || mType == ETrackPointType::Stop_2 || mType == ETrackPointType::Switch) {
        stream << mArgument << "\n";
    }
    else {
        stream << "\n";
    }
}
