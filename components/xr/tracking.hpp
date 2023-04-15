#ifndef OPENXR_TRACKER_HPP
#define OPENXR_TRACKER_HPP

#include <components/stereo/stereomanager.hpp>
#include <components/vr/trackingsource.hpp>
#include <openxr/openxr.h>

#include <array>
#include <map>

namespace XR
{
    class ActionSet;

    //! Serves as a C++ wrapper of openxr spaces
    //! Provides tracking of the following paths:
    //!     /stage/user/head/input/pose
    //!     /stage/user/hand/left/input/aim/pose
    //!     /stage/user/hand/left/input/aim/pose
    //!
    class Tracker : public VR::TrackingSource
    {
    public:
        Tracker(VR::VRPath path, XrSpace referenceSpace);
        ~Tracker();

        //! If not NULL, this action set will be updated during updateTracking()
        void setTrackingActionSet(ActionSet* actionSet);

        void addTrackingSpace(VR::VRPath path, XrSpace space);

        void deleteTrackingSpace(VR::VRPath path);

        std::vector<VR::VRPath> listSupportedPaths() const override;

    protected:
        void updateTracking() override;
        VR::TrackingPose locate(VR::VRPath path) override;

    private:
        std::array<Stereo::View, 2> locateViews(
            VR::DisplayTime predictedDisplayTime, XrSpace reference, XrSession session);
        void update(VR::TrackingPose& pose, XrSpace space, VR::DisplayTime predictedDisplayTime);

        XrSpace mReferenceSpace;
        VR::DisplayTime mLastUpdate = 0;
        ActionSet* mTrackingActionSet = nullptr;

        std::map<VR::VRPath, std::pair<XrSpace, VR::TrackingPose>> mSpaces;
    };
}

#endif
