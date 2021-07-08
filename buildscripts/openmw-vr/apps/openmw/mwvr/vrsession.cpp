#include "vrsession.hpp"
#include "vrgui.hpp"
#include "vrenvironment.hpp"
#include "vrinputmanager.hpp"
#include "openxrmanager.hpp"
#include "openxrswapchain.hpp"
#include "../mwinput/inputmanagerimp.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/statemanager.hpp"

#include <components/debug/debuglog.hpp>
#include <components/sdlutil/sdlgraphicswindow.hpp>
#include <components/misc/stringops.hpp>
#include <components/misc/constants.hpp>

#include <osg/Camera>

#include <algorithm>
#include <vector>
#include <array>
#include <iostream>
#include <time.h>
#include <thread>
#include <chrono>

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

namespace MWVR
{
    VRSession::VRSession()
    {
        mSeatedPlay = Settings::Manager::getBool("seated play", "VR");
    }

    VRSession::~VRSession()
    {
    }

    void VRSession::processChangedSettings(const std::set<std::pair<std::string, std::string>>& changed)
    {
        setSeatedPlay(Settings::Manager::getBool("seated play", "VR"));
    }

    void VRSession::beginFrame()
    {
        // Viewer traversals are sometimes entered without first updating the input manager.
        if (getFrame(FramePhase::Update) == nullptr)
        {
            beginPhase(FramePhase::Update);
        }

    }

    void VRSession::endFrame()
    {
        // Make sure we don't continue until the render thread has moved the current update frame to its next phase.
        std::unique_lock<std::mutex> lock(mMutex);
        while (getFrame(FramePhase::Update))
        {
            mCondition.wait(lock);
        }
    }

    void VRSession::setSeatedPlay(bool seatedPlay)
    {
        std::swap(mSeatedPlay, seatedPlay);
        if (mSeatedPlay != seatedPlay)
        {
            Environment::get().getInputManager()->requestRecenter(true);
        }
    }

    static void swapConvention(osg::Vec3& v3)
    {

        float y = v3.y();
        float z = v3.z();
        v3.y() = z;
        v3.z() = -y;
    }

    static void swapConvention(osg::Quat& q)
    {
        float y = q.y();
        float z = q.z();
        q.y() = z;
        q.z() = -y;
    }

    void VRSession::swapBuffers(osg::GraphicsContext* gc, VRViewer& viewer)
    {
        auto* xr = Environment::get().getManager();

        auto* frameMeta = getFrame(FramePhase::Draw).get();

        if (frameMeta->mShouldSyncFrameLoop)
        {
            gc->swapBuffersImplementation();
            if (frameMeta->mShouldRender)
            {
                std::array<CompositionLayerProjectionView, 2> layerStack{};
                layerStack[(int)Side::LEFT_SIDE].subImage = viewer.subImage(Side::LEFT_SIDE);
                layerStack[(int)Side::RIGHT_SIDE].subImage = viewer.subImage(Side::RIGHT_SIDE);
                layerStack[(int)Side::LEFT_SIDE].pose = frameMeta->mViews[(int)ReferenceSpace::STAGE][(int)Side::LEFT_SIDE].pose;
                layerStack[(int)Side::RIGHT_SIDE].pose = frameMeta->mViews[(int)ReferenceSpace::STAGE][(int)Side::RIGHT_SIDE].pose;
                layerStack[(int)Side::LEFT_SIDE].fov = frameMeta->mViews[(int)ReferenceSpace::STAGE][(int)Side::LEFT_SIDE].fov;
                layerStack[(int)Side::RIGHT_SIDE].fov = frameMeta->mViews[(int)ReferenceSpace::STAGE][(int)Side::RIGHT_SIDE].fov;
                xr->endFrame(frameMeta->mFrameInfo, &layerStack);
            }
            else
            {
                xr->endFrame(frameMeta->mFrameInfo, nullptr);
            }

            xr->xrResourceReleased();
        }

        {
            std::unique_lock<std::mutex> lock(mMutex);
            mLastRenderedFrame = frameMeta->mFrameNo;
            getFrame(FramePhase::Draw) = nullptr;
        }

        mCondition.notify_all();
    }

    void VRSession::beginPhase(FramePhase phase)
    {
        {
            // TODO: Use a queue system
            std::unique_lock<std::mutex> lock(mMutex);
            while (getFrame(phase))
            {
                Log(Debug::Verbose) << "Warning: beginPhase called with a frame already in the target phase";
                mCondition.wait(lock);
            }
        }

        auto& frame = getFrame(phase);

        if (phase == FramePhase::Update)
        {
            prepareFrame();
        }
        else
        {
            std::unique_lock<std::mutex> lock(mMutex);
            frame = std::move(getFrame(FramePhase::Update));
        }

        mCondition.notify_all();

        if (phase == FramePhase::Draw && frame->mShouldSyncFrameLoop)
        {
            Environment::get().getManager()->beginFrame();
        }

    }

    std::unique_ptr<VRSession::VRFrameMeta>& VRSession::getFrame(FramePhase phase)
    {
        if ((unsigned int)phase >= mFrame.size())
            throw std::logic_error("Invalid frame phase");
        return mFrame[(int)phase];
    }

    void VRSession::prepareFrame()
    {
        mFrames++;

        auto* xr = Environment::get().getManager();
        xr->handleEvents();
        auto& frame = getFrame(FramePhase::Update);
        frame.reset(new VRFrameMeta);

        frame->mFrameNo = mFrames;
        frame->mShouldSyncFrameLoop = xr->appShouldSyncFrameLoop();
        //frame->mShouldRender = xr->appShouldRender();
        if (frame->mShouldSyncFrameLoop)
        {
            frame->mFrameInfo = xr->waitFrame();
            frame->mShouldRender = frame->mFrameInfo.runtimeRequestsRender;
            xr->xrResourceAcquired();
        }
    }

    // OSG doesn't provide API to extract euler angles from a quat, but i need it.
    // Credits goes to Dennis Bunfield, i just copied his formula https://narkive.com/v0re6547.4
    void getEulerAngles(const osg::Quat& quat, float& yaw, float& pitch, float& roll)
    {
        // Now do the computation
        osg::Matrixd m2(osg::Matrixd::rotate(quat));
        double* mat = (double*)m2.ptr();
        double angle_x = 0.0;
        double angle_y = 0.0;
        double angle_z = 0.0;
        double D, C, tr_x, tr_y;
        angle_y = D = asin(mat[2]); /* Calculate Y-axis angle */
        C = cos(angle_y);
        if (fabs(C) > 0.005) /* Test for Gimball lock? */
        {
            tr_x = mat[10] / C; /* No, so get X-axis angle */
            tr_y = -mat[6] / C;
            angle_x = atan2(tr_y, tr_x);
            tr_x = mat[0] / C; /* Get Z-axis angle */
            tr_y = -mat[1] / C;
            angle_z = atan2(tr_y, tr_x);
        }
        else /* Gimball lock has occurred */
        {
            angle_x = 0; /* Set X-axis angle to zero
            */
            tr_x = mat[5]; /* And calculate Z-axis angle
            */
            tr_y = mat[4];
            angle_z = atan2(tr_y, tr_x);
        }

        yaw = angle_z;
        pitch = angle_x;
        roll = angle_y;
    }

}

