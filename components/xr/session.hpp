#ifndef XR_SESSION_HPP
#define XR_SESSION_HPP

#include <mutex>

#include <components/vr/constants.hpp>
#include <components/vr/session.hpp>

#include <openxr/openxr.h>

namespace VR
{
    class StageToWorldBinding;
}

namespace XR
{
    class Tracker;

    extern void getEulerAngles(const osg::Quat& quat, float& yaw, float& pitch, float& roll);

    /// \brief Manages VR logic, such as managing frames, predicting their poses, and handling frame synchronization
    /// with the VR runtime. Should not be confused with the openxr session object.
    class Session : public VR::Session
    {
    public:
        static Session& instance();

    public:
        Session(XrSession session, XrViewConfigurationType viewConfigType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO);
        ~Session();

        void xrResourceAcquired();
        void xrResourceReleased();

        XrSession xrSession() { return mXrSession; }

        XrSpace getReferenceSpace(VR::ReferenceSpace space);

        XR::Tracker& tracker() { return *mTracker; }
        std::array<Stereo::View, 2> getPredictedViews(int64_t predictedDisplayTime, VR::ReferenceSpace space) override;

        void xrDebugSetNames();

        std::array<VR::SwapchainConfig, 2> getRecommendedSwapchainConfig() const override;

    protected:
        void newFrame(uint64_t frameNo, bool& shouldSyncFrame, bool& shouldSyncInput) override;
        void syncFrameUpdate(uint64_t frameNo, bool& shouldRender, uint64_t& predictedDisplayTime,
            uint64_t& predictedDisplayPeriod) override;
        void syncFrameRender(VR::Frame& frame) override;
        void syncFrameEnd(VR::Frame& frame) override;

        void handleEvents();
        bool xrNextEvent(XrEventDataBuffer& eventBuffer);
        void xrQueueEvents();
        const XrEventDataBaseHeader* nextEvent();
        bool processEvent(const XrEventDataBaseHeader* header);
        void popEvent();
        bool handleSessionStateChanged(const XrEventDataSessionStateChanged& stateChangedEvent);
        bool checkStopCondition();

        void xrInteractionProfileChanged();

        void init();
        void cleanup();

        void createXrReferenceSpaces();
        void destroyXrReferenceSpaces();
        void logXrReferenceSpaces();

        void createXrTracker();
        void initCompositionLayerDepth();
        void initMSFTReprojection();

        void destroyXrSession();

        std::shared_ptr<VR::Swapchain> createSwapchain(uint32_t width, uint32_t height, uint32_t samples,
            uint32_t arraySize, VR::Swapchain::Attachment attachment, const std::string& name) override;

    private:
        XrSession mXrSession;
        std::queue<XrEventDataBuffer> mEventQueue;
        XrViewConfigurationType mViewConfigType;
        XrSessionState mState = XR_SESSION_STATE_UNKNOWN;

        std::vector<XrReferenceSpaceType> mReferenceSpaceTypes{};
        XrSpace mReferenceSpaceView = XR_NULL_HANDLE;
        XrSpace mReferenceSpaceStage = XR_NULL_HANDLE;
        XrSpace mReferenceSpaceLocal = XR_NULL_HANDLE;

        std::unique_ptr<XR::Tracker> mTracker;

        std::mutex mMutex;
        uint32_t mAcquiredResources = 0;

        bool mXrSessionShouldStop = false;
        bool mAppShouldSyncFrameLoop = false;
        bool mAppShouldReadInput = false;

        bool mMSFTReprojectionModeDepth = false;
    };

}

#endif
