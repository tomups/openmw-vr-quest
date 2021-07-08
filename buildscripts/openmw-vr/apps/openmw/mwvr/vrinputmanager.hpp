#ifndef VR_INPUT_MANAGER_HPP
#define VR_INPUT_MANAGER_HPP

#include "vrtypes.hpp"
#include "vrinput.hpp"

#include "../mwinput/inputmanagerimp.hpp"

#include <vector>
#include <array>
#include <iostream>

#include "../mwworld/ptr.hpp"

class TiXmlElement;

namespace MWVR
{
    struct OpenXRInput;
    struct OpenXRActionSet;

    namespace RealisticCombat {
        class StateMachine;
    }

    /// Extension of the input manager to include VR inputs
    class VRInputManager : public MWInput::InputManager
    {
    public:
        VRInputManager(
            SDL_Window* window,
            osg::ref_ptr<osgViewer::Viewer> viewer,
            osg::ref_ptr<osgViewer::ScreenCaptureHandler> screenCaptureHandler,
            osgViewer::ScreenCaptureHandler::CaptureOperation* screenCaptureOperation,
            const std::string& userFile, bool userFileExists,
            const std::string& userControllerBindingsFile,
            const std::string& controllerBindingsFile, bool grab,
            const std::string& xrControllerSuggestionsFile);

        virtual ~VRInputManager();

        /// Overriden to force vr modes such as hiding cursors and crosshairs
        void changeInputMode(bool guiMode) override;

        /// Overriden to update XR inputs
        void update(float dt, bool disableControls = false, bool disableEvents = false) override;

        /// Set current offset to 0 and re-align VR stage.
        void requestRecenter(bool resetZ);

        /// Tracking pose of the given limb at the given predicted time
        Pose getLimbPose(int64_t time, TrackedLimb limb);

        /// Currently active action set
        OpenXRActionSet& activeActionSet();

        /// Notify input manager that the active interaction profile has changed
        void notifyInteractionProfileChanged();

        /// OpenXR input interface
        OpenXRInput& xrInput() { return *mXRInput; }

    protected:
        void processAction(const class Action* action, float dt, bool disableControls);

        void updateActivationIndication(void);
        void pointActivation(bool onPress);

        void injectMousePress(int sdlButton, bool onPress);
        void injectChannelValue(MWInput::Actions action, float value);

        void applyHapticsLeftHand(float intensity) override;
        void applyHapticsRightHand(float intensity) override;
        void processChangedSettings(const std::set< std::pair<std::string, std::string> >& changed) override;

        void throwDocumentError(TiXmlElement* element, std::string error);
        std::string requireAttribute(TiXmlElement* element, std::string attribute);
        void readInteractionProfile(TiXmlElement* element);
        void readInteractionProfileActionSet(TiXmlElement* element, ActionSet actionSet, std::string profilePath);

        void setThumbstickDeadzone(float deadzoneRadius);

    private:
        std::shared_ptr<AxisAction::Deadzone> mAxisDeadzone{ new AxisAction::Deadzone };
        std::unique_ptr<OpenXRInput> mXRInput;
        std::unique_ptr<RealisticCombat::StateMachine> mRealisticCombat;
        std::string mXrControllerSuggestionsFile;
        bool mActivationIndication{ false };
        bool mHapticsEnabled{ true };

        std::map<std::string, std::string> mInteractionProfileLocalNames;
    };
}

#endif
