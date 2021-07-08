#include "openxrmanager.hpp"
#include "openxrdebug.hpp"
#include "vrenvironment.hpp"
#include "openxrmanagerimpl.hpp"
#include "../mwinput/inputmanagerimp.hpp"

#include <components/debug/debuglog.hpp>

namespace MWVR
{
    OpenXRManager::OpenXRManager()
        : mPrivate(nullptr)
        , mMutex()
    {

    }

    OpenXRManager::~OpenXRManager()
    {

    }

    bool
        OpenXRManager::realized() const
    {
        return !!mPrivate;
    }

    void OpenXRManager::handleEvents()
    {
        if (realized())
            return impl().handleEvents();
    }

    FrameInfo OpenXRManager::waitFrame()
    {
        return impl().waitFrame();
    }

    void OpenXRManager::beginFrame()
    {
        return impl().beginFrame();
    }

    void OpenXRManager::endFrame(FrameInfo frameInfo, const std::array<CompositionLayerProjectionView, 2>* layerStack)
    {
        return impl().endFrame(frameInfo, layerStack);
    }

    bool OpenXRManager::appShouldSyncFrameLoop() const
    {
        if (realized())
            return impl().appShouldSyncFrameLoop();
        return false;
    }

    bool OpenXRManager::appShouldRender() const
    {
        if (realized())
            return impl().appShouldRender();
        return false;
    }

    bool OpenXRManager::appShouldReadInput() const
    {
        if (realized())
            return impl().appShouldReadInput();
        return false;
    }

    void
        OpenXRManager::realize(
            osg::GraphicsContext* gc)
    {
        lock_guard lock(mMutex);
        if (!realized())
        {
            gc->makeCurrent();
            mPrivate = std::make_shared<OpenXRManagerImpl>(gc);
        }
    }

    void OpenXRManager::enablePredictions()
    {
        return impl().enablePredictions();
    }

    void OpenXRManager::disablePredictions()
    {
        return impl().disablePredictions();
    }

    void OpenXRManager::xrResourceAcquired()
    {
        return impl().xrResourceAcquired();
    }

    void OpenXRManager::xrResourceReleased()
    {
        return impl().xrResourceReleased();
    }

    std::array<View, 2> OpenXRManager::getPredictedViews(int64_t predictedDisplayTime, ReferenceSpace space)
    {
        return impl().getPredictedViews(predictedDisplayTime, space);
    }

    MWVR::Pose OpenXRManager::getPredictedHeadPose(int64_t predictedDisplayTime, ReferenceSpace space)
    {
        return impl().getPredictedHeadPose(predictedDisplayTime, space);
    }

    long long OpenXRManager::getLastPredictedDisplayTime()
    {
        return impl().getLastPredictedDisplayTime();
    }

    long long OpenXRManager::getLastPredictedDisplayPeriod()
    {
        return impl().getLastPredictedDisplayPeriod();
    }

    std::array<SwapchainConfig, 2> OpenXRManager::getRecommendedSwapchainConfig() const
    {
        return impl().getRecommendedSwapchainConfig();
    }

    bool OpenXRManager::xrExtensionIsEnabled(const char* extensionName) const
    {
        return impl().xrExtensionIsEnabled(extensionName);
    }

    int64_t OpenXRManager::selectColorFormat()
    {
        return impl().selectColorFormat();
    }

    int64_t OpenXRManager::selectDepthFormat()
    {
        return impl().selectDepthFormat();
    }

    void OpenXRManager::eraseFormat(int64_t format)
    {
        return impl().eraseFormat(format);
    }

    void
        OpenXRManager::CleanupOperation::operator()(
            osg::GraphicsContext* gc)
    {
        // TODO: Use this to make proper cleanup such as cleaning up VRFramebuffers.
    }
}

