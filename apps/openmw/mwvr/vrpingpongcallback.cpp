#include <osg/BlendFunc>
#include <osg/Drawable>
#include <osg/Fog>
#include <osg/LightModel>
#include <osg/Material>
#include <osg/MatrixTransform>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>

#include <components/sceneutil/shadow.hpp>

#include <components/settings/values.hpp>
#include <components/settings/categories/postprocessing.hpp>

#include <components/vr/viewer.hpp>

#include "../mwbase/world.hpp"

#include "../mwrender/renderingmanager.hpp"
#include "vrpingpongcallback.hpp"

namespace MWVR
{

    PingPongCallback::PingPongCallback(MWRender::PostProcessor *parent)
        : mParent(parent)
        , mDestinationViewport(new osg::Viewport)
    {

    }

    void PingPongCallback::pingPongBegin(osg::State& state, const MWRender::PingPongCanvas& canvas)
    {
        /*
         * Pseudocode:
         * determine view
         * vrviewer -> get target framebuffer for view
         * canvas -> set destination framebuffer
         */

        // if(frameId == mLastFrameId)
        //     view = left
        size_t frameId = state.getFrameStamp()->getFrameNumber();

        Stereo::Eye eye = Stereo::Eye::Left;
        if (Stereo::getMultiview())
            eye = Stereo::Eye::Center;
        else if (frameId == mLastFrameId)
            eye = Stereo::Eye::Right;

        auto fbo = VR::Viewer::instance().getFboForView(eye);

        fbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
        glClearColor(0.5, 0.5, 0.5, 1);
        glClear(GL_DEPTH_BUFFER_BIT);

        canvas.setDestinationFbo(fbo);

        auto& sm = Stereo::Manager::instance();
        auto resolution = sm.eyeResolution();
        mDestinationViewport->setViewport(0, 0, resolution.x(), resolution.y());

        canvas.setDestinationViewport(frameId, mDestinationViewport);
    }

    void PingPongCallback::pingPongEnd(osg::State& state, const MWRender::PingPongCanvas& canvas)
    {
        /*
         * Pseudocode:
         * determine view
         * vrviewer -> get target framebuffer for view
         *   [[-> acquire swapchain and use swapchain directly?]]
         * canvas -> get depth texture
         * blit depth texture if app should share depth info
         *
         */
        size_t frameId = state.getFrameStamp()->getFrameNumber();

        Stereo::Eye eye = Stereo::Eye::Left;
        if (Stereo::getMultiview())
            eye = Stereo::Eye::Center;
        else if (frameId == mLastFrameId)
            eye = Stereo::Eye::Right;
        mLastFrameId = frameId;

        osg::ref_ptr<osg::FrameBufferObject> fbo = nullptr;
        if (Settings::postProcessing().mTransparentPostpass)
            fbo = mParent->getFbo(MWRender::PostProcessor::FBO_OpaqueDepth, frameId);
        else
            fbo = mParent->getFbo(MWRender::PostProcessor::FBO_Primary, frameId);

        VR::Viewer::instance().submitDepthForView(state, fbo, eye);
    }

}
