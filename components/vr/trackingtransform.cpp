#include "trackingmanager.hpp"
#include "trackingsource.hpp"

#include "trackingtransform.hpp"
#include <components/debug/debuglog.hpp>
#include <components/misc/constants.hpp>

namespace VR
{
    TrackingTransform::TrackingTransform(VRPath path)
        : mPath(path)
    {
    }

    void TrackingTransform::onTrackingUpdated(TrackingManager& manager)
    {
        auto pose = manager.locate(mPath);
        if (!!pose.status)
        {
            _matrix.makeIdentity();
            _matrix.preMultTranslate(pose.pose.position.asMWUnits());
            _matrix.preMultRotate(pose.pose.orientation);
            _inverseDirty = true;
        }
    }
}
