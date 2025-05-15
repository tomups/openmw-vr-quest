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
        VRAnimation* getPlayerAnimation();

        template <typename T>
        std::optional<T> pickByHandedness(bool leftAvailable, bool rightAvailable, const T& l, const T& r)
        {
            leftAvailable = leftAvailable && VR::getLeftControllerActive();
            rightAvailable = rightAvailable && VR::getRightControllerActive();
            if (VR::getLeftHandedMode() && leftAvailable)
                return l;
            if (rightAvailable)
                return r;
            if (leftAvailable)
                return l;
            return std::nullopt;
        }

        template <typename T>
        const T& pickByHandedness(const T& l, const T& r)
        {
            if (VR::getLeftHandedMode() && VR::getLeftControllerActive())
                return l;
            if (VR::getRightControllerActive())
                return r;
            return l;
        }
    }

}

#endif
