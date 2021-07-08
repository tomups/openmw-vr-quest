#ifndef MWVR_OPENRXMANAGER_H
#define MWVR_OPENRXMANAGER_H
#ifndef USE_OPENXR
#error "openxrmanager.hpp included without USE_OPENXR defined"
#endif

#include <memory>
#include <array>
#include <mutex>
#include <components/debug/debuglog.hpp>
#include <components/sdlutil/sdlgraphicswindow.hpp>
#include <components/settings/settings.hpp>
#include <osg/Camera>
#include <osgViewer/Viewer>
#include "vrtypes.hpp"


struct XrSwapchainSubImage;
struct XrCompositionLayerBaseHeader;

namespace MWVR
{
    class OpenXRManagerImpl;

    /// \brief Manage the openxr runtime and session
    class OpenXRManager : public osg::Referenced
    {
    public:
        class CleanupOperation : public osg::GraphicsOperation
        {
        public:
            CleanupOperation() : osg::GraphicsOperation("OpenXRCleanupOperation", false) {};
            void operator()(osg::GraphicsContext* gc) override;

        private:
        };


    public:
        OpenXRManager();

        ~OpenXRManager();

        /// Manager has been initialized.
        bool realized() const;

        //! Forward call to xrWaitFrame()
        FrameInfo waitFrame();

        //! Forward call to xrBeginFrame()
        void beginFrame();

        //! Forward call to xrEndFrame()
        void endFrame(FrameInfo frameInfo, const std::array<CompositionLayerProjectionView, 2>* layerStack);

        //! Whether the app should call the openxr frame sync functions ( xr*Frame() )
        bool appShouldSyncFrameLoop() const;

        //! Whether the app should render anything.
        bool appShouldRender() const;

        //! Whether the session is focused and can read input
        bool appShouldReadInput() const;

        //! Process all openxr events
        void handleEvents();

        //! Instantiate implementation
        void realize(osg::GraphicsContext* gc);

        //! Enable pose predictions. Exist to police that predictions are never made out of turn.
        void enablePredictions();

        //! Disable pose predictions.
        void disablePredictions();

        //! Must be called every time an openxr resource is acquired to keep track
        void xrResourceAcquired();

        //! Must be called every time an openxr resource is released to keep track
        void xrResourceReleased();

        //! Get poses and fov of both eyes at the predicted time, relative to the given reference space. \note Will throw if predictions are disabled.
        std::array<View, 2> getPredictedViews(int64_t predictedDisplayTime, ReferenceSpace space);

        //! Get the pose of the player's head at the predicted time, relative to the given reference space. \note Will throw if predictions are disabled.
        MWVR::Pose getPredictedHeadPose(int64_t predictedDisplayTime, ReferenceSpace space);

        //! Last predicted display time returned from xrWaitFrame();
        long long getLastPredictedDisplayTime();

        //! Last predicted display period returned from xrWaitFrame();
        long long getLastPredictedDisplayPeriod();

        //! Configuration hints for instantiating swapchains, queried from openxr.
        std::array<SwapchainConfig, 2> getRecommendedSwapchainConfig() const;

        //! Check whether a given openxr extension is enabled or not
        bool xrExtensionIsEnabled(const char* extensionName) const;

        //! Selects a color format from among formats offered by the runtime
        //! Returns 0 if no format is supported.
        int64_t selectColorFormat();

        //! Selects a depth format from among formats offered by the runtime
        //! Returns 0 if no format is supported.
        int64_t selectDepthFormat();

        //! Erase format from list of format candidates
        void eraseFormat(int64_t format);

        OpenXRManagerImpl& impl() { return *mPrivate; }
        const OpenXRManagerImpl& impl() const { return *mPrivate; }

    private:
        std::shared_ptr<OpenXRManagerImpl> mPrivate;
        std::mutex mMutex;
        using lock_guard = std::lock_guard<std::mutex>;
    };
}

#endif
