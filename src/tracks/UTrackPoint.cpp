#include "tracks/UTrackPoint.hpp"
#include "util/uiutil.hpp"

#include <imgui.h>
#include <magic_enum.hpp>

#include <iostream>
#include <array>

UTracks::UTrackPoint::UTrackPoint() : mPosition(glm::zero<glm::vec3>()), mHandleA(glm::zero<glm::vec3>()),
    mHandleB(glm::zero<glm::vec3>()), mSomeScalar(0.0f), mStationType(ENodeStationType::None), bIsTunnel(false), bIsJunction(false),
    bIsCurve(false), bHighlighted(false), bSelected(false)
{

}

UTracks::UTrackPoint::~UTrackPoint() {

}

void UTracks::UTrackPoint::LoadPoint(std::stringstream& stream) {
    std::string token = "";

    std::getline(stream, token, ' ');
    if (token[0] == 'c') {
        bIsCurve = true;

        std::getline(stream, token, ' ');
        mPosition.x = std::stof(token.data());
        std::getline(stream, token, ' ');
        mPosition.y = std::stof(token.data());
        std::getline(stream, token, ' ');
        mPosition.z = std::stof(token.data());

        std::getline(stream, token, ' ');
        mHandleA.x = std::stof(token.data());
        std::getline(stream, token, ' ');
        mHandleA.y = std::stof(token.data());
        std::getline(stream, token, ' ');
        mHandleA.z = std::stof(token.data());

        std::getline(stream, token, ' ');
        mHandleB.x = std::stof(token.data());
        std::getline(stream, token, ' ');
        mHandleB.y = std::stof(token.data());
        std::getline(stream, token, ' ');
        mHandleB.z = std::stof(token.data());

        float yTmp = mPosition.y;
        mPosition.y = mPosition.z;
        mPosition.z = -yTmp;

        yTmp = mHandleA.y;
        mHandleA.y = mHandleA.z;
        mHandleA.z = -yTmp;

        yTmp = mHandleB.y;
        mHandleB.y = mHandleB.z;
        mHandleB.z = -yTmp;
    }
    else {
        mPosition.x = std::stof(token.data());
        std::getline(stream, token, ' ');
        mPosition.y = std::stof(token.data());
        std::getline(stream, token, ' ');
        mPosition.z = std::stof(token.data());

        float yTmp = mPosition.y;
        mPosition.y = mPosition.z;
        mPosition.z = -yTmp;

        mHandleA = mPosition;
        mHandleB = mPosition;
    }

    std::getline(stream, token, ' ');
    mSomeScalar = std::stof(token.data());

    uint8_t infoBits = stream.get() - 0x30; // Subtract the value of the char '0' to get the actual value.
    mStationType =  ENodeStationType(infoBits & ENodeInfoBits::BITS_STATION_TYPE);
    bIsTunnel    = (infoBits & ENodeInfoBits::BITS_IS_TUNNEL)   >> 2;
    bIsJunction  = (infoBits & ENodeInfoBits::BITS_IS_JUNCTION) >> 3;

    stream.get();

    if (bIsJunction || mStationType != ENodeStationType::None) {
        std::getline(stream, token, '\n');
        mArgument = std::string(token);
    }
}

void UTracks::UTrackPoint::SavePoint(std::stringstream& stream) {
    if (bIsCurve) {
        stream << "c ";
        stream << mPosition.x << " " << -mPosition.z << " " << mPosition.y << " ";
        stream << mHandleA.x  << " " << -mHandleA.z  << " " << mHandleA.y  << " ";
        stream << mHandleB.x  << " " << -mHandleB.z  << " " << mHandleB.y  << " ";
    }
    else {
        stream << mPosition.x << " " << -mPosition.z << " " << mPosition.y << " ";
    }

    stream << mSomeScalar << " ";

    uint8_t infoBits = 0;
    infoBits |= mStationType & ENodeInfoBits::BITS_STATION_TYPE;
    infoBits |= bIsTunnel   << 2;
    infoBits |= bIsJunction << 3;

    stream << char(infoBits + 0x30);

    if (bIsJunction || mStationType != ENodeStationType::None) {
        stream << " " << mArgument << "\n";
    }
    else {
        stream << "\n";
    }
}

void UTracks::UTrackPoint::TrySetJunctionArgument() {
    if (!bIsJunction || mJunctionPartner.expired()) {
        mArgument = "";
        return;
    }

    mArgument = mJunctionPartner.lock()->GetParentTrackName();
}

void UTracks::UTrackPoint::SetJunctionPartner(std::shared_ptr<UTrackPoint> partner) {
    if (partner == nullptr && !mJunctionPartner.expired()) {
        std::shared_ptr<UTrackPoint> partnerLocked = mJunctionPartner.lock();
        partnerLocked->BreakJunction();

        BreakJunction();
        return;
    }

    if (!bIsJunction) {
        bIsJunction = true;
    }

    mJunctionPartner = partner;
}

void UTracks::UTrackPoint::BreakJunction() {
    bIsJunction = false;
    mJunctionPartner = std::weak_ptr<UTrackPoint>();
}
