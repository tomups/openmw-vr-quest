#ifndef VR_TRACKING_MANAGER_H
#define VR_TRACKING_MANAGER_H

#include <components/vr/vr.hpp>

#include <list>
#include <map>
#include <set>
#include <vector>

namespace VR
{
    class TrackingListener;
    class TrackingSource;
    struct Frame;

    class TrackingManager
    {
    public:
        static TrackingManager& instance();

        TrackingManager();
        ~TrackingManager();

        void updateTracking();

        //! Bind listener to source, listener will receive tracking updates from source until unbound.
        //! \note A single listener can only receive tracking updates from one source.
        void addListener(TrackingListener* listener);

        //! Unbind listener, listener will no longer receive tracking updates.
        void removeListener(TrackingListener* listener);

        void processChangedSettings(const std::set<std::pair<std::string, std::string>>& changed);

        TrackingPose locate(VRPath path) const;

        TrackingSource* getTrackingSource(VRPath path) const;

        const std::set<VRPath>& availablePaths() const;

    private:
        friend class TrackingSource;
        void registerTrackingSource(TrackingSource* source);
        void unregisterTrackingSource(TrackingSource* source);
        void checkAvailablePathsChanged();
        void updateAvailablePaths();

        std::vector<TrackingSource*> mSources;
        std::map<VRPath, TrackingSource*> mPathToSourceMap;
        std::set<VRPath> mAvailablePaths;
        std::list<TrackingListener*> mListeners;
    };
}

#endif
