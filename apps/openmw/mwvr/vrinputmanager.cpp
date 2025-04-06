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

            bool leftHanded = Settings::Manager::getBool("left handed mode", "VR");
            std::string action = "";
            if (guiMode)
            {
                if (leftHanded)
                    action = VR::Paths::LEFT_HAND_AIM;
                else
                    action = VR::Paths::RIGHT_HAND_AIM;
            }
            else if (mPointerRight)
                action = VR::Paths::RIGHT_HAND_AIM;
            else if (mPointerLeft)
                action = VR::Paths::LEFT_HAND_AIM;
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

    void VRInputManager::updateCombat(float dt)
    {
        //if (!VR::getKBMouseModeActive())
        //    return updateRealisticCombat(dt);
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

    void VRInputManager::processChangedSettings(const std::set<std::pair<std::string, std::string>>& changed)
    {
        MWInput::InputManager::processChangedSettings(changed);

        for (Settings::CategorySettingVector::const_iterator it = changed.begin(); it != changed.end(); ++it)
        {
            //if (it->first == "VR" && it->second == "snap angle")
            //{
            //    mSnapAngle = Settings::Manager::getFloat("snap angle", "VR");
            //    Log(Debug::Verbose) << "Snap angle set to: " << mSnapAngle;
            //}
            //if (it->first == "VR" && it->second == "smooth turn rate")
            //{
            //    mSmoothTurnRate = Settings::Manager::getFloat("smooth turn rate", "VR");
            //    Log(Debug::Verbose) << "Smooth turning rate set to: " << mSmoothTurnRate;
            //}
            //if (it->first == "VR" && it->second == "smooth turning")
            //{
            //    mSmoothTurning = Settings::Manager::getBool("smooth turning", "VR");
            //    Log(Debug::Verbose) << "Smooth turning set to: " << mSmoothTurning;
            //}
            //if (it->first == "VR" && it->second == "haptics enabled")
            //{
            //    mHapticsEnabled = Settings::Manager::getBool("haptics enabled", "VR");
            //}
            //if (it->first == "VR" && it->second == "physical sneak enabled")
            //{
            //    mPhysicalSneakEnabled = Settings::Manager::getBool("physical sneak enabled", "VR");
            //}
            //if (it->first == "VR" && it->second == "physical sneak height offset")
            //{
            //    mPhysicalSneakHeightOffset
            //        = Stereo::Unit::fromMeters(Settings::Manager::getFloat("physical sneak height offset", "VR"));
            //}
            //if (it->first == "Input" && it->second == "joystick dead zone")
            //{
            //    setThumbstickDeadzone(Settings::Manager::getFloat("joystick dead zone", "Input"));
            //}
        }
    }

    //void VRInputManager::setThumbstickDeadzone(float deadzoneRadius)
    //{
    //    // TODO: Process deadzones in lua
    //    mXRInput->setThumbstickDeadzone(deadzoneRadius);
    //}

    //void VRInputManager::mouseMove(float x)
    //{
    //    // TODO: Process in lua?
    //    auto path = VR::stringToVRPath("/world/user");
    //    auto* stageToWorldBinding
    //        = static_cast<VR::StageToWorldBinding*>(VR::TrackingManager::instance().getTrackingSource(path));
    //    stageToWorldBinding->setWorldOrientation(x, true);
    //}

    // TODO: Process in lua?
    //void VRInputManager::turnLeftRight(float value, float previousValue, float dt)
    //{
    //    auto path = VR::stringToVRPath("/world/user");
    //    auto* stageToWorldBinding
    //        = static_cast<VR::StageToWorldBinding*>(VR::TrackingManager::instance().getTrackingSource(path));
    //    if (mSmoothTurning)
    //    {
    //        float yaw = osg::DegreesToRadians(value) * smoothTurnRate(dt);
    //        stageToWorldBinding->setWorldOrientation(yaw, true);
    //    }
    //    else
    //    {
    //        if (value > 0.9f && previousValue < 0.9f)
    //        {
    //            stageToWorldBinding->setWorldOrientation(osg::DegreesToRadians(mSnapAngle), true);
    //        }
    //        if (value < -0.9f && previousValue > -0.9f)
    //        {
    //            stageToWorldBinding->setWorldOrientation(-osg::DegreesToRadians(mSnapAngle), true);
    //        }
    //    }
    //}

    //float VRInputManager::smoothTurnRate(float dt) const
    //{
    //    return 360.f * mSmoothTurnRate * dt;
    //}

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
    //        // MERGETODO: Expose to Lua
    //        mIsPhysicalSneak = headsetHeight < playerHeight - mPhysicalSneakHeightOffset;
    //    }
    //}

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

        // TODO: Probably wanna rethink all of this
        //struct HeightListener : public VR::TrackingListener
        //{
        //    Stereo::Unit height = Stereo::Unit::fromMeters(1.8);
        //    bool receivedTrackingData = false;
        //    VR::VRPath path = VR::stringToVRPath("/stage/user/head/input/pose");

        //    void onTrackingUpdated(VR::TrackingManager& manager) override
        //    {
        //        auto pose = manager.locate(path);
        //        if (static_cast<int>(pose.status) > 0)
        //        {
        //            receivedTrackingData = true;
        //            height = pose.pose.position.mZ;
        //            Log(Debug::Verbose) << "Height Calibration: " << height.asMeters();
        //        }
        //    }
        //} heightListener;

        //Stereo::Unit height = Stereo::Unit::fromMeters(1.8);

        //int option = interactiveMessageBox(
        //    "To be able to accurately scale your height to the height of your character, OpenMW-VR needs to know how "
        //    "tall you are. Stand up straight, and then press the OK button to proceed. Or press cancel to use a "
        //    "default height of 1.8 meters. You can redo this calibration later in VR tab of the settings menu",
        //    { "OK", "Cancel" });

        //if (option == 0)
        //{
        //    height = heightListener.height;
        //}

        //Settings::Manager::setFloat("player height", "VR", height.asMeters());
        //Settings::Manager::setBool("intro sequence complete", "VR", true);
        //VR::Session::instance().computePlayerScale();
    }

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

    void VRInputManager::pointerActivate() 
    {
        if (MWVR::VRGUIManager::instance().hasFocus())
        {
            if (MWVR::VRGUIManager::instance().injectMouseClick())
                return;

            SDL_MouseButtonEvent arg;
            mMouseManager->mousePressed(arg, SDL_BUTTON_LEFT);
            mMouseManager->mouseReleased(arg, SDL_BUTTON_LEFT);
        }

        if (mVRPointer)
            mVRPointer->activate();
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
        //, mHapticsEnabled{ Settings::Manager::getBool("haptics enabled", "VR") }
        //, mSmoothTurning{ Settings::Manager::getBool("smooth turning", "VR") }
        //, mSnapAngle{ Settings::Manager::getFloat("snap angle", "VR") }
        //, mSmoothTurnRate{ Settings::Manager::getFloat("smooth turn rate", "VR") }
        //, mPhysicalSneakHeightOffset(
        //      Stereo::Unit::fromMeters(Settings::Manager::getFloat("physical sneak height offset", "VR")))
        //, mPhysicalSneakEnabled(Settings::Manager::getBool("physical sneak enabled", "VR"))
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
    }

}
