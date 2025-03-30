#include "debug.hpp"
#include "session.hpp"
#include "space.hpp"

#include <cassert>

namespace XR
{
    Space::Space(XrSpace space)
        : mSpace(space)
    {

    }

    Space::~Space()
    {
        if (mSpace != XR_NULL_HANDLE)
            CHECK_XRCMD(xrDestroySpace(mSpace));
    }

    VR::TrackingPose Space::locate(const VR::Space& reference) const
    {
        return XR::Session::instance().locateSpace(mSpace, dynamic_cast<const XR::Space&>(reference).mSpace);
    }

    VR::TrackingPose Space::locateInWorld() const
    {
        return XR::Session::instance().locateSpaceInWorld(mSpace);
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

    XrSpace ReferenceSpace::xrSpace() const
    {
        if (!mSpace)
            recreate();
        return mSpace;
    }

    void ReferenceSpace::recreate() const
    {
        auto& session = Session::instance();
        mSpace = session.createReferenceSpace(mType, {});
        if (mRecenterTo && mRecenterTo != mType)
        {
            auto other = session.getReferenceSpace(mRecenterTo.value());
            auto tp = other->locate(*this);
            if (!!tp.status)
            {
                //float yaw = 0;
                //float pitch = 0;
                //float roll = 0;
                //Stereo::getEulerAngles(tp.pose.orientation, yaw, pitch, roll);
                //tp.pose.orientation.makeRotate(yaw, osg::Vec3f(0, 0, -1));
                tp.pose.orientation = osg::Quat();
                mSpace = session.createReferenceSpace(mType, tp.pose);
            }
        }
        mRecenter = false;
    }

    VR::TrackingPose ReferenceSpace::locate(const VR::Space& reference) const
    {
        return XR::Space::locate(reference);
    }

    VR::TrackingPose ReferenceSpace::locateInWorld() const
    {
        return XR::Space::locateInWorld();
    }

}
