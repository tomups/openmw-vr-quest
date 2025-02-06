#ifndef VR_H
#define VR_H

#include <components/stereo/types.hpp>
#include <components/vr/constants.hpp>
#include <stdint.h>
#include <openxr/openxr.h>

namespace VR
{

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
    XrPath stringToXrPath(const std::string& path);

    //! Converts a path identifier back to string. Returns an empty string if no such identifier exists.
    std::string xrPathToString(XrPath path);

    bool getVR();
    bool getKBMouseModeActive();
    bool getSteamVR();
    bool getSeatedPlay();
    bool getStandingPlay();
    bool getControllerActive(XrPath controllerPath);
    XrPath getControllerInteractionProfile(XrPath controllerPath);
    bool getLeftControllerActive();
    bool getRightControllerActive();
    Stereo::Unit getPlayerHeight();
    DisplayTime getPredictedDisplayTime();
    DisplayTime getPredictedDisplayPeriod();
    std::string getRuntimeName();

    void setVR(bool VR);
    void setControllerActive(XrPath controllerPath, XrPath interactionProfilePath, bool active);
    void setSteamVR(bool steamVR);
    void setSeatedPlay(bool seated);
    void setSneakOffsetEnabled(bool enabled);
    void setPredictedDisplayTime(DisplayTime time);
    void setPredictedDisplayPeriod(DisplayTime time);
    void setRuntimeName(std::string name);

    void recenter();
    void resetEyeLevel();
}

#endif
