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
        auto source = mHeadWorldPath;
        if (VR::getLeftControllerActive() || VR::getRightControllerActive())
        {
            bool guiMode = MWBase::Environment::get().getWindowManager()->isGuiMode();

            if (!disableControls)
                MWBase::Environment::get().getWorld()->enableVRPointer(
                    (guiMode || mPointerLeft) && VR::getLeftControllerActive(),
                    (guiMode || mPointerRight) && VR::getRightControllerActive());

            bool leftHanded = Settings::Manager::getBool("left handed mode", "VR");

            if (guiMode)
            {
                if (leftHanded)
                    source = VR::getLeftControllerActive() ? mLeftHandWorldPath : mRightHandWorldPath;
                else
                    source = VR::getRightControllerActive() ? mRightHandWorldPath : mLeftHandWorldPath;
            }
            else if (mPointerLeft)
                source = mLeftHandWorldPath;
            else if (mPointerRight)
                source = mRightHandWorldPath;
            else
                source = 0;
        }

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
        }
    }

    void VRInputManager::updateCombat(float dt)
    {
        if (!VR::getKBMouseModeActive())
            return updateRealisticCombat(dt);
        else
        {
            MWBase::Environment::get().getWorld()->setWeaponPosePath(mHeadWorldPath);
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
                auto trackingPath = VR::stringToVRPath("/stage/user/hand/right/input/aim/pose");
                mRealisticCombat.reset(new RealisticCombat::StateMachine(playerPtr, trackingPath));
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
        MWBase::Environment::get().getWorld()->setWeaponPosePath(0);
    }

    void VRInputManager::pointActivation(bool onPress, bool injectMousePress)
    {
        if (controlsDisabled() || MWVR::VRGUIManager::instance().hasFocus())
        {
            if (!MWVR::VRGUIManager::instance().injectMouseClick(onPress) && injectMousePress)
            {
                SDL_MouseButtonEvent arg;
                if (onPress)
                    mMouseManager->mousePressed(arg, SDL_BUTTON_LEFT);
                else
                    mMouseManager->mouseReleased(arg, SDL_BUTTON_LEFT);
            }
        }

        if (mVRPointer && !onPress)
            mVRPointer->activate();
    }

    void VRInputManager::injectChannelValue(MWInput::Actions action, float value)
    {
        auto channel = mBindingsManager->ics().getChannel(MWInput::A_MoveLeftRight); // ->setValue(value);
        channel->setEnabled(true);
    }

    void VRInputManager::processChangedSettings(const std::set<std::pair<std::string, std::string>>& changed)
    {
        MWInput::InputManager::processChangedSettings(changed);

        for (Settings::CategorySettingVector::const_iterator it = changed.begin(); it != changed.end(); ++it)
        {
            if (it->first == "VR" && it->second == "snap angle")
            {
                mSnapAngle = Settings::Manager::getFloat("snap angle", "VR");
                Log(Debug::Verbose) << "Snap angle set to: " << mSnapAngle;
            }
            if (it->first == "VR" && it->second == "smooth turn rate")
            {
                mSmoothTurnRate = Settings::Manager::getFloat("smooth turn rate", "VR");
                Log(Debug::Verbose) << "Smooth turning rate set to: " << mSmoothTurnRate;
            }
            if (it->first == "VR" && it->second == "smooth turning")
            {
                mSmoothTurning = Settings::Manager::getBool("smooth turning", "VR");
                Log(Debug::Verbose) << "Smooth turning set to: " << mSmoothTurning;
            }
            if (it->first == "VR" && it->second == "haptics enabled")
            {
                mHapticsEnabled = Settings::Manager::getBool("haptics enabled", "VR");
            }
            if (it->first == "VR" && it->second == "physical sneak enabled")
            {
                mPhysicalSneakEnabled = Settings::Manager::getBool("physical sneak enabled", "VR");
            }
            if (it->first == "VR" && it->second == "physical sneak height offset")
            {
                mPhysicalSneakHeightOffset
                    = Stereo::Unit::fromMeters(Settings::Manager::getFloat("physical sneak height offset", "VR"));
            }
            if (it->first == "Input" && it->second == "joystick dead zone")
            {
                setThumbstickDeadzone(Settings::Manager::getFloat("joystick dead zone", "Input"));
            }
        }
    }

    void VRInputManager::setThumbstickDeadzone(float deadzoneRadius)
    {
        mXRInput->setThumbstickDeadzone(deadzoneRadius);
    }

    void VRInputManager::mouseMove(float x)
    {
        auto path = VR::stringToVRPath("/world/user");
        auto* stageToWorldBinding
            = static_cast<VR::StageToWorldBinding*>(VR::TrackingManager::instance().getTrackingSource(path));
        stageToWorldBinding->setWorldOrientation(x, true);
    }

    void VRInputManager::turnLeftRight(float value, float previousValue, float dt)
    {
        auto path = VR::stringToVRPath("/world/user");
        auto* stageToWorldBinding
            = static_cast<VR::StageToWorldBinding*>(VR::TrackingManager::instance().getTrackingSource(path));
        if (mSmoothTurning)
        {
            float yaw = osg::DegreesToRadians(value) * smoothTurnRate(dt);
            stageToWorldBinding->setWorldOrientation(yaw, true);
        }
        else
        {
            if (value > 0.9f && previousValue < 0.9f)
            {
                stageToWorldBinding->setWorldOrientation(osg::DegreesToRadians(mSnapAngle), true);
            }
            if (value < -0.9f && previousValue > -0.9f)
            {
                stageToWorldBinding->setWorldOrientation(-osg::DegreesToRadians(mSnapAngle), true);
            }
        }
    }

    float VRInputManager::smoothTurnRate(float dt) const
    {
        return 360.f * mSmoothTurnRate * dt;
    }

    int VRInputManager::interactiveMessageBox(const std::string& message, const std::vector<std::string>& buttons)
    {
        auto wm = MWBase::Environment::get().getWindowManager();

        wm->enterVoid();

        wm->interactiveMessageBox(message, buttons, true);

        wm->exitVoid();

        return wm->readPressedButton();
    }

    void VRInputManager::updatePhysicalSneak(Stereo::Unit headsetHeight)
    {
        // Do physical sneak toggle if necessary
        const auto playerHeight = VR::Session::instance().playerHeight();
        if (mPhysicalSneakEnabled && VR::getStandingPlay() && playerHeight.asMeters() > 0.0f)
        {
            // MERGETODO: Expose to Lua
            mIsPhysicalSneak = headsetHeight < playerHeight - mPhysicalSneakHeightOffset;
        }
    }

    void VRInputManager::calibrate()
    {
        updateVRPointer(false);

        if (!Settings::Manager::getBool("intro sequence complete", "VR"))
        {
            calibratePlayerHeight();
        }
    }
    void VRInputManager::calibratePlayerHeight()
    {

        struct HeightListener : public VR::TrackingListener
        {
            Stereo::Unit height = Stereo::Unit::fromMeters(1.8);
            bool receivedTrackingData = false;
            VR::VRPath path = VR::stringToVRPath("/stage/user/head/input/pose");

            void onTrackingUpdated(VR::TrackingManager& manager) override
            {
                auto pose = manager.locate(path);
                if (static_cast<int>(pose.status) > 0)
                {
                    receivedTrackingData = true;
                    height = pose.pose.position.mZ;
                    Log(Debug::Verbose) << "Height Calibration: " << height.asMeters();
                }
            }
        } heightListener;

        Stereo::Unit height = Stereo::Unit::fromMeters(1.8);

        int option = interactiveMessageBox(
            "To be able to accurately scale your height to the height of your character, OpenMW-VR needs to know how "
            "tall you are. Stand up straight, and then press the OK button to proceed. Or press cancel to use a "
            "default height of 1.8 meters. You can redo this calibration later in VR tab of the settings menu",
            { "OK", "Cancel" });

        if (option == 0)
        {
            height = heightListener.height;
        }

        Settings::Manager::setFloat("player height", "VR", height.asMeters());
        Settings::Manager::setBool("intro sequence complete", "VR", true);
        VR::Session::instance().computePlayerScale();
    }

    static VRInputManager* sInputManager;

    VRInputManager::VRInputManager(SDL_Window* window, osg::ref_ptr<osgViewer::Viewer> viewer,
        osg::ref_ptr<osgViewer::ScreenCaptureHandler> screenCaptureHandler,
        const std::filesystem::path& userFile, bool userFileExists,
        const std::filesystem::path& userControllerBindingsFile, const std::filesystem::path& controllerBindingsFile,
        bool grab, const std::filesystem::path& xrControllerSuggestionsFile,
        const std::filesystem::path& defaultXrControllerSuggestionsFile)
        : MWInput::InputManager(window, viewer, screenCaptureHandler, userFile, userFileExists,
            userControllerBindingsFile, controllerBindingsFile, grab)
        , mOSGViewer(viewer)
        , mVRPointer(nullptr)
        , mXRInput(new OpenXRInput(xrControllerSuggestionsFile, defaultXrControllerSuggestionsFile))
        , mHapticsEnabled{ Settings::Manager::getBool("haptics enabled", "VR") }
        , mSmoothTurning{ Settings::Manager::getBool("smooth turning", "VR") }
        , mSnapAngle{ Settings::Manager::getFloat("snap angle", "VR") }
        , mSmoothTurnRate{ Settings::Manager::getFloat("smooth turn rate", "VR") }
        , mPhysicalSneakHeightOffset(
              Stereo::Unit::fromMeters(Settings::Manager::getFloat("physical sneak height offset", "VR")))
        , mPhysicalSneakEnabled(Settings::Manager::getBool("physical sneak enabled", "VR"))
        , mLeftHandPath(VR::stringToVRPath("/user/hand/left"))
        , mLeftHandWorldPath(VR::stringToVRPath("/world/user/hand/left/input/aim/pose"))
        , mRightHandPath(VR::stringToVRPath("/user/hand/right"))
        , mRightHandWorldPath(VR::stringToVRPath("/world/user/hand/right/input/aim/pose"))
        , mHeadWorldPath(VR::stringToVRPath("/world/user/head/input/pose"))
    {
        setThumbstickDeadzone(Settings::Manager::getFloat("joystick dead zone", "Input"));

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

        updateCombat(dt);

        // Update tracking every frame if player is not currently in GUI mode.
        // This ensures certain widgets like Notifications will be visible.
        if (!guiMode)
        {
            MWVR::VRGUIManager::instance().resetStationaryPose();
        }
    }

    void VRInputManager::processAction(const XR::InputAction* action, float dt, bool disableControls)
    {
        auto wm = MWBase::Environment::get().getWindowManager();

        // OpenMW does not currently provide any way to directly request skipping a video.
        // This is copied from the controller manager and is used to skip videos,
        // and works because mygui only consumes the escape press if a video is currently playing.
        if (wm->isPlayingVideo())
        {
            auto kc = SDLUtil::sdlKeyToMyGUI(SDLK_ESCAPE);
            if (action->onActivate())
            {
                mBindingsManager->setPlayerControlsEnabled(!MyGUI::InputManager::getInstance().injectKeyPress(kc, 0));
            }
            else if (action->onDeactivate())
            {
                mBindingsManager->setPlayerControlsEnabled(!MyGUI::InputManager::getInstance().injectKeyRelease(kc));
            }
        }

        bool guiMode = MWBase::Environment::get().getWindowManager()->isGuiMode();

        if (guiMode)
        {
            MyGUI::KeyCode key = MyGUI::KeyCode::None;
            bool onPress = true;

            // Axis actions
            switch (action->openMWActionCode())
            {
                case A_MenuLeftRight:
                    if (action->value() > 0.6f && action->previousValue() < 0.6f)
                    {
                        key = MyGUI::KeyCode::ArrowRight;
                    }
                    if (action->value() < -0.6f && action->previousValue() > -0.6f)
                    {
                        key = MyGUI::KeyCode::ArrowLeft;
                    }
                    if (action->value() < 0.6f && action->previousValue() > 0.6f)
                    {
                        key = MyGUI::KeyCode::ArrowRight;
                        onPress = false;
                    }
                    if (action->value() > -0.6f && action->previousValue() < -0.6f)
                    {
                        key = MyGUI::KeyCode::ArrowLeft;
                        onPress = false;
                    }
                    break;
                case A_MenuUpDown:
                    if (action->value() > 0.6f && action->previousValue() < 0.6f)
                    {
                        key = MyGUI::KeyCode::ArrowUp;
                    }
                    if (action->value() < -0.6f && action->previousValue() > -0.6f)
                    {
                        key = MyGUI::KeyCode::ArrowDown;
                    }
                    if (action->value() < 0.6f && action->previousValue() > 0.6f)
                    {
                        key = MyGUI::KeyCode::ArrowUp;
                        onPress = false;
                    }
                    if (action->value() > -0.6f && action->previousValue() < -0.6f)
                    {
                        key = MyGUI::KeyCode::ArrowDown;
                        onPress = false;
                    }
                    break;
                case A_UtilityStick:
                    processUtilityStickX(action->value2d().x(), disableControls);
                    processUtilityStickY(action->value2d().y(), disableControls);
                    break;
                default:
                    break;
            }

            // OnActivate actions
            if (action->onActivate())
            {
                switch (action->openMWActionCode())
                {
                    case MWInput::A_GameMenu:
                        mActionManager->toggleMainMenu();
                        break;
                    case A_MenuSelect:
                        wm->injectKeyPress(MyGUI::KeyCode::Return, 0, false);
                        break;
                    case A_MenuBack:
                        if (MyGUI::InputManager::getInstance().isModalAny())
                        {
                            wm->exitCurrentModal();
                        }
                        else
                        {
                            // Console/postprocessorhud do not respect the GUI stack
                            // so i have to manually check for and close them.
                            if (wm->isConsoleMode())
                                wm->toggleConsole();
                            else if (wm->isPostProcessorHudVisible())
                                wm->togglePostProcessorHud();
                            else
                                wm->exitCurrentGuiMode();
                        }
                        break;
                    case MWInput::A_Screenshot:
                        mActionManager->screenshot();
                        break;
                    case A_Recenter:
                        MWVR::VRGUIManager::instance().resetStationaryPose();
                        break;
                    case MWInput::A_Use:
                        pointActivation(true);
                        break;
                    default:
                        break;
                }
            }

            // A few actions need to fire on deactivation
            if (action->onDeactivate())
            {
                switch (action->openMWActionCode())
                {
                    case MWInput::A_Use:
                        mBindingsManager->ics().getChannel(MWInput::A_Use)->setValue(0.f);
                        pointActivation(false);
                        break;
                    case A_MenuSelect:
                        wm->injectKeyRelease(MyGUI::KeyCode::Return);
                        break;
                    default:
                        break;
                }
            }

            if (key != MyGUI::KeyCode::None)
            {
                if (onPress)
                {
                    MWBase::Environment::get().getWindowManager()->injectKeyPress(key, 0, 0);
                }
                else
                {
                    MWBase::Environment::get().getWindowManager()->injectKeyRelease(key);
                }
            }
        }

        else
        {
            if (disableControls)
            {
                return;
            }

            // Hold actions
            switch (action->openMWActionCode())
            {
                case A_ActivateTouch:
                    resetIdleTime();

                    if (action->subAction() == VR::SubAction::HandLeft)
                        mPointerLeft = action->isActive();
                    if (action->subAction() == VR::SubAction::HandRight)
                        mPointerRight = action->isActive();
                    break;
                case MWInput::A_LookLeftRight:
                {
                    turnLeftRight(action->value(), action->previousValue(), dt);
                    break;
                }
                case MWInput::A_MoveLeftRight:
                    mBindingsManager->ics()
                        .getChannel(MWInput::A_MoveLeftRight)
                        ->setValue(action->value() / 2.f + 0.5f);
                    break;
                case MWInput::A_MoveForwardBackward:
                    mBindingsManager->ics()
                        .getChannel(MWInput::A_MoveForwardBackward)
                        ->setValue(-action->value() / 2.f + 0.5f);
                    break;
                case MWInput::A_Sneak:
                {
                    mBindingsManager->ics().getChannel(MWInput::A_Sneak)->setValue(action->isActive() ? 1.f : 0.f);
                    break;
                }
                case MWInput::A_Use:
                    if (!(mPointerLeft || mPointerRight || MWBase::Environment::get().getWindowManager()->isGuiMode()))
                    {
                        mBindingsManager->ics().getChannel(MWInput::A_Use)->setValue(action->value());
                    }
                    break;

                case A_MovementStick:
                    processMovementStick(action, dt, disableControls);
                    break;
                case A_UtilityStick:
                    processUtilityStickX(action->value2d().x(), disableControls);
                    processUtilityStickY(action->value2d().y(), disableControls);
                    break;

                default:
                    break;
            }

            if (action->onActivate())
                onActivateAction(action->openMWActionCode(), disableControls);

            if (action->onDeactivate())
                onDeactivateAction(action->openMWActionCode(), disableControls);
        }
    }

    void VRInputManager::processMovementStick(const XR::InputAction* action, float dt, bool disableControls)
    {
        mBindingsManager->ics().getChannel(MWInput::A_MoveLeftRight)->setValue(action->value2d().x() / 2.f + 0.5f);
        mBindingsManager->ics()
            .getChannel(MWInput::A_MoveForwardBackward)
            ->setValue(-action->value2d().y() / 2.f + 0.5f);
    }

    void VRInputManager::processUtilityStickX(float value, [[maybe_unused]] bool disableControls)
    {
        mBindingsManager->ics().getChannel(MWInput::A_LookLeftRight)->setValue(value / 2.f + 0.5f);
    }

    void VRInputManager::processUtilityStickY(float value, bool disableControls)
    {
        if (value < -0.9 && !mUtilityDownActive)
            toggleUtilityDown(disableControls);
        if (value > -0.5 && mUtilityDownActive)
            toggleUtilityDown(disableControls);
        if (value > 0.9 && !mUtilityUpActive)
            toggleUtilityUp(disableControls);
        if (value < 0.5 && mUtilityUpActive)
            toggleUtilityUp(disableControls);
    }

    void VRInputManager::toggleUtilityDown(bool disableControls)
    {
        mUtilityDownActive = !mUtilityDownActive;
        auto actionName = Settings::Manager::getString("utility axis down action", "VR");
        auto actionId = mXRInput->actionCodeFromName(actionName);
        if (actionId != 0)
        {
            if (mUtilityDownActive)
                onActivateAction(actionId, disableControls);
            else
                onDeactivateAction(actionId, disableControls);
        }
    }

    void VRInputManager::toggleUtilityUp(bool disableControls)
    {
        mUtilityUpActive = !mUtilityUpActive;
        auto actionName = Settings::Manager::getString("utility axis up action", "VR");
        auto actionId = mXRInput->actionCodeFromName(actionName);
        if (actionId != -1)
        {
            if (mUtilityUpActive)
                onActivateAction(actionId, disableControls);
            else
                onDeactivateAction(actionId, disableControls);
        }
    }

    void VRInputManager::onActivateAction(int actionId, [[maybe_unused]] bool disableControls)
    {
        switch (actionId)
        {
            case MWInput::A_ToggleThumbstickAutoRun:
                mControllerManager->setThumbstickAutoRun(!mControllerManager->thumbstickAutoRun());
                break;
            case A_VrMetaMenu:
                MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_VrMetaMenu);
                break;
            case A_RadialMenu:
                MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_RadialMenu);
                break;
            case A_Recenter:
                if (!MWBase::Environment::get().getWindowManager()->isGuiMode())
                {
                    VR::recenter();
                    VR::resetEyeLevel();
                }
                break;
            case MWInput::A_Use:
                if (mPointerLeft || mPointerRight || MWBase::Environment::get().getWindowManager()->isGuiMode())
                    pointActivation(true);
                break;
            default:
                if (actionId >= MWInput::A_Last)
                    break;
                // For these i ignore disableControls, the binding manager handles that already.
                auto channel = mBindingsManager->ics().getChannel(actionId);
                if (channel)
                    channel->setValue(1.0);
                break;
        }
    }

    void VRInputManager::onDeactivateAction(int actionId, [[maybe_unused]] bool disableControls)
    {
        switch (actionId)
        {
            case MWInput::A_Use:
                mBindingsManager->ics().getChannel(MWInput::A_Use)->setValue(0.f);
                if (mPointerLeft || mPointerRight || MWBase::Environment::get().getWindowManager()->isGuiMode())
                    pointActivation(false);
                break;
            case A_Recenter:
                break;
            case A_ToggleSneak:
                break;
            default:
                if (actionId >= MWInput::A_Last)
                    break;
                auto channel = mBindingsManager->ics().getChannel(actionId);
                if (channel)
                    channel->setValue(0.0);
                break;
        }
    }

    void VRInputManager::HeightUpdateListener::onTrackingUpdated(
        VR::TrackingManager& manager)
    {
        auto tpHead = manager.locate(mHeadPath);
        if (!!tpHead.status)
        {
            instance().updatePhysicalSneak(tpHead.pose.position.mZ);
        }
    }

}
