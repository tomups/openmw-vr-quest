#ifndef VR_UTIL_HPP
#define VR_UTIL_HPP

#include "../mwworld/ptr.hpp"

#include "vrtypes.hpp"

namespace MWRender
{
    struct RayResult;
}

namespace MWVR
{
    namespace Util {
        std::pair<MWWorld::Ptr, float> getTouchTarget();
        std::pair<MWWorld::Ptr, float> getWeaponTarget();
        float getPoseTarget(MWRender::RayResult& result, const Pose& pose, bool allowTelekinesis);
        Pose getNodePose(const osg::Node* node);
    }

}

#endif
