#ifndef VR_UTIL_HPP
#define VR_UTIL_HPP

#include "../mwworld/ptr.hpp"

#include <components/stereo/stereomanager.hpp>
#include <components/vr/vr.hpp>

namespace MWRender
{
    struct RayResult;
}

namespace MWVR
{
    class VRAnimation;

    namespace Util
    {
        std::pair<MWWorld::Ptr, float> getPointerTarget();
        std::pair<MWWorld::Ptr, float> getTouchTarget();
        std::pair<MWWorld::Ptr, float> getWeaponTarget();
        float getPoseTarget(MWRender::RayResult& result, const Stereo::Pose& pose, bool allowTelekinesis, bool ignore3DUI = true);
        Stereo::Pose getNodePose(const osg::Node* node);
        VR::TrackingPose getXrPose(const std::string& path);
        VRAnimation* getPlayerAnimation();
    }

}

#endif
