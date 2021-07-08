#include "openxrinput.hpp"
#include "openxrmanagerimpl.hpp"
#include "openxrplatform.hpp"
#include "openxrtracker.hpp"
#include "openxrtypeconversions.hpp"
#include "vrenvironment.hpp"
#include "vrinputmanager.hpp"
#include "vrsession.hpp"

#include <components/misc/constants.hpp>

namespace MWVR
{
    OpenXRTracker::OpenXRTracker(const std::string& name, XrSpace referenceSpace)
        : VRTrackingSource(name)
        , mReferenceSpace(referenceSpace)
        , mTrackingSpaces()
    {

    }

    OpenXRTracker::~OpenXRTracker()
    {
    }

    void OpenXRTracker::addTrackingSpace(VRPath path, XrSpace space)
    {
        mTrackingSpaces[path] = space;
        notifyAvailablePosesChanged();
    }

    void OpenXRTracker::deleteTrackingSpace(VRPath path)
    {
        mTrackingSpaces.erase(path);
        notifyAvailablePosesChanged();
    }

    void OpenXRTracker::setReferenceSpace(XrSpace referenceSpace)
    {
        mReferenceSpace = referenceSpace;
    }

    std::vector<VRPath> OpenXRTracker::listSupportedTrackingPosePaths() const
    {
        std::vector<VRPath> path;
        for (auto& e : mTrackingSpaces)
            path.push_back(e.first);
        return path;
    }

    void OpenXRTracker::updateTracking(DisplayTime predictedDisplayTime)
    {
        Environment::get().getInputManager()->xrInput().getActionSet(ActionSet::Tracking).updateControls();
        auto* xr = Environment::get().getManager();
        auto* session = Environment::get().getSession();

        auto& frame = session->getFrame(VRSession::FramePhase::Update);
        frame->mViews[(int)ReferenceSpace::STAGE] = locateViews(predictedDisplayTime, xr->impl().getReferenceSpace(ReferenceSpace::STAGE));
        frame->mViews[(int)ReferenceSpace::VIEW] = locateViews(predictedDisplayTime, xr->impl().getReferenceSpace(ReferenceSpace::VIEW));
    }

    XrSpace OpenXRTracker::getSpace(VRPath path)
    {
        auto it = mTrackingSpaces.find(path);
        if (it != mTrackingSpaces.end())
            return it->second;
        return 0;
    }

    VRTrackingPose OpenXRTracker::getTrackingPoseImpl(DisplayTime predictedDisplayTime, VRPath path, VRPath reference)
    {
        VRTrackingPose pose;
        pose.status = TrackingStatus::Good;
        XrSpace space = getSpace(path);
        XrSpace ref = reference == 0 ? mReferenceSpace : getSpace(reference);
        if (space == 0 || ref == 0)
            pose.status = TrackingStatus::NotTracked;
        if (!!pose.status)
            locate(pose, space, ref, predictedDisplayTime);
        return pose;
    }

    void OpenXRTracker::locate(VRTrackingPose& pose, XrSpace space, XrSpace reference, DisplayTime predictedDisplayTime)
    {
        XrSpaceLocation location{ XR_TYPE_SPACE_LOCATION };
        auto res = xrLocateSpace(space, mReferenceSpace, predictedDisplayTime, &location);

        if (XR_FAILED(res))
        {
            // Call failed, exit.
            CHECK_XRRESULT(res, "xrLocateSpace");
            pose.status = TrackingStatus::RuntimeFailure;
            return;
        }

        // Check that everything is being tracked
        if (!(location.locationFlags & (XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT | XR_SPACE_LOCATION_POSITION_TRACKED_BIT)))
        {
            // It's not, data is stale
            pose.status = TrackingStatus::Stale;
        }

        // Check that data is valid
        if (!(location.locationFlags & (XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_POSITION_VALID_BIT)))
        {
            // It's not, we've lost tracking
            pose.status = TrackingStatus::Lost;
        }

        pose.pose = MWVR::Pose{
            fromXR(location.pose.position),
            fromXR(location.pose.orientation)
        };
    }

    std::array<View, 2> OpenXRTracker::locateViews(DisplayTime predictedDisplayTime, XrSpace reference)
    {
        std::array<XrView, 2> xrViews{ {{XR_TYPE_VIEW}, {XR_TYPE_VIEW}} };
        XrViewState viewState{ XR_TYPE_VIEW_STATE };
        uint32_t viewCount = 2;

        XrViewLocateInfo viewLocateInfo{ XR_TYPE_VIEW_LOCATE_INFO };
        viewLocateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        viewLocateInfo.displayTime = predictedDisplayTime;
        viewLocateInfo.space = reference;

        auto* xr = Environment::get().getManager();
        CHECK_XRCMD(xrLocateViews(xr->impl().xrSession(), &viewLocateInfo, &viewState, viewCount, &viewCount, xrViews.data()));

        std::array<View, 2> vrViews{};
        vrViews[(int)Side::LEFT_SIDE].pose = fromXR(xrViews[(int)Side::LEFT_SIDE].pose);
        vrViews[(int)Side::RIGHT_SIDE].pose = fromXR(xrViews[(int)Side::RIGHT_SIDE].pose);
        vrViews[(int)Side::LEFT_SIDE].fov = fromXR(xrViews[(int)Side::LEFT_SIDE].fov);
        vrViews[(int)Side::RIGHT_SIDE].fov = fromXR(xrViews[(int)Side::RIGHT_SIDE].fov);
        return vrViews;
    }

    OpenXRTrackingToWorldBinding::OpenXRTrackingToWorldBinding()
    {
    }
}
