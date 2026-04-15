#include "instance.hpp"
#include "interactionprofiles.hpp"
#include "debug.hpp"
#include "platform.hpp"
#include "session.hpp"
#include "swapchain.hpp"
#include "typeconversion.hpp"

#include <components/debug/debuglog.hpp>
#include <components/misc/strings/lower.hpp>
#include <components/sdlutil/sdlgraphicswindow.hpp>
#include <components/vr/directx.hpp>
#include <components/vr/layer.hpp>
#include <components/vr/trackingmanager.hpp>
#include <components/vr/vr.hpp>

#include <openxr/openxr_reflection.h>

#include <osg/Camera>

#include <array>
#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace XR
{
    Instance* sInstance = nullptr;

    Instance& Instance::instance()
    {
        assert(sInstance);
        return *sInstance;
    }

    Instance::Instance(osg::GraphicsContext* gc, SDL_Window* window)
    {
        if (!sInstance)
            sInstance = this;
        else
            throw std::logic_error("Duplicated XR::Instance singleton");

        mSystemProperties.type = XR_TYPE_SYSTEM_PROPERTIES;
        mConfigViews[0].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
        mConfigViews[1].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;

        mExtensions = std::make_unique<Extensions>();
        for (auto& extension : getAllKnownInteractionProfileExtensions())
            mExtensions->requestExtension(extension);
        mPlatform = std::make_unique<Platform>(gc, window);
        mPlatform->selectGraphicsAPIExtension();
        mXrInstance = mExtensions->createXrInstance("openmw_vr");
        Debugging::setName(mXrInstance, "OpenMW XR Instance");

        LogInstanceInfo();
        setupDebugMessenger();
        getSystem();
        getSystemProperties();
        enumerateViews();

        // TODO: Blend mode
        // setupBlendMode();
    }

    void Instance::getSystem()
    {
        XrSystemGetInfo systemInfo{};
        systemInfo.type = XR_TYPE_SYSTEM_GET_INFO;
        systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        auto res = CHECK_XRCMD(xrGetSystem(mXrInstance, &systemInfo, &mSystemId));
        if (!XR_SUCCEEDED(res))
            initFailure(res, mXrInstance);
    }

    void Instance::getSystemProperties()
    {
        // Read and log graphics properties for the swapchain
        CHECK_XRCMD(xrGetSystemProperties(mXrInstance, mSystemId, &mSystemProperties));

        // Log system properties.
        {
            std::stringstream ss;
            ss << "System Properties: Name=" << mSystemProperties.systemName
               << " VendorId=" << mSystemProperties.vendorId << std::endl;
            ss << "System Graphics Properties: MaxWidth="
               << mSystemProperties.graphicsProperties.maxSwapchainImageWidth;
            ss << " MaxHeight=" << mSystemProperties.graphicsProperties.maxSwapchainImageHeight;
            ss << " MaxLayers=" << mSystemProperties.graphicsProperties.maxLayerCount << std::endl;
            ss << "System Tracking Properties: OrientationTracking="
               << (mSystemProperties.trackingProperties.orientationTracking ? "True" : "False");
            ss << " PositionTracking=" << (mSystemProperties.trackingProperties.positionTracking ? "True" : "False");
            Log(Debug::Verbose) << ss.str();
        }

        auto systemNameLowerCase = Misc::StringUtils::lowerCase(mSystemProperties.systemName);
        if (systemNameLowerCase.find("steamvr") != std::string::npos)
            VR::setSteamVR(true);
    }

    void Instance::enumerateViews()
    {
        uint32_t configCount = 0;
        CHECK_XRCMD(xrEnumerateViewConfigurations(mXrInstance, mSystemId, 0, &configCount, nullptr));
        mViewConfigs.resize(configCount);
        CHECK_XRCMD(
            xrEnumerateViewConfigurations(mXrInstance, mSystemId, configCount, &configCount, mViewConfigs.data()));
        Log(Debug::Verbose) << "Available view configurations:";
        for (auto config : mViewConfigs)
        {
            Log(Debug::Verbose) << "  " << to_string(config);
            XrViewConfigurationProperties properties{};
            properties.type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES;
            CHECK_XRCMD(xrGetViewConfigurationProperties(mXrInstance, mSystemId, config, &properties));
            Log(Debug::Verbose) << "    next:" << properties.next;
            Log(Debug::Verbose) << "    viewConfigurationType:" << to_string(properties.viewConfigurationType);
            Log(Debug::Verbose) << "    fovMutable:" << properties.fovMutable;
        }

        uint32_t viewCount = 0;
        CHECK_XRCMD(xrEnumerateViewConfigurationViews(
            mXrInstance, mSystemId, mViewConfigType, 2, &viewCount, mConfigViews.data()));

        if (viewCount != 2)
        {
            std::stringstream ss;
            ss << "xrEnumerateViewConfigurationViews returned " << viewCount << " views";
            Log(Debug::Verbose) << ss.str();
        }

        uint32_t environmentBlendModeCount = 0;
        CHECK_XRCMD(xrEnumerateEnvironmentBlendModes(
            mXrInstance, mSystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &environmentBlendModeCount, nullptr));
        mBlendModes.resize(environmentBlendModeCount);
        CHECK_XRCMD(xrEnumerateEnvironmentBlendModes(mXrInstance, mSystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
            environmentBlendModeCount, &environmentBlendModeCount, mBlendModes.data()));
        Log(Debug::Verbose) << "Available blend modes:";
        for (auto blendMode : mBlendModes)
        {
            Log(Debug::Verbose) << "  " << to_string(blendMode);
        }
    }

    void Instance::cleanup()
    {
        destroyXrInstance();
    }

    void Instance::destroyXrInstance()
    {
        if (mXrInstance)
            CHECK_XRCMD(xrDestroyInstance(mXrInstance));
    }

    std::string XrResultString(XrResult res)
    {
        return to_string(res);
    }

    Instance::~Instance() {}

    void Instance::setupExtensionsAndLayers() {}
    static XrBool32 xrDebugCallback(XrDebugUtilsMessageSeverityFlagsEXT messageSeverity,
        XrDebugUtilsMessageTypeFlagsEXT messageType, const XrDebugUtilsMessengerCallbackDataEXT* callbackData,
        void* userData)
    {
        Instance* manager = reinterpret_cast<Instance*>(userData);
        (void)manager;
        std::string severityStr = "";
        std::string typeStr = "";

        switch (messageSeverity)
        {
            case XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                severityStr = "Verbose";
                break;
            case XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                severityStr = "Info";
                break;
            case XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                severityStr = "Warning";
                break;
            case XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                severityStr = "Error";
                break;
            default:
                severityStr = "Unknown";
                break;
        }

        switch (messageType)
        {
            case XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
                typeStr = "General";
                break;
            case XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
                typeStr = "Validation";
                break;
            case XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
                typeStr = "Performance";
                break;
            case XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT:
                typeStr = "Conformance";
                break;
            default:
                typeStr = "Unknown";
                break;
        }

        Log(Debug::Verbose) << "XrCallback: [" << severityStr << "][" << typeStr
                            << "][ID=" << (callbackData->messageId ? callbackData->messageId : "null")
                            << "]: " << callbackData->message;

        return XR_FALSE;
    }

    void Instance::setupDebugMessenger(void)
    {
        if (mExtensions->extensionEnabled(XR_EXT_DEBUG_UTILS_EXTENSION_NAME))
        {
            XrDebugUtilsMessengerCreateInfoEXT createInfo{};
            createInfo.type = XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

            // Debug message severity levels
            if (Settings::Manager::getBool("XR_EXT_debug_utils message level verbose", "VR Debug"))
                createInfo.messageSeverities |= XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
            if (Settings::Manager::getBool("XR_EXT_debug_utils message level info", "VR Debug"))
                createInfo.messageSeverities |= XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
            if (Settings::Manager::getBool("XR_EXT_debug_utils message level warning", "VR Debug"))
                createInfo.messageSeverities |= XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
            if (Settings::Manager::getBool("XR_EXT_debug_utils message level error", "VR Debug"))
                createInfo.messageSeverities |= XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

            // Debug message types
            if (Settings::Manager::getBool("XR_EXT_debug_utils message type general", "VR Debug"))
                createInfo.messageTypes |= XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
            if (Settings::Manager::getBool("XR_EXT_debug_utils message type validation", "VR Debug"))
                createInfo.messageTypes |= XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
            if (Settings::Manager::getBool("XR_EXT_debug_utils message type performance", "VR Debug"))
                createInfo.messageTypes |= XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            if (Settings::Manager::getBool("XR_EXT_debug_utils message type conformance", "VR Debug"))
                createInfo.messageTypes |= XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT;

            createInfo.userCallback = &xrDebugCallback;
            createInfo.userData = this;

            PFN_xrCreateDebugUtilsMessengerEXT createDebugUtilsMessenger
                = reinterpret_cast<PFN_xrCreateDebugUtilsMessengerEXT>(xrGetFunction("xrCreateDebugUtilsMessengerEXT"));
            assert(createDebugUtilsMessenger);
            CHECK_XRCMD(createDebugUtilsMessenger(mXrInstance, &createInfo, &mDebugMessenger));
        }
    }

    void Instance::LogInstanceInfo()
    {

        XrInstanceProperties instanceProperties{};
        instanceProperties.type = XR_TYPE_INSTANCE_PROPERTIES;

        CHECK_XRCMD(xrGetInstanceProperties(mXrInstance, &instanceProperties));
        Log(Debug::Verbose) << "Instance RuntimeName=" << instanceProperties.runtimeName
                            << " RuntimeVersion=" << instanceProperties.runtimeVersion;

        VR::setRuntimeName(instanceProperties.runtimeName);
    }

    void Instance::endFrame(VR::Frame& frame) {}

    PFN_xrVoidFunction Instance::xrGetFunction(const std::string& name)
    {
        PFN_xrVoidFunction function = nullptr;
        xrGetInstanceProcAddr(mXrInstance, name.c_str(), &function);
        return function;
    }

    Platform& Instance::platform()
    {
        return *mPlatform;
    }

    std::shared_ptr<Session> Instance::createSession()
    {
        auto xrSession = mPlatform->createXrSession(mXrInstance, mSystemId, mSystemProperties.systemName);
        return std::make_shared<Session>(xrSession, mViewConfigType);
    }

    std::string Instance::getRuntimeName()
    {
        return std::string(mSystemProperties.systemName);
    }

    std::array<XrViewConfigurationView, 2> Instance::getRecommendedXrSwapchainConfig() const
    {
        return mConfigViews;
    }
}
