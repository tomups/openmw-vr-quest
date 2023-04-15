#include "vr.hpp"
#include "session.hpp"
#include "viewer.hpp"
#include "trackingmanager.hpp"

#include <components/debug/debuglog.hpp>
#include <components/misc/strings/algorithm.hpp>

namespace VR
{
    namespace
    {
        bool sVRMode = false;
        bool sSteamVR = false;
        bool sLeftControllerActive = false;
        bool sRightControllerActive = false;
        bool sSeatedPlay = false;
        DisplayTime sPredictedDisplayTime = 0;
        DisplayTime sPredictedDisplayPeriod = 0;
    }

    bool getVR()
    {
        return sVRMode;
    }

    bool getKBMouseModeActive()
    {
        return !(sLeftControllerActive || sRightControllerActive);
    }

    bool getLeftControllerActive()
    {
        return sLeftControllerActive;
    }

    bool getRightControllerActive()
    {
        return sRightControllerActive;
    }

    bool getSteamVR()
    {
        return sSteamVR;
    }

    bool getSeatedPlay()
    {
        return sSeatedPlay;
    }

    bool getStandingPlay()
    {
        return !sSeatedPlay;
    }

    TrackingPose getTrackedPose(VRPath path)
    {
        if (sVRMode)
            return TrackingManager::instance().locate(path);
        return TrackingPose();
    }

    Stereo::Unit getPlayerHeight()
    {
        if (sVRMode)
            return Session::instance().playerHeight();
        return Stereo::Unit();
    }

    DisplayTime getPredictedDisplayTime()
    {
        return sPredictedDisplayTime;
    }

    DisplayTime getPredictedDisplayPeriod()
    {
        return sPredictedDisplayPeriod;
    }

    void setVR(bool VR)
    {
        sVRMode = VR;
    }

    void setLeftControllerActive(bool active)
    {
        sLeftControllerActive = active;
    }

    void setRightControllerActive(bool active)
    {
        sRightControllerActive = active;
    }

    void setSteamVR(bool steamVR)
    {
        sSteamVR = steamVR;
    }

    void setSeatedPlay(bool seated)
    {
        sSeatedPlay = seated;
    }

    void setSneakOffsetEnabled(bool enabled) 
    {
        if (sVRMode)
            Session::instance().setSneak(enabled);
    }
    void setPredictedDisplayTime(DisplayTime time)
    {
        sPredictedDisplayTime = time;
    }
    void setPredictedDisplayPeriod(DisplayTime time)
    {
        sPredictedDisplayPeriod = time;
    }

    void recenter() 
    {
        if (sVRMode)
            Session::instance().recenter();
    }

    void resetEyeLevel()
    {
        if (sVRMode)
            Session::instance().resetEyeLevel();
    }

    std::map<std::string, VRPath, std::less<>> sPathIdentifiers;

    VRPath newPath(std::string_view path)
    {
        VRPath vrPath = sPathIdentifiers.size() + 1;
        auto res = sPathIdentifiers.emplace(path, vrPath);
        return res.first->second;
    }

    VRPath stringToVRPath(std::string_view path)
    {
        // Empty path is invalid
        if (path.empty())
        {
            Log(Debug::Error) << "VR::stringToVRPath: Empty path";
            return VRPath();
        }

        // Return path immediately if it already exists
        auto it = sPathIdentifiers.find(path);
        if (it != sPathIdentifiers.end())
            return it->second;

        // Add new path and return it
        return newPath(path);
    }

    std::string_view VRPathToString(VRPath path)
    {
        // Find the identifier in the map and return the corresponding string.
        for (auto& e : sPathIdentifiers)
            if (e.second == path)
                return e.first;

        // No path found, return empty string
        Log(Debug::Warning) << "VR::VRPathToString: No such path identifier (" << path << ")";
        return "";
    }
}
