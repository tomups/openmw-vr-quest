#include "trackingmanager.hpp"
#include "frame.hpp"
#include "trackinglistener.hpp"
#include "trackingsource.hpp"

#include <components/debug/debuglog.hpp>
#include <components/settings/settings.hpp>

#include <cassert>

namespace VR
{
    TrackingManager* sManager = nullptr;

    TrackingManager& TrackingManager::instance()
    {
        assert(sManager);
        return *sManager;
    }

    TrackingManager::TrackingManager()
    {
        if (!sManager)
            sManager = this;
        else
            throw std::logic_error("Duplicated VR::TrackingManager singleton");
    }

    TrackingManager::~TrackingManager()
    {
        sManager = nullptr;
    }

    void TrackingManager::addListener(TrackingListener* listener)
    {
        mListeners.push_back(listener);
    }

    void TrackingManager::removeListener(TrackingListener* listener)
    {
        for (auto it = mListeners.begin(); it != mListeners.end();)
            if (*it == listener)
                it = mListeners.erase(it);
            else
                it++;
    }

    void TrackingManager::processChangedSettings(const std::set<std::pair<std::string, std::string>>& changed)
    {
    }

    TrackingPose TrackingManager::locate(VRPath path) const
    {
        auto it = mPathToSourceMap.find(path);
        if (it != mPathToSourceMap.end())
            return it->second->locate(path);

        TrackingPose pose = TrackingPose();
        pose.status = TrackingStatus::NotTracked;
        return pose;
    }

    TrackingSource* TrackingManager::getTrackingSource(VRPath path) const
    {
        for (auto* source : mSources)
            if (source->path() == path)
                return source;
        return nullptr;
    }

    const std::set<VRPath>& TrackingManager::availablePaths() const
    {
        return mAvailablePaths;
    }

    void TrackingManager::registerTrackingSource(TrackingSource* source)
    {
        mSources.emplace_back(source);
    }

    void TrackingManager::unregisterTrackingSource(TrackingSource* source)
    {
        for (auto it = mSources.begin(); it != mSources.end(); it++)
        {
            if (*it == source)
            {
                it = mSources.erase(it);
            }

            if (it == mSources.end())
                break;
        }
    }

    void TrackingManager::updateTracking()
    {
        if (VR::getPredictedDisplayTime()  == 0)
            // Can't update
            return;

        checkAvailablePathsChanged();

        for (auto* source : mSources)
            source->updateTracking();

        for (auto* listener : mListeners)
            listener->onTrackingUpdated(*this);
    }

    void TrackingManager::checkAvailablePathsChanged()
    {
        bool availablePosesChanged = false;

        for (auto source : mSources)
            availablePosesChanged |= source->availablePosesChanged();

        if (availablePosesChanged)
            updateAvailablePaths();

        for (auto source : mSources)
            source->clearAvailablePosesChanged();
    }

    void TrackingManager::updateAvailablePaths()
    {
        mAvailablePaths.clear();
        mPathToSourceMap.clear();
        for (auto source : mSources)
        {
            auto paths = source->listSupportedPaths();
            mAvailablePaths.insert(paths.begin(), paths.end());
            for (auto& path : paths)
            {
                mPathToSourceMap.emplace(path, source);
            }
        }
        for (auto* listener : mListeners)
            listener->onAvailablePathsChanged(mAvailablePaths);
    }

}
