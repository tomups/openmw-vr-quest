#include "engine.hpp"

#include "mwvr/openxrmanager.hpp"
#include "mwvr/vrsession.hpp"
#include "mwvr/vrviewer.hpp"
#include "mwvr/vrgui.hpp"
#include "mwvr/vrtracking.hpp"

#ifndef USE_OPENXR
#error "USE_OPENXR not defined"
#endif

void OMW::Engine::initVr()
{
    if (!mViewer)
        throw std::logic_error("mViewer must be initialized before calling initVr()");

    mXrEnvironment.setManager(new MWVR::OpenXRManager);
    mXrEnvironment.setSession(new MWVR::VRSession());
    mXrEnvironment.setViewer(new MWVR::VRViewer(mViewer));
    mXrEnvironment.setTrackingManager(new MWVR::VRTrackingManager());
}
