#ifndef OPENXR_MANAGER_IMPL_HPP
#define OPENXR_MANAGER_IMPL_HPP

#include "openxrmanager.hpp"
#include "openxrplatform.hpp"
#include "openxrtracker.hpp"
#include "../mwinput/inputmanagerimp.hpp"

#include <components/debug/debuglog.hpp>
#include <components/sdlutil/sdlgraphicswindow.hpp>

#include <openxr/openxr.h>

#include <vector>
#include <array>
#include <map>
#include <iostream>
#include <thread>
#include <chrono>
#include <queue>

namespace MWVR
{
    /// \brief Implementation of OpenXRManager
    class OpenXRManagerImpl
    {
    public:
        OpenXRManagerImpl(osg::GraphicsContext* gc);
        ~OpenXRManagerImpl(void);

        FrameInfo waitFrame();
        void beginFrame();
        void endFrame(FrameInfo frameInfo, const std::array<CompositionLayerProjectionView, 2>* layerStack);
        bool appShouldSyncFrameLoop() const { return mAppShouldSyncFrameLoop; }
        bool appShouldRender() const { return mAppShouldRender; }
        bool appShouldReadInput() const { return mAppShouldReadInput; }
        std::array<View, 2> getPredictedViews(int64_t predictedDisplayTime, ReferenceSpace space);
        MWVR::Pose getPredictedHeadPose(int64_t predictedDisplayTime, ReferenceSpace space);
        void handleEvents();
        void enablePredictions();
        void disablePredictions();
        long long getLastPredictedDisplayTime();
        long long getLastPredictedDisplayPeriod();
        std::array<SwapchainConfig, 2> getRecommendedSwapchainConfig() const;
        XrSpace getReferenceSpace(ReferenceSpace space);
        XrSession xrSession() const { return mSession; };
        XrInstance xrInstance() const { return mInstance; };
        bool xrExtensionIsEnabled(const char* extensionName) const;
        void xrResourceAcquired();
        void xrResourceReleased();
        void xrUpdateNames();
        PFN_xrVoidFunction xrGetFunction(const std::string& name);
        int64_t selectColorFormat();
        int64_t selectDepthFormat();
        void eraseFormat(int64_t format);
        OpenXRPlatform& platform() { return mPlatform; }
        OpenXRTracker& tracker() { return *mTracker; }
        void initTracker();

    protected:
        void setupExtensionsAndLayers();
        void setupDebugMessenger(void);
        void LogInstanceInfo();
        void LogReferenceSpaces();
        bool xrNextEvent(XrEventDataBuffer& eventBuffer);
        void xrQueueEvents();
        const XrEventDataBaseHeader* nextEvent();
        bool processEvent(const XrEventDataBaseHeader* header);
        void popEvent();
        bool handleSessionStateChanged(const XrEventDataSessionStateChanged& stateChangedEvent);
        bool checkStopCondition();
        void createReferenceSpaces();
        void getSystem();
        void getSystemProperties();
        void enumerateViews();
        void setupLayerDepth();

    private:

        bool initialized = false;
        bool mPredictionsEnabled = false;
        XrInstance mInstance = XR_NULL_HANDLE;
        XrSession mSession = XR_NULL_HANDLE;
        XrSpace mSpace = XR_NULL_HANDLE;
        XrFormFactor mFormFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        XrViewConfigurationType mViewConfigType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        XrEnvironmentBlendMode mEnvironmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        XrSystemId mSystemId = XR_NULL_SYSTEM_ID;
        XrSystemProperties mSystemProperties{ XR_TYPE_SYSTEM_PROPERTIES };
        std::array<XrViewConfigurationView, 2> mConfigViews{ { {XR_TYPE_VIEW_CONFIGURATION_VIEW}, {XR_TYPE_VIEW_CONFIGURATION_VIEW} } };
        XrSpace mReferenceSpaceView = XR_NULL_HANDLE;
        XrSpace mReferenceSpaceStage = XR_NULL_HANDLE;
        XrSpace mReferenceSpaceLocal = XR_NULL_HANDLE;
        XrFrameState mFrameState{};
        XrSessionState mSessionState = XR_SESSION_STATE_UNKNOWN;
        XrDebugUtilsMessengerEXT mDebugMessenger{ nullptr };
        OpenXRPlatform mPlatform;

        std::unique_ptr<OpenXRTracker> mTracker{ nullptr };
        std::unique_ptr<VRTrackingToWorldBinding> mTrackerToWorldBinding{ nullptr };

        bool mXrSessionShouldStop = false;
        bool mAppShouldSyncFrameLoop = false;
        bool mAppShouldRender = false;
        bool mAppShouldReadInput = false;

        uint32_t mAcquiredResources = 0;
        std::mutex mMutex{};
        std::queue<XrEventDataBuffer> mEventQueue;

        std::array<XrCompositionLayerDepthInfoKHR, 2> mLayerDepth;
    };
}

#endif 
