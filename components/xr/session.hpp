#ifndef XR_SESSION_HPP
#define XR_SESSION_HPP

#include <mutex>

#include <components/vr/constants.hpp>
#include <components/vr/session.hpp>

#include <openxr/openxr.h>

#include <array>

namespace VR
{
    class StageToWorldBinding;
}

namespace XR
{
    class Space;
    class ReferenceSpace;
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

        std::array<Stereo::View, 2> locateViews(int64_t predictedDisplayTime, VR::Space& reference) override;

        VR::TrackingPose locateSpace(XrSpace space, XrSpace referenceSpace) const;
        VR::TrackingPose locateSpaceInWorld(XrSpace space) const;
        void setReferenceWorldPose(Stereo::Pose pose) override;

        std::array<VR::SwapchainConfig, 2> getRecommendedSwapchainConfig() const override;
        std::shared_ptr<VR::Space> getReferenceSpace(VR::ReferenceSpace space) override;
        XrSpace createReferenceSpace(VR::ReferenceSpace reference, Stereo::Pose pose);

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

        void logXrReferenceSpaces();

        void initCompositionLayerDepth();
        void initMSFTReprojection();

        void destroyXrSession();

        void recenter() override;

        std::shared_ptr<VR::Swapchain> createSwapchain(uint32_t width, uint32_t height, uint32_t samples,
            uint32_t arraySize, VR::Swapchain::Attachment attachment, const std::string& name) override;
        void createReferenceSpaces();
        std::vector<VR::ReferenceSpace> getSupportedReferenceSpaceTypes() const override;

    private:
        XrSession mXrSession;
        std::queue<XrEventDataBuffer> mEventQueue;
        XrViewConfigurationType mViewConfigType;
        XrSessionState mState = XR_SESSION_STATE_UNKNOWN;
        Stereo::Pose mReferenceWorldPose = {};
        std::array<std::shared_ptr<ReferenceSpace>, 3> mReferenceSpaces;
        mutable std::map<std::pair<XrSpace, XrSpace>, VR::TrackingPose> mPoseCache;

        std::mutex mMutex;
        uint32_t mAcquiredResources = 0;

        bool mXrSessionShouldStop = false;
        bool mAppShouldSyncFrameLoop = false;
        bool mAppShouldReadInput = false;

        bool mMSFTReprojectionModeDepth = false;
    };

}

#endif
