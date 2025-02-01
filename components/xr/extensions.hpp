#ifndef XR_EXTENSIONS_HPP
#define XR_EXTENSIONS_HPP

#include <map>
#include <string>
#include <vector>

#include <openxr/openxr.h>

#include <components/vr/directx.hpp>

namespace XR
{
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
        void selectGraphicsAPIExtension(const std::string& extensionName);
        std::string_view graphicsAPIExtension() const;

        XrInstance createXrInstance(const std::string& name);

        bool extensionEnabled(const std::string& name) const;
        bool extensionSupported(const std::string& name) const;
        void requestExtension(const std::string& name);

    private:
        void enumerateExtensions(const char* layerName, int logIndent);
        bool supportsLayer(const std::string& layerName) const;
        bool enableExtension(const std::string& extension);
        void setupExtensions();

        ExtensionMap mAvailableExtensions;
        LayerMap mAvailableLayers;
        LayerExtensionMap mAvailableLayerExtensions;
        std::set<std::string> mRequestedExtensions;
        std::set<std::string> mEnabledExtensions;
        std::set<std::string> mSupportedExtensions;
        std::string mGraphicsAPIExtension;
    };
}

#endif
