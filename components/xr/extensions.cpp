#include "extensions.hpp"
#include "debug.hpp"

#include <cassert>
#include <openxr/openxr.h>

#include <components/debug/debuglog.hpp>
#include <components/settings/settings.hpp>

namespace XR
{
    Extension KHR_opengl_enable("XR_KHR_opengl_enable", Extension::EnableMode::Manual);
    Extension KHR_D3D11_enable("XR_KHR_D3D11_enable", Extension::EnableMode::Manual);
    Extension KHR_composition_layer_depth("XR_KHR_composition_layer_depth", Extension::EnableMode::Auto);
    Extension EXT_debug_utils("XR_EXT_debug_utils", Extension::EnableMode::Auto);
    Extension EXT_hp_mixed_reality_controller("XR_EXT_hp_mixed_reality_controller", Extension::EnableMode::Auto);
    Extension HTC_vive_cosmos_controller_interaction(
        "XR_HTC_vive_cosmos_controller_interaction", Extension::EnableMode::Auto);
    Extension HUAWEI_controller_interaction("XR_HUAWEI_controller_interaction", Extension::EnableMode::Auto);
    Extension MSFT_composition_layer_reprojection(
        "XR_MSFT_composition_layer_reprojection", Extension::EnableMode::Auto);
    Extension FB_space_warp("XR_FB_space_warp", Extension::EnableMode::Auto);

    std::vector<Extension*> sExtensionsVector = { &KHR_opengl_enable, &KHR_D3D11_enable, &KHR_composition_layer_depth,
        &EXT_debug_utils, &EXT_hp_mixed_reality_controller, &HTC_vive_cosmos_controller_interaction,
        &HUAWEI_controller_interaction, &MSFT_composition_layer_reprojection, &FB_space_warp };

    static Extensions* sExtensions = nullptr;

    Extensions& Extensions::instance()
    {
        assert(sExtensions);
        return *sExtensions;
    }

    Extensions::Extensions()
    {
        if (!sExtensions)
            sExtensions = this;
        else
            throw std::logic_error("Duplicated XR::Extensions singleton");

        XrApiLayerProperties apiLayerProperties{};
        apiLayerProperties.type = XR_TYPE_API_LAYER_PROPERTIES;

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

        for (auto* extension : sExtensionsVector)
        {
            initExtension(extension);
        }
    }

    Extensions::~Extensions() {}

    bool Extensions::supportsLayer(const std::string& layerName) const
    {
        return mAvailableLayers.count(layerName) > 0;
    }

    bool Extensions::initExtension(Extension* extension)
    {
        auto it = mAvailableExtensions.find(extension->name());
        if (it != mAvailableExtensions.end())
        {
            extension->setSupported(true);
            extension->setProperties(&it->second);
        }
        return false;
    }

    bool Extensions::enableExtension(Extension* extension)
    {
        if (extension->shouldEnable())
        {
            Log(Debug::Verbose) << "  " << extension->name() << ": enabled";
            mEnabledExtensions.push_back(extension);
            extension->setEnabled(true);
        }
        else
        {
            if (extension->disabledByConfig())
                Log(Debug::Verbose) << "  " << extension->name() << ": disabled (by config)";
            else if (extension->enableMode() == Extension::EnableMode::Auto
                || (extension->requested() && !extension->supported()))
                Log(Debug::Verbose) << "  " << extension->name() << ": disabled (not supported)";
            else
                Log(Debug::Verbose) << "  " << extension->name() << ": disabled";
        }
        return extension->enabled();
    }

    void Extensions::selectGraphicsAPIExtension(Extension* extension)
    {
        mGraphicsAPIExtension = extension;
        if (mGraphicsAPIExtension)
            mGraphicsAPIExtension->setRequested(true);
    }

    Extension* Extensions::graphicsAPIExtension() const
    {
        return mGraphicsAPIExtension;
    }

    XrInstance Extensions::createXrInstance(const std::string& name)
    {
        setupExtensions();

        std::vector<const char*> cExtensionNames;
        for (const auto* extension : mEnabledExtensions)
            cExtensionNames.push_back(extension->name().c_str());

        XrInstance instance = XR_NULL_HANDLE;
        XrInstanceCreateInfo createInfo{};
        createInfo.type = XR_TYPE_INSTANCE_CREATE_INFO;
        createInfo.enabledExtensionCount = cExtensionNames.size();
        createInfo.enabledExtensionNames = cExtensionNames.data();
        strcpy(createInfo.applicationInfo.applicationName, "openmw_vr");
        createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

        auto res = CHECK_XRCMD(xrCreateInstance(&createInfo, &instance));
        if (!XR_SUCCEEDED(res))
            initFailure(res, instance);
        return instance;
    }

    bool Extensions::extensionEnabled(const std::string& name) const
    {
        for (auto* extension : mEnabledExtensions)
        {
            if (extension->name() == name)
            {
                return true;
            }
        }
        return false;
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
            layerName, availableExtensions.size(), &extensionCount, availableExtensions.data()));

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

    Extension::Extension(const char* name, EnableMode enableMode)
        : mName(name)
        , mEnableMode(enableMode)
        , mEnabled(false)
        , mSupported(false)
        , mRequested(false)
        , mProperties(nullptr)
    {
    }

    bool Extension::operator==(const Extension& rhs)
    {
        return mName == rhs.mName;
    }

    bool Extension::disabledByConfig() const
    {
        return Settings::Manager::getBool(std::string("disable ") + mName, "VR Debug");
    }

    bool Extension::shouldEnable() const
    {
        if (!supported())
            return false;

        switch (mEnableMode)
        {
            case EnableMode::Auto:
                return mSupported;
            case EnableMode::Manual:
                return mRequested && mSupported;
        }
        return false;
    }

    void Extensions::setupExtensions()
    {
        Log(Debug::Verbose) << "Using extensions:";

        for (auto extension : sExtensionsVector)
            enableExtension(extension);

        auto graphicsAPI = graphicsAPIExtension();
        if (!graphicsAPI || !graphicsAPI->enabled())
            throw std::runtime_error("No graphics API supported by openmw are supported by the OpenXR runtime.");
    }
}
