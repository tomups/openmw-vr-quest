#include "vrenvironment.hpp"

#include <cassert>

#include "vranimation.hpp"
#include "vrinputmanager.hpp"
#include "vrsession.hpp"
#include "vrgui.hpp"

#include "../mwbase/environment.hpp"

MWVR::Environment* MWVR::Environment::sThis = 0;

MWVR::Environment::Environment()
    : mSession(nullptr)
{
    assert(!sThis);
    sThis = this;
}

MWVR::Environment::~Environment()
{
    cleanup();
    sThis = 0;
}

void MWVR::Environment::cleanup()
{
    if (mSession)
        delete mSession;
    mSession = nullptr;
    if (mGUIManager)
        delete mGUIManager;
    mGUIManager = nullptr;
    if (mViewer)
        delete mViewer;
    mViewer = nullptr;
    if (mOpenXRManager)
        delete mOpenXRManager;
    mOpenXRManager = nullptr;
}

MWVR::Environment& MWVR::Environment::get()
{
    assert(sThis);
    return *sThis;
}

MWVR::VRInputManager* MWVR::Environment::getInputManager() const
{
    auto* inputManager = MWBase::Environment::get().getInputManager();
    assert(inputManager);
    auto xrInputManager = dynamic_cast<MWVR::VRInputManager*>(inputManager);
    assert(xrInputManager);
    return xrInputManager;
}

MWVR::VRSession* MWVR::Environment::getSession() const
{
    return mSession;
}

void MWVR::Environment::setSession(MWVR::VRSession* xrSession)
{
    mSession = xrSession;
}

MWVR::VRGUIManager* MWVR::Environment::getGUIManager() const
{
    return mGUIManager;
}

void MWVR::Environment::setGUIManager(MWVR::VRGUIManager* GUIManager)
{
    mGUIManager = GUIManager;
}

MWVR::VRAnimation* MWVR::Environment::getPlayerAnimation() const
{
    return mPlayerAnimation;
}

void MWVR::Environment::setPlayerAnimation(MWVR::VRAnimation* xrAnimation)
{
    mPlayerAnimation = xrAnimation;
}


MWVR::VRViewer* MWVR::Environment::getViewer() const
{
    return mViewer;
}

void MWVR::Environment::setViewer(MWVR::VRViewer* xrViewer)
{
    mViewer = xrViewer;
}

MWVR::OpenXRManager* MWVR::Environment::getManager() const
{
    return mOpenXRManager;
}

void MWVR::Environment::setManager(MWVR::OpenXRManager* xrManager)
{
    mOpenXRManager = xrManager;
}

MWVR::VRTrackingManager* MWVR::Environment::getTrackingManager() const
{
    return mTrackingManager;
}

void MWVR::Environment::setTrackingManager(MWVR::VRTrackingManager* trackingManager)
{
    mTrackingManager = trackingManager;
}
