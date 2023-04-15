#ifndef VR_H
#define VR_H

#include <components/stereo/types.hpp>
#include <components/vr/constants.hpp>
#include <stdint.h>

namespace VR
{

    //! @brief An identifier representing an path, with 0 representing no path.
    //! A VRPath can be optained from the string representation of a path using VRTrackingManager::stringToVRPath()
    //! \note Support is determined by each VRTrackingSource. ALL strings are convertible to VRPaths but won't be useful
    //! unless they match a supported string. \sa VRTrackingManager::getTrackingPath()
    using VRPath = uint32_t;

    //! Display time as a 64bit integer. Units and reference are undefined, value is as provided by the VR runtime.
    using DisplayTime = int64_t;

    //! A single tracked pose
    struct TrackingPose
    {
        TrackingStatus status = TrackingStatus::Unknown; //!< State of the prediction.
        Stereo::Pose pose = {}; //!< The predicted pose.
        DisplayTime time = 0; //!< The time for which the pose was predicted.
    };

    //! Converts a string representation of a path to a VRTrackerPath identifier
    VRPath stringToVRPath(std::string_view path);

    //! Converts a path identifier back to string. Returns an empty string if no such identifier exists.
    std::string_view VRPathToString(VRPath path);


    bool getVR();
    bool getKBMouseModeActive();
    bool getLeftControllerActive();
    bool getRightControllerActive();
    bool getSteamVR();
    bool getSeatedPlay();
    bool getStandingPlay();
    TrackingPose getTrackedPose(VRPath path);
    Stereo::Unit getPlayerHeight();
    DisplayTime getPredictedDisplayTime();
    DisplayTime getPredictedDisplayPeriod();

    void setVR(bool VR);
    void setLeftControllerActive(bool active);
    void setRightControllerActive(bool active);
    void setSteamVR(bool steamVR);
    void setSeatedPlay(bool seated);
    void setSneakOffsetEnabled(bool enabled);
    void setPredictedDisplayTime(DisplayTime time);
    void setPredictedDisplayPeriod(DisplayTime time);

    void recenter();
    void resetEyeLevel();
}

#endif
