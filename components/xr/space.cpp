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
        CHECK_XRCMD(xrDestroySpace(mSpace));
    }

    VR::TrackingPose Space::locate(const std::shared_ptr<VR::Space>& reference) const
    {
        return XR::Session::instance().locateSpace(mSpace, static_cast<const Space*>(reference.get())->mSpace);
    }

    VR::TrackingPose Space::locateInWorld() const
    {
        return XR::Session::instance().locateSpaceInWorld(mSpace);
    }
}
