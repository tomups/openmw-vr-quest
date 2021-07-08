#include "vrtracking.hpp"
#include "vrenvironment.hpp"
#include "vrsession.hpp"
#include "openxrmanagerimpl.hpp"

#include <components/misc/constants.hpp>

#include <mutex>

namespace MWVR
{
    VRTrackingManager::VRTrackingManager()
    {
        mHandDirectedMovement = Settings::Manager::getBool("hand directed movement", "VR");
        mHeadPath = stringToVRPath("/user/head/input/pose");
        mHandPath = stringToVRPath("/user/hand/left/input/aim/pose");
    }

    VRTrackingManager::~VRTrackingManager()
    {
    }

    void VRTrackingManager::registerTrackingSource(VRTrackingSource* source, const std::string& name)
    {
        mSources.emplace(name, source);
        notifySourceChanged(name);
    }

    void VRTrackingManager::unregisterTrackingSource(VRTrackingSource* source)
    {
        std::string name = "";
        {
            auto it = mSources.begin();
            while (it->second != source) it++;
            name = it->first;
            mSources.erase(it);
        }
        notifySourceChanged(name);
    }

    VRTrackingSource* VRTrackingManager::getSource(const std::string& name)
    {
        auto it = mSources.find(name);
        if (it != mSources.end())
            return it->second;
        return nullptr;
    }

    void VRTrackingManager::movementAngles(float& yaw, float& pitch)
    {
        yaw = mMovementYaw;
        pitch = mMovementPitch;
    }

    void VRTrackingManager::processChangedSettings(const std::set<std::pair<std::string, std::string>>& changed)
    {
        mHandDirectedMovement = Settings::Manager::getBool("hand directed movement", "VR");
    }

    void VRTrackingManager::notifySourceChanged(const std::string& name)
    {
        auto* source = getSource(name);
        for (auto& it : mBindings)
        {
            if (it.second == name)
            {
                if (source)
                    it.first->onTrackingAttached(*source);
                else
                    it.first->onTrackingDetached();
            }
        }
    }

    void VRTrackingManager::updateMovementAngles(DisplayTime predictedDisplayTime)
    {
        if (mHandDirectedMovement)
        {
            float headYaw = 0.f;
            float headPitch = 0.f;
            float headsWillRoll = 0.f;

            float handYaw = 0.f;
            float handPitch = 0.f;
            float handRoll = 0.f;

            auto pcsource = getSource("pcstage");

            if (pcsource)
            {
                auto tpHead = pcsource->getTrackingPose(predictedDisplayTime, mHeadPath);
                auto tpHand = pcsource->getTrackingPose(predictedDisplayTime, mHandPath);

                if (!!tpHead.status && !!tpHand.status)
                {
                    getEulerAngles(tpHead.pose.orientation, headYaw, headPitch, headsWillRoll);
                    getEulerAngles(tpHand.pose.orientation, handYaw, handPitch, handRoll);

                    mMovementYaw = handYaw - headYaw;
                    mMovementPitch = handPitch - headPitch;
                }
            }
        }
        else
        {
            mMovementYaw = 0;
            mMovementPitch = 0;
        }
    }

    void VRTrackingManager::bind(VRTrackingListener* listener, std::string sourceName)
    {
        unbind(listener);
        mBindings.emplace(listener, sourceName);
        
        auto* source = getSource(sourceName);
        if (source)
            listener->onTrackingAttached(*source);
        else
            listener->onTrackingDetached();
    }

    void VRTrackingManager::unbind(VRTrackingListener* listener)
    {
        auto it = mBindings.find(listener);
        if (it != mBindings.end())
        {
            listener->onTrackingDetached();
            mBindings.erase(listener);
        }
    }

    VRPath VRTrackingManager::stringToVRPath(const std::string& path)
    {
        // Empty path is invalid
        if (path.empty())
        {
            Log(Debug::Error) << "Empty path";
            return 0;
        }

        // Return path immediately if it already exists
        auto it = mPathIdentifiers.find(path);
        if (it != mPathIdentifiers.end())
            return it->second;

        // Add new path and return it
        auto res = mPathIdentifiers.emplace(path, mPathIdentifiers.size() + 1);
        return res.first->second;
    }

    std::string VRTrackingManager::VRPathToString(VRPath path)
    {
        // Find the identifier in the map and return the corresponding string.
        for (auto& e : mPathIdentifiers)
            if (e.second == path)
                return e.first;

        // No path found, return empty string
        Log(Debug::Warning) << "No such path identifier (" << path << ")";
        return "";
    }

    void VRTrackingManager::updateTracking()
    {
        MWVR::Environment::get().getSession()->endFrame();
        MWVR::Environment::get().getSession()->beginFrame();
        auto& frame = Environment::get().getSession()->getFrame(VRSession::FramePhase::Update);

        if (frame->mFrameInfo.runtimePredictedDisplayTime == 0)
            return;

        for (auto source : mSources)
            source.second->updateTracking(frame->mFrameInfo.runtimePredictedDisplayTime);

        updateMovementAngles(frame->mFrameInfo.runtimePredictedDisplayTime);

        for (auto& binding : mBindings)
        {
            auto* listener = binding.first;
            auto* source = getSource(binding.second);
            if (source)
            {
                if (source->availablePosesChanged())
                    listener->onAvailablePosesChanged(*source);
                listener->onTrackingUpdated(*source, frame->mFrameInfo.runtimePredictedDisplayTime);
            }
        }

        for (auto source : mSources)
            source.second->clearAvailablePosesChanged();
    }

    VRTrackingSource::VRTrackingSource(const std::string& name)
    {
        Environment::get().getTrackingManager()->registerTrackingSource(this, name);
    }

    VRTrackingSource::~VRTrackingSource()
    {
        Environment::get().getTrackingManager()->unregisterTrackingSource(this);
    }

    VRTrackingPose VRTrackingSource::getTrackingPose(DisplayTime predictedDisplayTime, VRPath path, VRPath reference)
    {
        auto it = mCache.find(std::pair(path, reference));
        
        if (it == mCache.end())
        {
            mCache[std::pair(path, reference)] = getTrackingPoseImpl(predictedDisplayTime, path, reference);
            mCache[std::pair(path, reference)].time = predictedDisplayTime;
        }

        if (predictedDisplayTime <= it->second.time)
            return it->second;

        auto tp = getTrackingPoseImpl(predictedDisplayTime, path, reference);
        tp.time = predictedDisplayTime;
        if (!tp.status)
            tp.pose = it->second.pose;
        it->second = tp;

        return tp;
    }

    bool VRTrackingSource::availablePosesChanged() const
    {
        return mAvailablePosesChanged;
    }

    void VRTrackingSource::clearAvailablePosesChanged()
    {
        mAvailablePosesChanged = false;
    }

    void VRTrackingSource::clearCache()
    {
        mCache.clear();
    }

    void VRTrackingSource::notifyAvailablePosesChanged()
    {
        mAvailablePosesChanged = true;
    }

    VRTrackingListener::~VRTrackingListener()
    {
        Environment::get().getTrackingManager()->unbind(this);
    }

    VRTrackingToWorldBinding::VRTrackingToWorldBinding(const std::string& name, VRTrackingSource* source, VRPath movementReference)
        : VRTrackingSource(name)
        , mMovementReference(movementReference)
        , mSource(source)
    {
    }


    void VRTrackingToWorldBinding::setWorldOrientation(float yaw, bool adjust)
    {
        auto yawQuat = osg::Quat(yaw, osg::Vec3(0, 0, -1));
        if (adjust)
            mOrientation = yawQuat * mOrientation;
        else
            mOrientation = yawQuat;
    }

    osg::Vec3 VRTrackingToWorldBinding::movement() const
    {
        return mMovement;
    }

    void VRTrackingToWorldBinding::consumeMovement(const osg::Vec3& movement)
    {
        mMovement.x() -= movement.x();
        mMovement.y() -= movement.y();
    }

    void VRTrackingToWorldBinding::recenter(bool resetZ)
    {
        mMovement.x() = 0;
        mMovement.y() = 0;
        if (resetZ)
        {
            if (mSeatedPlay)
                mMovement.z() = mEyeLevel;
            else
                mMovement.z() = mLastPose.position.z();
        }
    }

    VRTrackingPose VRTrackingToWorldBinding::getTrackingPoseImpl(DisplayTime predictedDisplayTime, VRPath path, VRPath reference)
    {
        auto tp = mSource->getTrackingPose(predictedDisplayTime, path, reference);
        tp.pose.position *= Constants::UnitsPerMeter;

        if (reference == 0 && !!tp.status)
        {
            tp.pose.position -= mLastPose.position;
            tp.pose.position = mOrientation * tp.pose.position;
            tp.pose.position += mMovement;
            tp.pose.orientation = tp.pose.orientation * mOrientation;

            if(mOrigin)
                tp.pose.position += mOriginWorldPose.position;
        }
        return tp;
    }

    std::vector<VRPath> VRTrackingToWorldBinding::listSupportedTrackingPosePaths() const
    {
        return mSource->listSupportedTrackingPosePaths();
    }

    void VRTrackingToWorldBinding::updateTracking(DisplayTime predictedDisplayTime)
    {
        mOriginWorldPose = Pose();
        if (mOrigin)
        {
            auto worldMatrix = osg::computeLocalToWorld(mOrigin->getParentalNodePaths()[0]);
            mOriginWorldPose.position = worldMatrix.getTrans();
            mOriginWorldPose.orientation = worldMatrix.getRotate();
        }

        auto mtp = mSource->getTrackingPose(predictedDisplayTime, mMovementReference, 0);
        if (!!mtp.status)
        {
            mtp.pose.position *= Constants::UnitsPerMeter;
            osg::Vec3 vrMovement = mtp.pose.position - mLastPose.position;
            mLastPose = mtp.pose;
            if (mHasTrackingData)
                mMovement += mOrientation * vrMovement;
            else
                mMovement.z() = mLastPose.position.z();
            mHasTrackingData = true;
        }

        mAvailablePosesChanged = mSource->availablePosesChanged();
    }
}


