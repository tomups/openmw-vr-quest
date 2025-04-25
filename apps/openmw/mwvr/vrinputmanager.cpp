#include "vrinputmanager.hpp"

#include "openxrinput.hpp"
#include "realisticcombat.hpp"
#include "vranimation.hpp"
#include "vrgui.hpp"
#include "vrpointer.hpp"

#include <components/debug/debuglog.hpp>
#include <components/sceneutil/visitor.hpp>
#include <components/sdlutil/sdlmappings.hpp>
#include <components/vr/session.hpp>
#include <components/vr/trackingmanager.hpp>
#include <components/vr/viewer.hpp>
#include <components/vr/vr.hpp>
#include <components/xr/action.hpp>
#include <components/xr/actionset.hpp>
#include <components/xr/instance.hpp>
#include <components/xr/session.hpp>

#include <MyGUI_InputManager.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwgui/draganddrop.hpp"
#include "../mwgui/inventorywindow.hpp"

#include "../mwinput/actionmanager.hpp"
#include "../mwinput/bindingsmanager.hpp"
#include "../mwinput/controllermanager.hpp"
#include "../mwinput/mousemanager.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwworld/player.hpp"

#include "../mwrender/camera.hpp"
#include "../mwrender/renderingmanager.hpp"

#include <extern/oics/ICSInputControlSystem.h>

#include <iostream>

namespace MWVR
{
    void VRInputManager::updateVRPointer(bool disableControls)
    {
        std::shared_ptr<VR::Space> source;
        if (VR::getLeftControllerActive() || VR::getRightControllerActive())
        {
            bool guiMode = MWBase::Environment::get().getWindowManager()->isGuiMode();

            if (!disableControls)
                MWBase::Environment::get().getWorld()->enableVRPointer(
                    mPointerLeft && VR::getLeftControllerActive(), mPointerRight && VR::getRightControllerActive());

            std::string action = "";
            if (mPointerRight)
                action = VR::Paths::RIGHT_HAND_AIM;
            else if (mPointerLeft)
                action = VR::Paths::LEFT_HAND_AIM;
            else if (guiMode)
                action = VR::Paths::RIGHT_HAND_AIM;
            if (!action.empty())
                source = mXRInput->getSpace(action);
        }
        else
            source = mXRInput->getSpace(OpenXRInput::DefaultReferenceSpaceView);

        if (!mVRPointer && !disableControls)
        {
            osg::ref_ptr<osgViewer::Viewer> viewer;
            mOSGViewer.lock(viewer);
            if (viewer)
            {
                mVRPointer = std::make_unique<UserPointer>(viewer->getSceneData()->asGroup());
            }
        }

        if (mVRPointer)
        {
            mVRPointer->setSource(source);
            mVRPointer->update();
        }
    }

    void VRInputManager::updateRealisticCombat(float dt)
    {
        bool guiMode = MWBase::Environment::get().getWindowManager()->isGuiMode();

        if (!guiMode)
        {
            auto world = MWBase::Environment::get().getWorld();

            auto& player = world->getPlayer();
            auto playerPtr = world->getPlayerPtr();
            if (!mRealisticCombat || mRealisticCombat->ptr() != playerPtr)
            {
                // TODO: de-hardcode right-handedness for when ability to equip weapons with left hand is completed
                mRealisticCombat.reset(new RealisticCombat::StateMachine(playerPtr, VR::Paths::RIGHT_HAND_AIM));
            }
            bool enabled = !guiMode && player.getDrawState() == MWMechanics::DrawState::Weapon && !player.isDisabled();
            mRealisticCombat->update(dt, enabled);
        }
        else if (mRealisticCombat)
            mRealisticCombat->update(dt, false);

        auto ptr = MWBase::Environment::get().getWorld()->getPlayerPtr();
        auto* anim = MWBase::Environment::get().getWorld()->getAnimation(ptr);
        auto* vrAnim = static_cast<MWVR::VRAnimation*>(anim);
        mVRAimNode = vrAnim->getWeaponTransform();
    }

    MWWorld::Ptr VRInputManager::getPointerTarget() const
    {
        // TODO: In the future, dehardcode drag and drop as well
        if (!MWBase::Environment::get().getWindowManager()->getDragAndDrop().mIsOnDragAndDrop)
        {
            const auto& ray = mVRPointer->getPointerRay();
            if (ray.mHit)
            {
                return ray.mHitObject;
            }
        }
        return MWWorld::Ptr();
    }

    void VRInputManager::injectChannelValue(MWInput::Actions action, float value)
    {
        auto channel = mBindingsManager->ics().getChannel(MWInput::A_MoveLeftRight); // ->setValue(value);
        channel->setEnabled(true);
    }

    int VRInputManager::interactiveMessageBox(const std::string& message, const std::vector<std::string>& buttons)
    {
        auto wm = MWBase::Environment::get().getWindowManager();

        wm->enterVoid();

        wm->interactiveMessageBox(message, buttons, true);

        wm->exitVoid();

        return wm->readPressedButton();
    }

    //void VRInputManager::updatePhysicalSneak(Stereo::Unit headsetHeight)
    //{
    //    // Do physical sneak toggle if necessary
    //    const auto playerHeight = VR::Session::instance().playerHeight();
    //    if (mPhysicalSneakEnabled && VR::getStandingPlay() && playerHeight.asMeters() > 0.0f)
    //    {
    //        mIsPhysicalSneak = headsetHeight < playerHeight - mPhysicalSneakHeightOffset;
    //    }
    //}

    void VRInputManager::setPointerLeft(bool enabled)
    {
        mPointerLeft = enabled;
    }

    bool VRInputManager::getPointerLeft() const
    {
        return mPointerLeft;
    }

    void VRInputManager::setPointerRight(bool enabled)
    {
        mPointerRight = enabled;
    }

    bool VRInputManager::getPointerRight() const
    {
        return mPointerRight;
    }

    void VRInputManager::pointerActivate(bool injectMouseClickIfApplicable)
    {
        if (injectMouseClickIfApplicable && MWVR::VRGUIManager::instance().hasFocus())
        {
            if (MWVR::VRGUIManager::instance().injectMouseClick())
                return;

            SDL_MouseButtonEvent arg;
            mMouseManager->mousePressed(arg, SDL_BUTTON_LEFT);
            mMouseManager->mouseReleased(arg, SDL_BUTTON_LEFT);
            return;
        }

        if (mVRPointer)
            mVRPointer->activate();
    }

    static VRInputManager* sInputManager;

    VRInputManager::VRInputManager(SDL_Window* window, osg::ref_ptr<osgViewer::Viewer> viewer,
        osg::ref_ptr<osgViewer::ScreenCaptureHandler> screenCaptureHandler,
        const std::filesystem::path& userFile, bool userFileExists,
        const std::filesystem::path& userControllerBindingsFile, const std::filesystem::path& controllerBindingsFile,
        bool grab)
        : MWInput::InputManager(window, viewer, screenCaptureHandler, userFile, userFileExists,
            userControllerBindingsFile, controllerBindingsFile, grab)
        , mOSGViewer(viewer)
        , mVRPointer(nullptr)
        , mXRInput(new OpenXRInput())
    {

        sInputManager = this;
    }

    VRInputManager::~VRInputManager() {}

    VRInputManager& VRInputManager::instance()
    {
        assert(sInputManager);
        return *sInputManager;
    }

    void VRInputManager::changeInputMode(bool mode)
    {
        // VR mode has no concept of these
        // mGuiCursorEnabled = false;
        MWInput::InputManager::changeInputMode(mode);
        MWBase::Environment::get().getWindowManager()->showCrosshair(false);
    }

    void VRInputManager::update(float dt, bool disableControls, bool disableEvents)
    {
        mDt = dt;
        updateVRPointer(disableControls);

        if (MWVR::VRGUIManager::instance().hasFocus())
        {
            auto guiCursor = MWVR::VRGUIManager::instance().guiCursor();
            mMouseManager->setMousePosition(guiCursor.x(), guiCursor.y());
        }

        if (!VR::getKBMouseModeActive())
        {
            auto& actionSet = mXRInput->getActionSet(MWActionSet::Actions);
            actionSet.update();
        }

        MWInput::InputManager::update(dt, disableControls, disableEvents);

        // The rest of this code assumes the game is running
        if (MWBase::Environment::get().getStateManager()->getState() == MWBase::StateManager::State_NoGame)
            return;

        bool guiMode = MWBase::Environment::get().getWindowManager()->isGuiMode();

        // OpenMW assumes all input will come via SDL which i often violate.
        // This keeps player controls correctly enabled for my purposes.
        mBindingsManager->setPlayerControlsEnabled(!guiMode);
    }

    void VRInputManager::onSpaceUpdate() 
    {
        if (!VR::getKBMouseModeActive()
            && MWBase::Environment::get().getStateManager()->getState() == MWBase::StateManager::State_Running)
            updateRealisticCombat(mDt);
    }

}
