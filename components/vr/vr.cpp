#include "vr.hpp"
#include "session.hpp"
#include "viewer.hpp"
#include "trackingmanager.hpp"

#include <components/debug/debuglog.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/xr/debug.hpp>
#include <components/xr/instance.hpp>

#include <map>
#include <optional>

namespace VR
{
    namespace
    {
        bool sRecenterLua = true;
        bool sVRMode = false;
        bool sSteamVR = false;
        bool sLocatingSpacesAllowed = false;
        bool sLeftHanded = false;
        DisplayTime sPredictedDisplayTime = 0;
        DisplayTime sPredictedDisplayPeriod = 0;
        std::map<XrPath, XrPath> sActiveControllers = {};
        std::string sRuntimeName = "UNINITIALIZED";
    }

    XrPath stringToXrPath(const std::string& path)
    {
        XrPath xrPath = XR_NULL_PATH;
        CHECK_XRCMD(::xrStringToPath(XR::Instance::instance().xrInstance(), path.c_str(), &xrPath));
        return xrPath;
    }

    std::string xrPathToString(XrPath path)
    {
        std::vector<char> output;
        uint32_t size = 0;
        ::xrPathToString(XR::Instance::instance().xrInstance(), path, 0, &size, nullptr);
        output.resize(size + 1, 0);
        ::xrPathToString(XR::Instance::instance().xrInstance(), path, output.size(), &size, output.data());

        return std::string(output.data());
    }

    bool getVR()
    {
        return sVRMode;
    }

    bool getKBMouseModeActive()
    {
        return !getVR()
            || !(getControllerActive(stringToXrPath("/user/hand/left"))
                || getControllerActive(stringToXrPath("/user/hand/right")));
    }

    bool getSteamVR()
    {
        return sSteamVR;
    }

    bool getControllerActive(XrPath path)
    {
        return sActiveControllers.contains(path);
    }

    XrPath getControllerInteractionProfile(XrPath controllerPath)
    {
        auto it = sActiveControllers.find(controllerPath);
        if (it != sActiveControllers.end())
            return it->second;
        return 0;
    }

    bool getLeftControllerActive()
    {
        return getControllerActive(stringToXrPath("/user/hand/left"));
    }

    bool getRightControllerActive()
    {
        return getControllerActive(stringToXrPath("/user/hand/right"));
    }

    bool getLocatingSpacesAllowed()
    {
        return sLocatingSpacesAllowed;
    }

    bool getLeftHandedMode()
    {
        return sLeftHanded;
    }

    void setControllerActive(XrPath controllerPath, XrPath interactionProfilePath, bool active)
    {
        if (active)
            sActiveControllers[controllerPath] = interactionProfilePath;
        else
            sActiveControllers.erase(controllerPath);
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

    const char* getPreferredAimPath()
    {
        if (sLeftHanded)
        {
            if (getLeftControllerActive())
                return Paths::LEFT_HAND_AIM;
            return Paths::RIGHT_HAND_AIM;
        }
        if (getRightControllerActive())
            return Paths::RIGHT_HAND_AIM;
        return Paths::LEFT_HAND_AIM;
    }

    void setVR(bool VR)
    {
        sVRMode = VR;
    }

    void setSteamVR(bool steamVR)
    {
        sSteamVR = steamVR;
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

    void setLocatingSpacesAllowed(bool allowed) 
    {
        sLocatingSpacesAllowed = allowed;
    }

    void setRuntimeName(std::string name)
    {
        sRuntimeName = name;
    }

    void setLeftHandedMode(bool enable) 
    {
        sLeftHanded = enable;
    }

    void recenter() 
    {
        sRecenterLua = true;
        if (sVRMode)
            Session::instance().requestRecenter();
    }

    bool getShouldRecenterLua()
    {
        return sRecenterLua;
    }

    void setShouldRecenterLua(bool arg) 
    {
        sRecenterLua = arg;
    }
}
