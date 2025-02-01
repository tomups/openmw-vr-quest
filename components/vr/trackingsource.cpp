//#include "trackingsource.hpp"
//#include "frame.hpp"
//#include "session.hpp"
//#include "trackingmanager.hpp"
//
//#include <components/debug/debuglog.hpp>
//#include <components/misc/constants.hpp>
//#include <components/vr/vr.hpp>
//
//namespace VR
//{
//
//    StageToWorldBinding::StageToWorldBinding(VRPath movementReference)
//        : mMovementReference(movementReference)
//    {
//    }
//
//    void StageToWorldBinding::setWorldOrientation(float yaw, bool adjust)
//    {
//        auto yawQuat = osg::Quat(yaw, osg::Vec3(0, 0, -1));
//        if (adjust)
//            mOrientation = yawQuat * mOrientation;
//        else
//            mOrientation = yawQuat;
//    }
//
//    void StageToWorldBinding::setEyeLevel(Stereo::Unit eyeLevel)
//    {
//        if (VR::getSeatedPlay())
//            mMovement.mZ = eyeLevel;
//        else
//            mMovement.mZ = mLastPose.pose.position.mZ;
//    }
//
//    void StageToWorldBinding::consumeMovement(const Stereo::Position& movement)
//    {
//        mMovement.mX -= movement.mX;
//        mMovement.mY -= movement.mY;
//    }
//
//    void StageToWorldBinding::recenter()
//    {
//        mMovement.mX = {};
//        mMovement.mY = {};
//    }
//
//    void StageToWorldBinding::bindPaths(VRPath worldPath, VRPath stagePath)
//    {
//        mBindings.emplace(worldPath, stagePath);
//        notifyAvailablePosesChanged();
//    }
//
//    void StageToWorldBinding::unbindPath(VRPath worldPath)
//    {
//        mBindings.erase(worldPath);
//        notifyAvailablePosesChanged();
//    }
//
//    void StageToWorldBinding::instantTransition()
//    {
//        // When the cell changes, openmw rotates the character.
//        // To make sure the player faces the same direction regardless of current orientation,
//        // compute the offset from character orientation to player orientation and reset yaw offset to this.
//        // float yaw = 0.f;
//        // float pitch = 0.f;
//        // float roll = 0.f;
//        // Stereo::getEulerAngles(mLastPose.pose.orientation, yaw, pitch, roll);
//        // setWorldOrientation(-yaw, false);
//        mInstantTransition = true;
//    }
//
//    TrackingPose StageToWorldBinding::locate(VRPath path)
//    {
//        updateTracking();
//
//        auto it = mBindings.find(path);
//        if (it == mBindings.end())
//        {
//            Log(Debug::Error) << "Tried to locate invalid path " << path
//                              << ". This is a developer error. Please locate using TrackingManager::locate() only.";
//            throw std::logic_error("Invalid Argument");
//        }
//
//        auto stagePose = TrackingManager::instance().locate(it->second);
//
//        auto worldPose = stagePose;
//        worldPose.pose.position -= mLastPose.pose.position;
//        worldPose.pose.position *= mOrientation;
//        worldPose.pose.position += mMovement;
//        worldPose.pose.orientation = worldPose.pose.orientation * mOrientation;
//
//        if (VR::getStandingPlay())
//        {
//            auto heightAdjustment = mLastPose.pose.position.mZ * (Session::instance().playerScale() - 1.f);
//            worldPose.pose.position.mZ += heightAdjustment;
//        }
//
//        if (mOrigin)
//            worldPose.pose.position += mOriginWorldPose.position;
//
//        worldPose.pose.position.mZ += Session::instance().getSneakOffset();
//
//        return worldPose;
//    }
//
//    std::vector<VRPath> StageToWorldBinding::listSupportedPaths() const
//    {
//        std::vector<VRPath> paths;
//        for (auto pose : mBindings)
//            paths.emplace_back(pose.first);
//        return paths;
//    }
//
//    void StageToWorldBinding::updateTracking()
//    {
//        if (VR::getPredictedDisplayTime() == mLastPose.time)
//            return;
//
//        mOriginWorldPose = Stereo::Pose();
//        if (mOrigin)
//        {
//            auto worldMatrix = osg::computeLocalToWorld(mOrigin->getParentalNodePaths()[0]);
//            mOriginWorldPose.position = Stereo::Position::fromMWUnits(worldMatrix.getTrans());
//            mOriginWorldPose.orientation = worldMatrix.getRotate();
//        }
//
//        if (mInstantTransition)
//        {
//            float yawWorld = 0.f;
//            float yawStage = 0.f;
//            float pitch = 0.f;
//            float roll = 0.f;
//            Stereo::getEulerAngles(mOriginWorldPose.orientation, yawWorld, pitch, roll);
//            Stereo::getEulerAngles(mLastPose.pose.orientation, yawStage, pitch, roll);
//            setWorldOrientation(yawWorld - yawStage, false);
//            mInstantTransition = false;
//        }
//
//        auto mtp = TrackingManager::instance().locate(mMovementReference);
//        if (!!mtp.status)
//        {
//            auto vrMovement = mtp.pose.position - mLastPose.pose.position;
//            mLastPose = mtp;
//            if (mHasTrackingData)
//                mMovement += mOrientation * vrMovement;
//            else
//                mMovement.mZ = mLastPose.pose.position.mZ;
//            mHasTrackingData = true;
//        }
//    }
//}
