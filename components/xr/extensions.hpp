#ifndef XR_EXTENSIONS_HPP
#define XR_EXTENSIONS_HPP

#include <map>
#include <string>
#include <vector>

#include <openxr/openxr.h>

#include <components/vr/directx.hpp>

namespace XR
{
    struct Extension
    {
        enum class EnableMode
        {
            Auto, //!< Automatically enable if available
            Manual //!< Only activate if requested
        };

        Extension(const char* name, EnableMode enableMode);

        bool operator==(const Extension& rhs);

        const std::string& name() const { return mName; }
        EnableMode enableMode() const { return mEnableMode; }
        bool enabled() const { return mEnabled; }
        bool disabledByConfig() const;
        bool supported() const { return mSupported && !disabledByConfig(); }
        bool requested() const { return mRequested; }
        const XrExtensionProperties* properties() const { return mProperties; }

        void setRequested(bool requested) { mRequested = requested; }

    private:
        friend class Extensions;
        void setProperties(XrExtensionProperties* properties) { mProperties = properties; }
        void setEnabled(bool enabled) { mEnabled = enabled; }
        void setSupported(bool supported) { mSupported = supported; }
        bool shouldEnable() const;

        std::string mName;
        EnableMode mEnableMode;
        bool mEnabled;
        bool mSupported;
        bool mRequested;
        XrExtensionProperties* mProperties;
    };

    class Extensions
    {
        using ExtensionMap = std::map<std::string, XrExtensionProperties>;
        using LayerMap = std::map<std::string, XrApiLayerProperties>;
        using LayerExtensionMap = std::map<std::string, ExtensionMap>;

    public:
        static Extensions& instance();

    public:
        Extensions();
        ~Extensions();
        void selectGraphicsAPIExtension(Extension* extensionName);
        Extension* graphicsAPIExtension() const;

        XrInstance createXrInstance(const std::string& name);

        bool extensionEnabled(const std::string& name) const;

    private:
        void enumerateExtensions(const char* layerName, int logIndent);
        bool supportsLayer(const std::string& layerName) const;
        bool initExtension(Extension* extension);
        bool enableExtension(Extension* extension);
        void setupExtensions();

        ExtensionMap mAvailableExtensions;
        LayerMap mAvailableLayers;
        LayerExtensionMap mAvailableLayerExtensions;
        std::vector<Extension*> mEnabledExtensions;
        Extension* mGraphicsAPIExtension = nullptr;
    };

    extern Extension KHR_opengl_enable;
    extern Extension KHR_D3D11_enable;
    extern Extension KHR_composition_layer_depth;
    extern Extension EXT_hp_mixed_reality_controller;
    extern Extension EXT_debug_utils;
    extern Extension HTC_vive_cosmos_controller_interaction;
    extern Extension HUAWEI_controller_interaction;
    extern Extension MSFT_composition_layer_reprojection;
    extern Extension FB_space_warp;
}

#endif
