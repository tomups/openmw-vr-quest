#include "vr.hpp"
#include "session.hpp"
#include "viewer.hpp"
#include "trackingmanager.hpp"

#include <components/debug/debuglog.hpp>
#include <components/misc/strings/algorithm.hpp>

#include <map>
#include <optional>

namespace VR
{
    namespace
    {
        bool sVRMode = false;
        bool sSteamVR = false;
        bool sSeatedPlay = false;
        DisplayTime sPredictedDisplayTime = 0;
        DisplayTime sPredictedDisplayPeriod = 0;
        std::map<VRPath, VRPath> sActiveControllers = {};
        std::string sRuntimeName = "UNINITIALIZED";

        std::map<std::string, VRPath, std::less<>> sPathIdentifiers;

        namespace Paths
        {
            // Cache recurring path values for this source file
            static const VRPath lctrl = stringToVRPath("/user/hand/left");
            static const VRPath rctrl = stringToVRPath("/user/hand/right");
        }
    }

    bool getVR()
    {
        return sVRMode;
    }

    bool getKBMouseModeActive()
    {
        return !(getControllerActive(Paths::lctrl) || getControllerActive(Paths::rctrl));
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

    bool getControllerActive(VRPath path)
    {
        return sActiveControllers.contains(path);
    }

    VRPath getControllerInteractionProfile(VRPath controllerPath)
    {
        auto it = sActiveControllers.find(controllerPath);
        if (it != sActiveControllers.end())
            return it->second;
        return 0;
    }

    bool getLeftControllerActive()
    {
        return getControllerActive(Paths::lctrl);
    }

    bool getRightControllerActive()
    {
        return getControllerActive(Paths::rctrl);
    }

    void setControllerActive(
        VRPath controllerPath, VRPath interactionProfilePath, bool active)
    {
        if (active)
            sActiveControllers[controllerPath] = interactionProfilePath;
        else
            sActiveControllers.erase(controllerPath);
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

    std::string getRuntimeName()
    {
        return sRuntimeName;
    }

    void setVR(bool VR)
    {
        sVRMode = VR;
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

    void setRuntimeName(std::string name)
    {
        sRuntimeName = name;
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

    struct Interaction
    {
        enum ValueType
        {
            BOOLEAN,
            FLOAT,
            AXIS,
            POSE
        };

        ValueType valueType;
        std::string path;
        std::optional<std::string> requiredExtension = std::nullopt;
    };
    using Interactions = std::vector<Interaction>;
    using Controllers = std::map<std::string, Interactions>;
    using InteractionProfiles = std::map<std::string, Controllers>;
    constexpr const char* LEFT_HAND = "/user/hand/left";
    constexpr const char* RIGHT_HAND = "/user/hand/right";

    namespace
    {
    }
}
