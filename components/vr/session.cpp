#include "session.hpp"

#include <components/debug/debuglog.hpp>
#include <components/misc/constants.hpp>
#include <components/sdlutil/sdlgraphicswindow.hpp>
#include <components/settings/values.hpp>
#include <components/vr/trackingsource.hpp>
#include <components/vr/vr.hpp>
#include <components/vr/space.hpp>

#include <osg/Camera>

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <time.h>
#include <vector>

namespace VR
{
    Session* sSession = nullptr;

    Session& Session::instance()
    {
        assert(sSession);
        return *sSession;
    }

    Session::Session()
    {
        if (!sSession)
            sSession = this;
        else
            throw std::logic_error("Duplicated VR::Session singleton");

        readSettings();
    }

    Session::~Session() {}

    void Session::processChangedSettings(const std::set<std::pair<std::string, std::string>>& changed)
    {
        readSettings();
    }

    VR::Frame Session::newFrame()
    {
        static uint64_t frameNo = 0;
        VR::Frame frame;
        frame.frameNumber = frameNo++;
        newFrame(frame.frameNumber, frame.shouldSyncFrameLoop, frame.shouldSyncInput);
        return frame;
    }

    void Session::frameBeginUpdate(VR::Frame& frame)
    {
        if (frame.shouldSyncFrameLoop)
        {
            syncFrameUpdate(
                frame.frameNumber, frame.shouldRender, frame.predictedDisplayTime, frame.predictedDisplayPeriod);

            VR::setPredictedDisplayTime(frame.predictedDisplayTime);
            VR::setPredictedDisplayPeriod(frame.predictedDisplayPeriod);
        }
        for (auto& listener : mListeners)
            listener->onFrameUpdate(frame);
        VR::setLocatingSpacesAllowed(true);
        if (mRecenter)
            recenter();
    }

    void Session::updateSpaces()
    {
        for (auto& listener : mListeners)
            listener->onSpaceUpdate();
        VR::setLocatingSpacesAllowed(false);
    }

    void Session::frameBeginRender(VR::Frame& frame)
    {
        if (frame.shouldSyncFrameLoop)
            syncFrameRender(frame);
        for (auto& listener : mListeners)
            listener->onFrameRender(frame);
    }

    void Session::frameEnd(osg::RenderInfo& info, VR::Frame& frame)
    {
        for (auto& listener : mListeners)
            listener->onFrameEnd(info, frame);
    }

    void Session::swapBuffers(osg::GraphicsContext* gc, VR::Frame& frame)
    {
        gc->swapBuffersImplementation();
        if (frame.shouldSyncFrameLoop)
            syncFrameEnd(frame);
    }

    void Session::addListener(Listener* listener) 
    {
        mListeners.push_back(listener);
    }

    void Session::removeListener(Listener* listener)
    {
        std::erase_if(mListeners, [&](Listener* lhs) { return lhs == listener; });
    }

    void Session::readSettings()
    {
        mHandsOffset.x() = (Settings::vr().mHandsOffsetX - 0.5) * Constants::UnitsPerMeter;
        mHandsOffset.y() = (Settings::vr().mHandsOffsetY - 0.5) * Constants::UnitsPerMeter;
        mHandsOffset.z() = (Settings::vr().mHandsOffsetZ - 0.5) * Constants::UnitsPerMeter;
    }

    void Session::computePlayerScale()
    {
        mPlayerHeight = Stereo::Unit::fromMeters(Settings::Manager::getFloat("player height", "VR"));
        Log(Debug::Verbose) << "Read player height: " << mPlayerHeight.asMeters();
        mPlayerScale = mCharHeight / mPlayerHeight;
        Log(Debug::Verbose) << "Calculated player scale: " << mPlayerScale;
    }

    void Session::setCharHeight(Stereo::Unit height)
    {
        Log(Debug::Verbose) << "Set char height: " << height.asMeters();
        mCharHeight = height;
        computePlayerScale();
    }

    void Session::recenter()
    {
        for (auto& listener : mListeners)
            listener->onRecenter();
        mRecenter = false;
    }

    void Session::instantTransition()
    {
        for (auto& listener : mListeners)
            listener->onInstantTransition();
    }

    void Session::setInteractionProfileActive(XrPath topLevelPath, XrPath profile, bool active)
    {
        VR::setControllerActive(topLevelPath, profile, active);

        if (active)
            mActiveInteractionProfiles.insert(topLevelPath);
        else
            mActiveInteractionProfiles.erase(topLevelPath);

        for (auto& listener : mListeners)
            listener->onInteractionProfileActiveChanged(topLevelPath, active);
    }

    bool Session::getInteractionProfileActive(XrPath topLevelPath) const
    {
        return !!mActiveInteractionProfiles.count(topLevelPath);
    }

    void Session::setSneak(bool sneak)
    {
        if (sneak)
            mSneakOffset = Stereo::Unit::fromMWUnits(-20.f);
        else
            mSneakOffset = {};
    }

    Session::Listener::Listener()
    {
        if (VR::getVR())
            Session::instance().addListener(this);
    }

    Session::Listener::~Listener()
    {
        if (VR::getVR())
            Session::instance().removeListener(this);
    }
}
