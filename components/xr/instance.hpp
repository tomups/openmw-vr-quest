#ifndef XR_INSTANCE_HPP
#define XR_INSTANCE_HPP

#include "extensions.hpp"
#include "platform.hpp"

#include <components/debug/debuglog.hpp>
#include <components/sdlutil/sdlgraphicswindow.hpp>
#include <components/stereo/stereomanager.hpp>
#include <components/vr/constants.hpp>
#include <components/vr/directx.hpp>
#include <components/vr/frame.hpp>

#include <openxr/openxr.h>

#include <array>
#include <vector>

namespace XR
{
    class Session;

    /// \brief Instantiates and manages and openxr instance.
    class Instance
    {
    public:
        static Instance& instance();

    public:
        Instance(osg::GraphicsContext* gc);
        ~Instance(void);

        void endFrame(VR::Frame& frame);
        std::array<XrViewConfigurationView, 2> getRecommendedXrSwapchainConfig() const;
        XrInstance xrInstance() const { return mXrInstance; }
        XrSystemId xrSystemId() const { return mSystemId; }
        PFN_xrVoidFunction xrGetFunction(const std::string& name);
        XR::Platform& platform();

        std::shared_ptr<XR::Session> createSession();

        std::string getRuntimeName();

    protected:
        void setupExtensionsAndLayers();
        void setupDebugMessenger(void);
        void LogInstanceInfo();
        void getSystem();
        void getSystemProperties();
        void enumerateViews();

        void cleanup();
        void destroyXrInstance();

    private:
        XrInstance mXrInstance = XR_NULL_HANDLE;
        XrViewConfigurationType mViewConfigType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        XrSystemId mSystemId = XR_NULL_SYSTEM_ID;
        XrSystemProperties mSystemProperties{};
        std::vector<XrViewConfigurationType> mViewConfigs{};
        std::vector<XrEnvironmentBlendMode> mBlendModes{};
        std::array<XrViewConfigurationView, 2> mConfigViews{};
        XrDebugUtilsMessengerEXT mDebugMessenger{ nullptr };
        std::unique_ptr<Extensions> mExtensions{};
        std::unique_ptr<Platform> mPlatform{};

        // TODO: uint32_t mAcquiredResources = 0;
        std::mutex mMutex{};
    };
}

#endif
