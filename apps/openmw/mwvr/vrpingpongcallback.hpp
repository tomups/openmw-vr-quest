#ifndef MWVR_PINGPONGCALLBACK_H
#define MWVR_PINGPONGCALLBACK_H

#include "../mwrender/pingpongcanvas.hpp"
#include "../mwrender/postprocessor.hpp"
#include "../mwrender/renderingmanager.hpp"

#include <components/vr/trackinglistener.hpp>
#include <osg/ref_ptr>

namespace osg
{
    class Viewport;
}

namespace MWVR
{
    //! PingPongCallback that takes care of connecting the post processor output to VR
    class PingPongCallback : public MWRender::PingPongCanvas::PingPongCallback
    {
    public:
        PingPongCallback(MWRender::PostProcessor* parent);
        ~PingPongCallback(){}

        void pingPongBegin(osg::State& state, const MWRender::PingPongCanvas& canvas) override;
        void pingPongEnd(osg::State& state, const MWRender::PingPongCanvas& canvas) override;

        size_t mLastFrameId = 0xffffffff;
        MWRender::PostProcessor* mParent;
        osg::ref_ptr<osg::Viewport> mDestinationViewport;
    };
}

#endif
