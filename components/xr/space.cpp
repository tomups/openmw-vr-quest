#include "debug.hpp"
#include "session.hpp"
#include "space.hpp"

#include <cassert>

namespace XR
{
    XrSpaceWrapper::~XrSpaceWrapper() 
    {
        if (mSpace != XR_NULL_HANDLE)
            CHECK_XRCMD(xrDestroySpace(mSpace));
    }

    void XrSpaceWrapper::operator=(XrSpace space) 
    {
        if (mSpace != XR_NULL_HANDLE)
            CHECK_XRCMD(xrDestroySpace(mSpace));
        mSpace = space;
    }

    Space::Space(XrSpace space)
        : mSpace(space)
    {

    }

    Space::~Space()
    {
    }

    VR::TrackingPose Space::locate(VR::Space& reference)
    {
        return XR::Session::instance().locateSpace(xrSpace(), dynamic_cast<XR::Space&>(reference).xrSpace());
    }

    VR::TrackingPose Space::locateInWorld()
    {
        return XR::Session::instance().locateSpaceInWorld(xrSpace());
    }

    ReferenceSpace::ReferenceSpace(VR::ReferenceSpace type, std::optional<VR::ReferenceSpace> recenterTo)
        : Space(XR_NULL_HANDLE)
        , mType(type)
        , mRecenterTo(recenterTo)
        , mRecenter(true)
    {
    }

    ReferenceSpace::~ReferenceSpace()
    {
    }

    XrSpace ReferenceSpace::xrSpace()
    {
        if (!mSpace.mSpace)
            recreate();
        return mSpace.mSpace;
    }

    void ReferenceSpace::recreate()
    {
        recenter(true, true, true);
    }

    void ReferenceSpace::recenter(bool recenterX, bool recenterY, bool recenterZ) 
    {
        auto& session = Session::instance();
        mSpace = session.createReferenceSpace(mType, {});
        if (mRecenterTo && mRecenterTo != mType)
        {
            auto other = session.getReferenceSpace(mRecenterTo.value());
            auto tp = other->locate(*this);
            if (!!tp.status)
            {
                if (recenterX)
                    mCenter.position.mX = tp.pose.position.mX;
                if (recenterY)
                    mCenter.position.mY = tp.pose.position.mY;
                if (recenterZ)
                    mCenter.position.mZ = tp.pose.position.mZ;
                mSpace = session.createReferenceSpace(mType, mCenter);
            }
        }
        mRecenter = false;
    }

}
