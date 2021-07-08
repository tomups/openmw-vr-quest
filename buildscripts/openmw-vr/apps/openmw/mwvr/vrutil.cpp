#include "vrutil.hpp"
#include "vrenvironment.hpp"
#include "vrtracking.hpp"
#include "vranimation.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwrender/renderingmanager.hpp"

#include "osg/Transform"

namespace MWVR
{
    namespace Util
    {
        std::pair<MWWorld::Ptr, osg::Vec3f> getHitContact(float distance, std::vector<MWWorld::Ptr>& targets)
        {
            return std::pair<MWWorld::Ptr, osg::Vec3f>();
        }

        std::pair<MWWorld::Ptr, float> getTouchTarget()
        {
            MWRender::RayResult result;
            auto* tm = Environment::get().getTrackingManager();
            VRPath rightHandPath = tm->stringToVRPath("/user/hand/right/input/aim/pose");
            auto* source = tm->getSource("pcworld");
            auto distance = getPoseTarget(result, source->getTrackingPose(0, rightHandPath).pose, true);
            return std::pair<MWWorld::Ptr, float>(result.mHitObject, distance);
        }

        std::pair<MWWorld::Ptr, float> getWeaponTarget()
        {
            auto* anim = MWVR::Environment::get().getPlayerAnimation();

            MWRender::RayResult result;
            auto distance = getPoseTarget(result, getNodePose(anim->getNode("weapon bone")), false);
            return std::pair<MWWorld::Ptr, float>(result.mHitObject, distance);
        }

        float getPoseTarget(MWRender::RayResult& result, const Pose& pose, bool allowTelekinesis)
        {
            auto* wm = MWBase::Environment::get().getWindowManager();
            auto* world = MWBase::Environment::get().getWorld();

            if (wm->isGuiMode() && wm->isConsoleMode())
                return world->getTargetObject(result, pose.position, pose.orientation, world->getMaxActivationDistance() * 50, true);
            else
            {
                float activationDistance = 0.f;
                if (allowTelekinesis)
                    activationDistance = world->getActivationDistancePlusTelekinesis();
                else
                    activationDistance = world->getMaxActivationDistance();

                auto distance = world->getTargetObject(result, pose.position, pose.orientation, world->getMaxActivationDistance(), true);

                if (!result.mHitObject.isEmpty() && !result.mHitObject.getClass().allowTelekinesis(result.mHitObject)
                    && distance > world->getMaxActivationDistance() && !MWBase::Environment::get().getWindowManager()->isGuiMode())
                {
                    result.mHit = false;
                    result.mHitObject = nullptr;
                    distance = 0.f;
                };
                return distance;
            }
        }

        Pose getNodePose(const osg::Node* node)
        {
            osg::Matrix worldMatrix = osg::computeLocalToWorld(node->getParentalNodePaths()[0]);
            Pose pose;
            pose.position = worldMatrix.getTrans();
            pose.orientation = worldMatrix.getRotate();
            return pose;
        }
    }
}
