#include "tracking.hpp"
#include "actionset.hpp"
#include "debug.hpp"
#include "input.hpp"
#include "instance.hpp"
#include "typeconversion.hpp"

#include <components/misc/constants.hpp>

#include <components/vr/frame.hpp>
#include <components/vr/vr.hpp>

namespace XR
{
    Tracker::Tracker(VR::VRPath path, XrSpace referenceSpace)
        : VR::TrackingSource(path)
        , mReferenceSpace(referenceSpace)
    {
    }

    Tracker::~Tracker() {}

    void Tracker::setTrackingActionSet(ActionSet* actionSet)
    {
        mTrackingActionSet = actionSet;
    }

    void Tracker::addTrackingSpace(VR::VRPath path, XrSpace space)
    {
        mSpaces.emplace(path, std::pair(space, VR::TrackingPose()));
    }

    void Tracker::deleteTrackingSpace(VR::VRPath path)
    {
        mSpaces.erase(path);
    }

    std::vector<VR::VRPath> Tracker::listSupportedPaths() const
    {
        std::vector<VR::VRPath> paths;
        for (auto space : mSpaces)
            paths.emplace_back(space.first);
        return paths;
    }

    void Tracker::updateTracking()
    {
        if (VR::getPredictedDisplayTime() == mLastUpdate)
            return;

        mLastUpdate = VR::getPredictedDisplayTime();

        if (mTrackingActionSet)
            mTrackingActionSet->updateControls(false);

        for (auto& space : mSpaces)
        {
            update(space.second.second, space.second.first, VR::getPredictedDisplayTime());
        }
    }

    VR::TrackingPose Tracker::locate(VR::VRPath path)
    {
        updateTracking();

        auto it = mSpaces.find(path);
        if (it != mSpaces.end())
            return it->second.second;

        Log(Debug::Error) << "Tried to locate invalid path " << path
                          << ". This is a developer error. Please locate using TrackingManager::locate() only.";
        throw std::logic_error("Invalid Argument");
    }

    std::array<Stereo::View, 2> Tracker::locateViews(
        VR::DisplayTime predictedDisplayTime, XrSpace reference, XrSession session)
    {
        std::array<XrView, 2> xrViews{};
        xrViews[0].type = XR_TYPE_VIEW;
        xrViews[1].type = XR_TYPE_VIEW;

        XrViewState viewState{};
        viewState.type = XR_TYPE_VIEW_STATE;
        uint32_t viewCount = 2;

        XrViewLocateInfo viewLocateInfo{};
        viewLocateInfo.type = XR_TYPE_VIEW_LOCATE_INFO;
        viewLocateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        viewLocateInfo.displayTime = predictedDisplayTime;
        viewLocateInfo.space = reference;

        CHECK_XRCMD(xrLocateViews(session, &viewLocateInfo, &viewState, viewCount, &viewCount, xrViews.data()));

        std::array<Stereo::View, 2> vrViews{};
        for (auto side : { VR::Side_Left, VR::Side_Right })
        {
            vrViews[side].pose = fromXR(xrViews[side].pose);
            vrViews[side].fov = fromXR(xrViews[side].fov);
        }
        return vrViews;
    }

    void Tracker::update(VR::TrackingPose& pose, XrSpace space, VR::DisplayTime predictedDisplayTime)
    {
        pose.status = VR::TrackingStatus::Good;
        pose.time = predictedDisplayTime;
        XrSpaceLocation location{};
        location.type = XR_TYPE_SPACE_LOCATION;
        auto res = xrLocateSpace(space, mReferenceSpace, predictedDisplayTime, &location);

        if (XR_FAILED(res))
        {
            // Call failed, exit.
            CHECK_XRRESULT(res, "xrLocateSpace");
            pose.status = VR::TrackingStatus::RuntimeFailure;
            return;
        }

        // Check that everything is being tracked
        if (!(location.locationFlags
                & (XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT | XR_SPACE_LOCATION_POSITION_TRACKED_BIT)))
        {
            // It's not, data is stale
            pose.status = VR::TrackingStatus::Stale;
        }

        // Check that data is valid
        if (!(location.locationFlags
                & (XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_POSITION_VALID_BIT)))
        {
            // It's not, we've lost tracking
            pose.status = VR::TrackingStatus::Lost;
            return;
        }

        pose.pose = fromXR(location.pose);
    }

}
