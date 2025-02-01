#include "debug.hpp"
#include "extensions.hpp"
#include "instance.hpp"

#include <components/settings/settings.hpp>

#include <openxr/openxr.h>
#include <openxr/openxr_reflection.h>

#include <sstream>
#include <thread>

namespace XR
{
#define ENUM_CASE_STR(name, val)                                                                                       \
    case name:                                                                                                         \
        return #name;
#define MAKE_TO_STRING_FUNC(enumType)                                                                                  \
    const char* to_string(enumType e)                                                                                  \
    {                                                                                                                  \
        switch (e)                                                                                                     \
        {                                                                                                              \
            XR_LIST_ENUM_##enumType(ENUM_CASE_STR) default : return "Unknown " #enumType;                              \
        }                                                                                                              \
    }

    MAKE_TO_STRING_FUNC(XrReferenceSpaceType)
    MAKE_TO_STRING_FUNC(XrViewConfigurationType)
    MAKE_TO_STRING_FUNC(XrEnvironmentBlendMode)
    MAKE_TO_STRING_FUNC(XrSessionState)
    MAKE_TO_STRING_FUNC(XrResult)
    MAKE_TO_STRING_FUNC(XrFormFactor)
    MAKE_TO_STRING_FUNC(XrStructureType)

    XrResult CheckXrResult(XrResult res, const char* originator, const char* sourceLocation)
    {
        static bool initialized = false;
        static bool sLogAllXrCalls = false;
        static bool sContinueOnErrors = false;
        if (!initialized)
        {
            initialized = true;
            sLogAllXrCalls = Settings::Manager::getBool("log all openxr calls", "VR Debug");
            sContinueOnErrors = Settings::Manager::getBool("continue on errors", "VR Debug");
        }

        auto resultString = XrResultString(res);

        if (XR_FAILED(res))
        {
            std::stringstream ss;
            ss << sourceLocation << ": OpenXR[Error: " << resultString << "][Thread: " << std::this_thread::get_id()
               << "]: " << originator;
            Log(Debug::Error) << ss.str();
            if (!sContinueOnErrors)
                throw std::runtime_error(ss.str().c_str());
        }
        else if (res != XR_SUCCESS || sLogAllXrCalls)
        {
            Log(Debug::Verbose) << sourceLocation << ": OpenXR[" << resultString << "][" << std::this_thread::get_id()
                                << "]: " << originator;
        }

        return res;
    }

    static void xrSetDebugUtilsObjectNameEXT(XrDebugUtilsObjectNameInfoEXT* nameInfo)
    {
        static PFN_xrSetDebugUtilsObjectNameEXT setDebugUtilsObjectNameEXT = nullptr;
        if (setDebugUtilsObjectNameEXT == nullptr)
        {
            setDebugUtilsObjectNameEXT = reinterpret_cast<PFN_xrSetDebugUtilsObjectNameEXT>(
                Instance::instance().xrGetFunction("xrSetDebugUtilsObjectNameEXT"));
        }
        CHECK_XRCMD(setDebugUtilsObjectNameEXT(Instance::instance().xrInstance(), nameInfo));
    }

    void Debugging::setName(uint64_t handle, XrObjectType type, const std::string& name)
    {

        if (XR::Extensions::instance().extensionEnabled(XR_EXT_DEBUG_UTILS_EXTENSION_NAME))
        {
            XrDebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.type = XR_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            nameInfo.objectHandle = handle;
            nameInfo.objectType = type;
            nameInfo.objectName = name.c_str();

            xrSetDebugUtilsObjectNameEXT(&nameInfo);
        }
    }

    static XrInstanceProperties getInstanceProperties(XrInstance instance)
    {
        XrInstanceProperties properties{};
        properties.type = XR_TYPE_INSTANCE_PROPERTIES;

        if (instance)
            xrGetInstanceProperties(instance, &properties);
        return properties;
    }

    std::string getInstanceName(XrInstance instance)
    {
        if (instance)
            return getInstanceProperties(instance).runtimeName;
        return "unknown";
    }

    XrVersion getInstanceVersion(XrInstance instance)
    {
        if (instance)
            return getInstanceProperties(instance).runtimeVersion;
        return 0;
    }

    void initFailure(XrResult res, XrInstance instance)
    {
        std::stringstream ss;
        std::string runtimeName = getInstanceName(instance);
        XrVersion runtimeVersion = getInstanceVersion(instance);
        ss << "Error caught while initializing VR device: " << XrResultString(res) << std::endl;
        ss << "Device: " << runtimeName << std::endl;
        ss << "Version: " << runtimeVersion << std::endl;
        if (res == XR_ERROR_FORM_FACTOR_UNAVAILABLE)
        {
            ss << "Cause: Unable to open VR device. Make sure your device is plugged in and the VR driver is running."
               << std::endl;
            ss << std::endl;
            if (runtimeName == "Oculus" || runtimeName == "Quest")
            {
                ss << "Your device has been identified as an Oculus device." << std::endl;
                ss << "The most common cause for this error when using an oculus device, is quest users attempting to "
                      "run the game via Virtual Desktop."
                   << std::endl;
                ss << "Unfortunately this is currently broken, and quest users will need to play via a link cable."
                   << std::endl;
            }
        }
        else if (res == XR_ERROR_LIMIT_REACHED)
        {
            ss << "Cause: Device resources exhausted. Close other VR applications if you have any open. If you have "
                  "none, you may need to reboot to reset the driver."
               << std::endl;
        }
        else
        {
            ss << "Cause: Unknown. Make sure your device is plugged in and ready." << std::endl;
        }
        throw std::runtime_error(ss.str());
    }
}
