#include "extensions.hpp"
#include "debug.hpp"

#include <cassert>
#include <openxr/openxr.h>

#ifdef XR_USE_PLATFORM_ANDROID
#include <EGL/egl.h> // openxr_platform.h's GLES structs reference EGL types
#include <jni.h>
#include <SDL_system.h>
#include <openxr/openxr_platform.h>
#endif

#include <components/debug/debuglog.hpp>
#include <components/settings/settings.hpp>

namespace XR
{
    static Extensions* sExtensions = nullptr;

    Extensions& Extensions::instance()
    {
        assert(sExtensions);
        return *sExtensions;
    }

    void initAndroidLoader()
    {
#ifdef XR_USE_PLATFORM_ANDROID
        JNIEnv* jniEnv = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
        JavaVM* javaVm = nullptr;
        if (jniEnv)
            jniEnv->GetJavaVM(&javaVm);
        jobject activity = static_cast<jobject>(SDL_AndroidGetActivity());

        PFN_xrInitializeLoaderKHR xrInitializeLoaderKHR = nullptr;
        if (XR_SUCCEEDED(xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR",
                reinterpret_cast<PFN_xrVoidFunction*>(&xrInitializeLoaderKHR)))
            && xrInitializeLoaderKHR)
        {
            XrLoaderInitInfoAndroidKHR loaderInitInfo{};
            loaderInitInfo.type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR;
            loaderInitInfo.applicationVM = javaVm;
            loaderInitInfo.applicationContext = activity;
            xrInitializeLoaderKHR(reinterpret_cast<const XrLoaderInitInfoBaseHeaderKHR*>(&loaderInitInfo));
        }
        else
            Log(Debug::Warning) << "xrInitializeLoaderKHR not available; OpenXR init may fail on Android";
#endif
    }

    Extensions::Extensions()
    {
        if (!sExtensions)
            sExtensions = this;
        else
            throw std::logic_error("Duplicated XR::Extensions singleton");

        XrApiLayerProperties apiLayerProperties{};
        apiLayerProperties.type = XR_TYPE_API_LAYER_PROPERTIES;

        // List of extensions we always enable if supported
        std::vector<std::string> autoExtensions = {
            "XR_KHR_composition_layer_depth", "XR_EXT_debug_utils", "XR_MSFT_composition_layer_reprojection",
            // "XR_FB_space_warp",

        };

        for (auto& extension : autoExtensions)
        {
            if (Settings::Manager::getBool("disable " + std::string(extension), "VR Debug"))
            {
                Log(Debug::Verbose) << "  " << extension << ": disabled (by config)";
            }
            else
            {
                requestExtension(extension);
            }
        }

        // Enumerate layers and their extensions.
        uint32_t layerCount;
        CHECK_XRCMD(xrEnumerateApiLayerProperties(0, &layerCount, nullptr));
        std::vector<XrApiLayerProperties> layers(layerCount, apiLayerProperties);
        CHECK_XRCMD(xrEnumerateApiLayerProperties((uint32_t)layers.size(), &layerCount, layers.data()));

        Log(Debug::Verbose) << "Available Extensions: ";
        enumerateExtensions(nullptr, 2);
        Log(Debug::Verbose) << "Available Layers: ";

        if (layers.size() == 0)
        {
            Log(Debug::Verbose) << "  No layers available";
        }
        for (const XrApiLayerProperties& layer : layers)
        {
            Log(Debug::Verbose) << "  Name=" << layer.layerName << " SpecVersion=" << layer.layerVersion;
            mAvailableLayers[layer.layerName] = layer;
            enumerateExtensions(layer.layerName, 4);
        }
    }

    Extensions::~Extensions() {}

    bool Extensions::supportsLayer(const std::string& layerName) const
    {
        return mAvailableLayers.count(std::string(layerName)) > 0;
    }

    bool Extensions::enableExtension(const std::string& extension)
    {
        //if (Settings::Manager::getBool("disable " + std::string(extension), "VR Debug"))
        //{
        //    Log(Debug::Verbose) << "  " << extension << ": disabled (by config)";
        //    return false;
        //}
        if (!extensionSupported(extension))
        {
            Log(Debug::Verbose) << "  " << extension << ": disabled (not supported)";
            return false;
        }
        Log(Debug::Verbose) << "  " << extension << ": enabled";
        mEnabledExtensions.insert(extension);
        return true;
    }

    void Extensions::selectGraphicsAPIExtension(const std::string& extension)
    {
        mGraphicsAPIExtension = extension;
        requestExtension(extension);
    }

    std::string_view Extensions::graphicsAPIExtension() const
    {
        return mGraphicsAPIExtension;
    }

    XrInstance Extensions::createXrInstance(const std::string& name)
    {
#ifdef XR_USE_PLATFORM_ANDROID
        // The instance must be created with XR_KHR_android_create_instance carrying the
        // JavaVM + activity. The loader itself is initialised earlier, in initAndroidLoader().
        requestExtension(XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME);

        JNIEnv* jniEnv = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
        JavaVM* javaVm = nullptr;
        if (jniEnv)
            jniEnv->GetJavaVM(&javaVm);
        jobject activity = static_cast<jobject>(SDL_AndroidGetActivity());
#endif

        setupExtensions();

        std::vector<const char*> cExtensionNames;
        for (const auto& extension : mEnabledExtensions)
            cExtensionNames.push_back(extension.c_str());

        XrInstance instance = XR_NULL_HANDLE;
        XrInstanceCreateInfo createInfo{};
        createInfo.type = XR_TYPE_INSTANCE_CREATE_INFO;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(cExtensionNames.size());
        createInfo.enabledExtensionNames = cExtensionNames.data();
        strcpy(createInfo.applicationInfo.applicationName, "openmw_vr");
        createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

#ifdef XR_USE_PLATFORM_ANDROID
        XrInstanceCreateInfoAndroidKHR androidCreateInfo{};
        androidCreateInfo.type = XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR;
        androidCreateInfo.applicationVM = javaVm;
        androidCreateInfo.applicationActivity = activity;
        createInfo.next = &androidCreateInfo;
#endif

        auto res = CHECK_XRCMD(xrCreateInstance(&createInfo, &instance));
        if (!XR_SUCCEEDED(res))
            initFailure(res, instance);
        return instance;
    }

    bool Extensions::extensionEnabled(const std::string& name) const
    {
        return std::find(mEnabledExtensions.begin(), mEnabledExtensions.end(), name) != mEnabledExtensions.end();
    }

    bool Extensions::extensionSupported(const std::string& name) const
    {
        return mAvailableExtensions.contains(name);
    }

    void Extensions::requestExtension(const std::string& name)
    {
        mRequestedExtensions.insert(name);
    }

    void Extensions::enumerateExtensions(const char* layerName, int logIndent)
    {
        XrExtensionProperties extensionProperties{};
        extensionProperties.type = XR_TYPE_EXTENSION_PROPERTIES;

        uint32_t extensionCount = 0;
        std::vector<XrExtensionProperties> availableExtensions;
        CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layerName, 0, &extensionCount, nullptr));
        availableExtensions.resize(extensionCount, extensionProperties);
        CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(
            layerName, static_cast<uint32_t>(availableExtensions.size()), &extensionCount, availableExtensions.data()));

        std::vector<std::string> extensionNames;
        const std::string indentStr(logIndent, ' ');
        for (auto& extension : availableExtensions)
        {
            if (layerName)
                mAvailableLayerExtensions[layerName][extension.extensionName] = extension;
            else
                mAvailableExtensions[extension.extensionName] = extension;

            Log(Debug::Verbose) << indentStr << "Name=" << extension.extensionName
                                << " SpecVersion=" << extension.extensionVersion;
        }
    }

    void Extensions::setupExtensions()
    {
        Log(Debug::Verbose) << "Using extensions:";

        for (auto extension : mRequestedExtensions)
            enableExtension(extension);

        auto graphicsAPI = graphicsAPIExtension();
        if (graphicsAPI.empty() || !extensionEnabled(std::string(graphicsAPI)))
            throw std::runtime_error("No graphics API supported by openmw are supported by the OpenXR runtime.");
    }
}
