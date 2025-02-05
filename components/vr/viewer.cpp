#include "viewer.hpp"

#include <osg/BufferObject>
#include <osg/StateAttribute>
#include <osg/Texture2DArray>
#include <osg/VertexArrayState>
#include <osgViewer/Renderer>

#include <components/debug/gldebug.hpp>

#include <components/misc/callbackmanager.hpp>
#include <components/misc/constants.hpp>
#include <components/misc/strings/lower.hpp>
#include <components/misc/strings/algorithm.hpp>

#include <components/stereo/multiview.hpp>
#include <components/stereo/stereomanager.hpp>

#include <components/vr/layer.hpp>
#include <components/vr/session.hpp>
#include <components/vr/swapchain.hpp>
#include <components/vr/trackingmanager.hpp>
#include <components/vr/trackingtransform.hpp>

#include <components/sceneutil/color.hpp>
#include <components/sceneutil/depth.hpp>
#include <components/sceneutil/util.hpp>
#include <components/sdlutil/sdlgraphicswindow.hpp>

#include <cassert>

namespace VR
{
    static bool isNumber(const std::string& in)
    {
        for (auto c : in)
        {
            if (!std::isdigit(c))
            {
                return false;
            }
        }
        return true;
    }

    int parseResolution(std::string conf, int recommended, int max)
    {
        if (isNumber(conf))
        {
            int res = std::atoi(conf.c_str());
            if (res <= 0)
                return recommended;
            if (res > max)
                return max;
            return res;
        }
        conf = Misc::StringUtils::lowerCase(conf);
        if (conf == "auto" || conf == "recommended")
        {
            return recommended;
        }
        if (conf == "max")
        {
            return max;
        }
        return recommended;
    }

    struct UpdateViewCallback : public Stereo::Manager::UpdateViewCallback
    {
        UpdateViewCallback(Viewer* viewer)
            : mViewer(viewer){}

        //! Called during the update traversal of every frame to source updated stereo values.
        virtual void updateView(Stereo::View& left, Stereo::View& right) override { mViewer->updateView(left, right); }

        Viewer* mViewer;
    };

    struct SwapBuffersCallback : public osg::GraphicsContext::SwapCallback
    {
    public:
        SwapBuffersCallback(Viewer* viewer)
            : mViewer(viewer){}
        void swapBuffersImplementation(osg::GraphicsContext* gc) override { mViewer->swapBuffersCallback(gc); }

    private:
        Viewer* mViewer;
    };

    struct InitialDrawCallback : public Misc::CallbackManager::MwDrawCallback
    {
    public:
        InitialDrawCallback(Viewer* viewer)
            : mViewer(viewer)
        {
        }

        bool operator()(osg::RenderInfo& info, Misc::CallbackManager::View view) const override
        {
            mViewer->initialDrawCallback(info, view);
            return true;
        }

    private:
        Viewer* mViewer;
    };

    struct FinaldrawCallback : public Misc::CallbackManager::MwDrawCallback
    {
    public:
        FinaldrawCallback(Viewer* viewer)
            : mViewer(viewer)
        {
        }

        bool operator()(osg::RenderInfo& info, Misc::CallbackManager::View view) const override
        {
            mViewer->finalDrawCallback(info, view);
            return true;
        }

    private:
        Viewer* mViewer;
    };

    Viewer* sViewer = nullptr;

    Viewer& Viewer::instance()
    {
        assert(sViewer);
        return *sViewer;
    }

    Viewer::Viewer(std::shared_ptr<VR::Session> session, osg::ref_ptr<osgViewer::Viewer> viewer)
        : mSession(session)
        , mViewer(viewer)
        , mSwapBuffersCallback(new SwapBuffersCallback(this))
        , mInitialDraw(new InitialDrawCallback(this))
        , mFinalDraw(new FinaldrawCallback(this))
        , mUpdateViewCallback(new UpdateViewCallback(this))
    {
        if (!sViewer)
            sViewer = this;
        else
            throw std::logic_error("Duplicated VR::Viewer singleton");

        // Read swapchain configs
        std::array<std::string, 2> xConfString;
        std::array<std::string, 2> yConfString;
        auto swapchainConfigs = VR::Session::instance().getRecommendedSwapchainConfig();
        xConfString[0] = Settings::Manager::getString("left eye resolution x", "VR");
        yConfString[0] = Settings::Manager::getString("left eye resolution y", "VR");
        xConfString[1] = Settings::Manager::getString("right eye resolution x", "VR");
        yConfString[1] = Settings::Manager::getString("right eye resolution y", "VR");

        // Instantiate swapchains for each view
        std::array<const char*, 2> viewNames = { "LeftEye", "RightEye" };
        for (unsigned i = 0; i < viewNames.size(); i++)
        {
            auto width
                = parseResolution(xConfString[i], swapchainConfigs[i].recommendedWidth, swapchainConfigs[i].maxWidth);
            auto height
                = parseResolution(yConfString[i], swapchainConfigs[i].recommendedHeight, swapchainConfigs[i].maxHeight);

            Log(Debug::Verbose) << viewNames[i] << " resolution: Recommended x=" << swapchainConfigs[i].recommendedWidth
                                << ", y=" << swapchainConfigs[i].recommendedHeight;
            Log(Debug::Verbose) << viewNames[i] << " resolution: Max x=" << swapchainConfigs[i].maxWidth
                                << ", y=" << swapchainConfigs[i].maxHeight;
            Log(Debug::Verbose) << viewNames[i] << " resolution: Selected x=" << width << ", y=" << height;

            mSubImages[i].width = width;
            mSubImages[i].height = height;
            mSubImages[i].x = mSubImages[i].y = 0;
        }

        // Determine samples and dimensions of framebuffers.
        mFramebufferWidth = mSubImages[0].width;
        mFramebufferHeight = mSubImages[0].height;

        if (mSubImages[0].width != mSubImages[1].width || mSubImages[0].height != mSubImages[1].height)
            Log(Debug::Warning) << "Warning: Eyes have differing resolutions. This case is not implemented";

        mViewer->getCamera()->setViewport(0, 0, mFramebufferWidth, mFramebufferHeight);

        // The gamma resolve framebuffer will be used to write the result of gamma post-processing.
        mGammaResolveFramebuffer = new osg::FrameBufferObject();
        mGammaResolveFramebuffer->setAttachment(osg::Camera::COLOR_BUFFER,
            osg::FrameBufferAttachment(new osg::RenderBuffer(
                mFramebufferWidth, mFramebufferHeight, SceneUtil::Color::colorInternalFormat(), 0)));

        mViewer->setReleaseContextAtEndOfFrameHint(false);
        mViewer->getCamera()->getGraphicsContext()->setSwapCallback(mSwapBuffersCallback);
        mViewer->getCamera()->setViewport(0, 0, mFramebufferWidth, mFramebufferHeight);
        Stereo::Manager::instance().overrideEyeResolution(osg::Vec2i(mFramebufferWidth, mFramebufferHeight));

        setupMirrorTexture();
        setupSwapchains();

        mProjectionLayer = std::make_shared<VR::ProjectionLayer>();
        for (uint32_t i = 0; i < 2; i++)
        {
            mProjectionLayer->views[i].subImage.index = 0;
            mProjectionLayer->views[i].subImage.width = mFramebufferWidth;
            mProjectionLayer->views[i].subImage.height = mFramebufferHeight;
            mProjectionLayer->views[i].subImage.x = 0;
            mProjectionLayer->views[i].subImage.y = 0;
            mProjectionLayer->views[i].colorSwapchain = mColorSwapchain[i];
            if (mSession->appShouldShareDepthInfo())
                mProjectionLayer->views[i].depthSwapchain = mDepthSwapchain[i];
        }
    }

    Viewer::~Viewer(void)
    {
        sViewer = nullptr;
    }

    static Viewer::MirrorTextureEye mirrorTextureEyeFromString(const std::string& str)
    {
        if (Misc::StringUtils::ciEqual(str, "left"))
            return Viewer::MirrorTextureEye::Left;
        if (Misc::StringUtils::ciEqual(str, "right"))
            return Viewer::MirrorTextureEye::Right;
        if (Misc::StringUtils::ciEqual(str, "both"))
            return Viewer::MirrorTextureEye::Both;
        return Viewer::MirrorTextureEye::Both;
    }

    void Viewer::configureCallbacks()
    {
        if (mCallbacksConfigured)
            return;

        // Give the main camera an initial draw callback that disables camera setup (we don't want it)
        Stereo::Manager::instance().setUpdateViewCallback(mUpdateViewCallback);
        Misc::CallbackManager::instance().addCallback(Misc::CallbackManager::DrawStage::Initial, mInitialDraw);
        Misc::CallbackManager::instance().addCallback(Misc::CallbackManager::DrawStage::Final, mFinalDraw);

        mCallbacksConfigured = true;
    }

    void Viewer::setupMirrorTexture()
    {
        mMirrorTextureEnabled = Settings::Manager::getBool("mirror texture", "VR");
        mMirrorTextureEye = mirrorTextureEyeFromString(Settings::Manager::getString("mirror texture eye", "VR"));
        mFlipMirrorTextureOrder = Settings::Manager::getBool("flip mirror texture order", "VR");

        mMirrorTextureViews.clear();
        if (mMirrorTextureEye == MirrorTextureEye::Left || mMirrorTextureEye == MirrorTextureEye::Both)
            mMirrorTextureViews.push_back(VR::Side_Left);
        if (mMirrorTextureEye == MirrorTextureEye::Right || mMirrorTextureEye == MirrorTextureEye::Both)
            mMirrorTextureViews.push_back(VR::Side_Right);
        if (mFlipMirrorTextureOrder)
            std::reverse(mMirrorTextureViews.begin(), mMirrorTextureViews.end());
    }

    void Viewer::processChangedSettings(const std::set<std::pair<std::string, std::string>>& changed)
    {
        bool mirrorTextureChanged = false;
        for (Settings::CategorySettingVector::const_iterator it = changed.begin(); it != changed.end(); ++it)
        {
            if (it->first == "VR" && it->second == "mirror texture")
            {
                mirrorTextureChanged = true;
            }
            if (it->first == "VR" && it->second == "mirror texture eye")
            {
                mirrorTextureChanged = true;
            }
            if (it->first == "VR" && it->second == "flip mirror texture order")
            {
                mirrorTextureChanged = true;
            }
        }

        if (mirrorTextureChanged)
            setupMirrorTexture();
    }

    void Viewer::insertLayer(std::shared_ptr<Layer> layer)
    {
        mLayers.push_back(layer);
    }

    void Viewer::removeLayer(std::shared_ptr<Layer> layer)
    {
        for (auto it = mLayers.begin(); it != mLayers.end(); it++)
        {
            if (*it == layer)
            {
                mLayers.erase(it);
                return;
            }
        }

        Log(Debug::Warning) << "VR::Viewer::removeLayer() called, but no such layer existed";
    }

    osg::ref_ptr<osg::FrameBufferObject> Viewer::getFboForView(Stereo::Eye view)
    {
        osg::ref_ptr<osg::FrameBufferObject> fbo = nullptr;
        auto stereoFbo = Stereo::Manager::instance().multiviewFramebuffer();
        if (Stereo::getMultiview())
        {
            fbo = stereoFbo->multiviewFbo();
        }
        else
        {
            int i = view == Stereo::Eye::Left ? 0 : 1;
            fbo = stereoFbo->layerFbo(i);
        }

        return fbo;
    }

    void Viewer::submitDepthForView(osg::State& state, osg::FrameBufferObject* depthFbo, Stereo::Eye view)
    {
        if (!mSession->appShouldShareDepthInfo())
            return;

        auto stereoFbo = Stereo::Manager::instance().multiviewFramebuffer();

        if (Stereo::getMultiview())
        {
            // TODO: Should cache this, but the pp keeps remaking the depth fbo so i need a dirty/cleanup step too.
            auto foo = std::make_unique<Stereo::MultiviewFramebufferResolve>(
                depthFbo, stereoFbo->multiviewFbo(), GL_DEPTH_BUFFER_BIT);
            foo->resolveImplementation(state);
        }
        else
        {
            stereoFbo->layerFbo(view == Stereo::Eye::Left ? 0 : 1)
                ->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
            depthFbo->apply(state, osg::FrameBufferObject::READ_FRAMEBUFFER);
            osg::GLExtensions* ext = state.get<osg::GLExtensions>();
            ext->glBlitFramebuffer(0, 0, mFramebufferWidth, mFramebufferHeight, 0, 0, mFramebufferWidth,
                mFramebufferHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        }
    }

    const VR::Frame& Viewer::currentUpdateFrame()
    {
        if (mReadyFrames.empty())
            throw std::logic_error("VR::Viewer::currentUpdateFrame() called outside of update");
        return mReadyFrames.back();
    }

    void Viewer::setFinalDrawCallback(std::function<void(osg::RenderInfo& info)> cb) {
        mFinalDrawCallback = cb;
    }

    osg::ref_ptr<osg::FrameBufferObject> Viewer::getXrFramebuffer(uint32_t view, osg::State* state)
    {
        uint64_t colorImage = mColorSwapchain[view]->image()->glImage();
        uint64_t depthImage = 0;
        uint32_t textureTarget = mColorSwapchain[view]->textureTarget();

        if (mSession->appShouldShareDepthInfo())
            depthImage = mDepthSwapchain[view]->image()->glImage();

        auto it = mSwapchainFramebuffers.find(std::pair{ colorImage, depthImage });
        if (it == mSwapchainFramebuffers.end())
        {
            osg::ref_ptr<osg::FrameBufferObject> fbo = new osg::FrameBufferObject();

            auto colorAttachment = Stereo::createLayerAttachmentFromHandle(
                state, colorImage, textureTarget, mFramebufferWidth, mFramebufferHeight, view);
            fbo->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER, colorAttachment);

            if (depthImage != 0)
            {
                auto depthAttachment = Stereo::createLayerAttachmentFromHandle(
                    state, depthImage, textureTarget, mFramebufferWidth, mFramebufferHeight, view);
                fbo->setAttachment(osg::FrameBufferObject::BufferComponent::DEPTH_BUFFER, depthAttachment);
            }

            it = mSwapchainFramebuffers.emplace(std::pair{ colorImage, depthImage }, fbo).first;
        }
        return it->second;
    }

    void Viewer::blitXrFramebuffer(osg::State* state, int i)
    {
        auto* gl = osg::GLExtensions::Get(state->getContextID(), false);

        auto dst = getXrFramebuffer(i, state);
        dst->apply(*state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);

        auto width = mSubImages[i].width;
        auto height = mSubImages[i].height;
        bool flip = mColorSwapchain[i]->mustFlipVertical();

        uint32_t srcX0 = 0;
        uint32_t srcX1 = srcX0 + width;
        uint32_t srcY0 = flip ? height : 0;
        uint32_t srcY1 = flip ? 0 : height;
        uint32_t dstX0 = mSubImages[i].x;
        uint32_t dstX1 = dstX0 + width;
        uint32_t dstY0 = mSubImages[i].y;
        uint32_t dstY1 = dstY0 + height;

        Stereo::Manager::instance().multiviewFramebuffer()->layerFbo(i)->apply(
            *state, osg::FrameBufferObject::READ_FRAMEBUFFER);
        gl->glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        if (mSession->appShouldShareDepthInfo())
        {
            gl->glBlitFramebuffer(
                srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        }

        gl->glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
    }

    void Viewer::blitMirrorTexture(osg::State* state, int i)
    {
        auto* gl = osg::GLExtensions::Get(state->getContextID(), false);

        auto* traits = SDLUtil::GraphicsWindowSDL2::findContext(*mViewer)->getTraits();
        int screenWidth = traits->width;
        int screenHeight = traits->height;

        // Compute the dimensions of each eye on the mirror texture.
        int dstWidth = screenWidth / mMirrorTextureViews.size();

        // Blit each eye
        // Which eye is blitted left/right is determined by which order left/right was added to mMirrorTextureViews
        int dstX = 0;
        gl->glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
        Stereo::Manager::instance().multiviewFramebuffer()->layerFbo(i)->apply(
            *state, osg::FrameBufferObject::READ_FRAMEBUFFER);
        for (auto viewId : mMirrorTextureViews)
        {
            if (viewId == static_cast<unsigned int>(i))
                gl->glBlitFramebuffer(0, 0, mFramebufferWidth, mFramebufferHeight, dstX, 0, dstX + dstWidth,
                    screenHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
            dstX += dstWidth;
        }
    }

    void Viewer::setupSwapchains()
    {
        for (int i : { 0, 1 })
        {
            mColorSwapchain[i] = VR::Session::instance().createSwapchain(mFramebufferWidth, mFramebufferHeight, 1, 1,
                VR::Swapchain::Attachment::Color, i == 0 ? "LeftEye" : "RightEye");
            if (mSession->appShouldShareDepthInfo())
            {
                // Depth support is buggy or just not supported on some runtimes and has to be guarded.
                try
                {
                    mDepthSwapchain[i] = VR::Session::instance().createSwapchain(mFramebufferWidth, mFramebufferHeight,
                        1, 1, VR::Swapchain::Attachment::DepthStencil, i == 0 ? "LeftEye" : "RightEye");
                }
                catch (std::exception& e)
                {
                    Log(Debug::Warning) << "XR_KHR_composition_layer_depth was enabled, but a depth attachment "
                                           "swapchain could not be created. Depth information will not be submitted: "
                                        << e.what();
                    mSession->setAppShouldShareDepthBuffer(false);
                    mDepthSwapchain[0] = mDepthSwapchain[1] = nullptr;
                }
            }
        }
    }

    void Viewer::blit(osg::RenderInfo& info)
    {
        auto* state = info.getState();
        auto* gl = osg::GLExtensions::Get(state->getContextID(), false);

        for (auto i = 0; i < 2; i++)
        {
            mColorSwapchain[i]->beginFrame(state->getGraphicsContext());
            if (mSession->appShouldShareDepthInfo())
                mDepthSwapchain[i]->beginFrame(state->getGraphicsContext());

            if (mMirrorTextureEnabled)
                blitMirrorTexture(state, i);
            blitXrFramebuffer(state, i);

            mColorSwapchain[i]->endFrame(state->getGraphicsContext());
            if (mSession->appShouldShareDepthInfo())
                mDepthSwapchain[i]->endFrame(state->getGraphicsContext());
        }

        // Undo all framebuffer bindings we have done.
        gl->glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
    }

    void Viewer::initialDrawCallback(osg::RenderInfo& info, Misc::CallbackManager::View view)
    {
        // Should only activate on the first callback
        if (view == Misc::CallbackManager::View::Right)
            return;

        {
            std::scoped_lock lock(mMutex);
            mDrawFrame = mReadyFrames.front();
            mReadyFrames.pop();
        }
        VR::Session::instance().frameBeginRender(mDrawFrame);
    }

    void Viewer::finalDrawCallback(osg::RenderInfo& info, Misc::CallbackManager::View view)
    {
        // Should only activate on the last callback
        if (view == Misc::CallbackManager::View::Left)
            return;
        VR::Session::instance().frameEnd(info, mDrawFrame);
        if (mFinalDrawCallback)
            mFinalDrawCallback(info);
        if (mDrawFrame.shouldRender)
        {
            blit(info);
        }
    }

    void Viewer::swapBuffersCallback(osg::GraphicsContext* gc)
    {
        VR::Session::instance().swapBuffers(gc, mDrawFrame);
    }

    void Viewer::newFrame()
    {
        auto frame = mSession->newFrame();
        mSession->frameBeginUpdate(frame);
        std::unique_lock<std::mutex> lock(mMutex);
        mReadyFrames.push(frame);
    }

    void Viewer::updateView(Stereo::View& left, Stereo::View& right)
    {
        newFrame();

        std::unique_lock<std::mutex> lock(mMutex);
        auto& frame = mReadyFrames.back();

        auto localViews
            = VR::Session::instance().locateViews(frame.predictedDisplayTime, XR_REFERENCE_SPACE_TYPE_LOCAL);
        auto views = VR::Session::instance().locateViews(frame.predictedDisplayTime, XR_REFERENCE_SPACE_TYPE_VIEW);

        if (frame.shouldRender && frame.shouldSyncFrameLoop)
        {
            left = views[VR::Side_Left];
            right = views[VR::Side_Right];

            // Print view once to log, useful for debugging the views of headsets i do not posess.
            static bool havePrintedView = false;
            if (!havePrintedView)
            {
                havePrintedView = true;
                Log(Debug::Verbose) << "Left View: " << left;
                Log(Debug::Verbose) << "Right View: " << right;
            }

            std::shared_ptr<VR::ProjectionLayer> projectionLayer
                = std::make_shared<VR::ProjectionLayer>(*mProjectionLayer);

            for (uint32_t i = 0; i < 2; i++)
            {
                projectionLayer->views[i].view = localViews[i];
            }
            frame.layers.push_back(projectionLayer);
            if (!mLayers.empty())
                frame.layers.insert(frame.layers.end(), mLayers.begin(), mLayers.end());
        }
    }
}
