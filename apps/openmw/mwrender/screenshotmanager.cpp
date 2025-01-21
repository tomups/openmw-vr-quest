#include "screenshotmanager.hpp"

#include <condition_variable>
#include <mutex>

#include <components/stereo/multiview.hpp>
#include <components/stereo/stereomanager.hpp>
#include <osg/Texture2DArray>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "postprocessor.hpp"

// ## VR_PATCH BEGIN
#include <components/misc/callbackmanager.hpp>
// ## VR_PATCH END

namespace MWRender
{

    // ## VR_PATCH BEGIN
    //  Port to use Misc::CallbackManager::MwDrawCallback
    class NotifyDrawCompletedCallback : public Misc::CallbackManager::MwDrawCallback
    {
    public:
        NotifyDrawCompletedCallback()
            : mFrame(0)
        {
        }

        bool operator()(osg::RenderInfo& renderInfo, Misc::CallbackManager::View view) const override
        {
            if (view == Misc::CallbackManager::View::Left)
                return false;

            std::lock_guard<std::mutex> lock(mMutex);
            if (renderInfo.getState()->getFrameStamp()->getFrameNumber() >= mFrame)
            {
                mCondition.notify_one();
                return true;
            }
            return false;
        }

        void reset(unsigned int frame)
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mFrame = frame;
        }

        mutable std::condition_variable mCondition;
        mutable std::mutex mMutex;
        unsigned int mFrame;
        // ## VR_PATCH END
    };

    class ReadImageFromFramebufferCallback : public osg::Drawable::DrawCallback
    {
    public:
        ReadImageFromFramebufferCallback(osg::Image* image, int width, int height)
            : mWidth(width)
            , mHeight(height)
            , mImage(image)
        {
        }
        void drawImplementation(osg::RenderInfo& renderInfo, const osg::Drawable* /*drawable*/) const override
        {
            int screenW = renderInfo.getCurrentCamera()->getViewport()->width();
            int screenH = renderInfo.getCurrentCamera()->getViewport()->height();
            if (Stereo::getStereo())
            {
                auto eyeRes = Stereo::Manager::instance().eyeResolution();
                screenW = eyeRes.x();
                screenH = eyeRes.y();
            }
            double imageaspect = (double)mWidth / (double)mHeight;
            int leftPadding = std::max(0, static_cast<int>(screenW - screenH * imageaspect) / 2);
            int topPadding = std::max(0, static_cast<int>(screenH - screenW / imageaspect) / 2);
            int width = screenW - leftPadding * 2;
            int height = screenH - topPadding * 2;

            osg::GLExtensions* ext = osg::GLExtensions::Get(renderInfo.getContextID(), false);
            if (ext && Stereo::getMultiview())
            {
                PostProcessor* postProcessor
                    = dynamic_cast<PostProcessor*>(renderInfo.getCurrentCamera()->getUserData());
                size_t frameId = renderInfo.getState()->getFrameStamp()->getFrameNumber() % 2;

                if (postProcessor && postProcessor->getFbo(PostProcessor::FBO_Primary, frameId))
                {
                    osg::FrameBufferObject* fbo = postProcessor->getFbo(PostProcessor::FBO_Primary, frameId);
                    auto tex = fbo->getAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0).getTexture();
                    auto tex2dArray = dynamic_cast<const osg::Texture2DArray*>(tex);
                    if (tex2dArray)
                    {
                        fbo = new osg::FrameBufferObject;
                        fbo->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0,
                            osg::FrameBufferAttachment(const_cast<osg::Texture2DArray*>(tex2dArray), 0));
                    }
                }
            }
            mImage->readPixels(leftPadding, topPadding, width, height, GL_RGB, GL_UNSIGNED_BYTE);
            mImage->scaleImage(mWidth, mHeight, 1);
        }

    private:
        int mWidth;
        int mHeight;
        osg::ref_ptr<osg::Image> mImage;
    };

    ScreenshotManager::ScreenshotManager(osgViewer::Viewer* viewer)
        : mViewer(viewer)
        , mDrawCompleteCallback(new NotifyDrawCompletedCallback)
    {
    }

    ScreenshotManager::~ScreenshotManager() {}

    void ScreenshotManager::screenshot(osg::Image* image, int w, int h)
    {
        osg::Camera* camera = MWBase::Environment::get().getWorld()->getPostProcessor()->getHUDCamera();
        osg::ref_ptr<osg::Drawable> tempDrw = new osg::Drawable;
        tempDrw->setDrawCallback(new ReadImageFromFramebufferCallback(image, w, h));
        tempDrw->setCullingActive(false);
        tempDrw->getOrCreateStateSet()->setRenderBinDetails(100, "RenderBin",
            osg::StateSet::USE_RENDERBIN_DETAILS); // so its after all scene bins but before POST_RENDER gui camera
        camera->addChild(tempDrw);

        // Ref https://gitlab.com/OpenMW/openmw/-/issues/6013
        mDrawCompleteCallback->reset(mViewer->getFrameStamp()->getFrameNumber());
        // ## VR_PATCH BEGIN
        Misc::CallbackManager::instance().addCallbackOneshot(
            Misc::CallbackManager::DrawStage::Final, mDrawCompleteCallback);
        MWBase::Environment::get().getWindowManager()->viewerTraversals();
        Misc::CallbackManager::instance().waitCallbackOneshot(
            Misc::CallbackManager::DrawStage::Final, mDrawCompleteCallback);
        // ## VR_PATCH END

        // now that we've "used up" the current frame, get a fresh frame number for the next frame() following after the
        // screenshot is completed
        mViewer->advance(mViewer->getFrameStamp()->getSimulationTime());
        camera->removeChild(tempDrw);
    }
}
