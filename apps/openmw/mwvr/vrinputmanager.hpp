#ifndef VR_INPUT_MANAGER_HPP
#define VR_INPUT_MANAGER_HPP

#include "../mwinput/inputmanagerimp.hpp"

#include <vector>

#include <components/vr/trackinglistener.hpp>
#include <components/vr/session.hpp>
#include <components/vr/vr.hpp>

#include "../mwworld/ptr.hpp"

namespace XR
{
    class ActionSet;
    class InputAction;
}

namespace MWVR
{
    class OpenXRInput;
    class UserPointer;

    namespace RealisticCombat
    {
        class StateMachine;
    }

    /// Extension of the input manager to include VR inputs
    class VRInputManager : public MWInput::InputManager, private VR::Session::Listener
    {
    public:
        VRInputManager(SDL_Window* window, osg::ref_ptr<osgViewer::Viewer> viewer,
            osg::ref_ptr<osgViewer::ScreenCaptureHandler> screenCaptureHandler,
            const std::filesystem::path& userFile, bool userFileExists,
            const std::filesystem::path& userControllerBindingsFile,
            const std::filesystem::path& controllerBindingsFile, bool grab);

        virtual ~VRInputManager();

        static VRInputManager& instance();

        /// Overriden to force vr modes such as hiding cursors and crosshairs
        void changeInputMode(bool guiMode) override;

        /// Overriden to update XR inputs
        void update(float dt, bool disableControls = false, bool disableEvents = false) override;

        /// Update realistic combat
        void onSpaceUpdate() override;

        /// OpenXR input interface
        OpenXRInput& xrInput() { return *mXRInput; }

        UserPointer* vrPointer() { return mVRPointer.get(); }
        const UserPointer* vrPointer() const { return mVRPointer.get(); }

        osg::Node* vrAimNode() { return mVRAimNode; }
        const osg::Node* vrAimNode() const { return mVRAimNode; }

        void setPointerLeft(bool enabled);
        bool getPointerLeft() const;
        void setPointerRight(bool enabled);
        bool getPointerRight() const;
        void pointerActivate(bool injectMouseClickIfApplicable);
        void pointerActivateDelayed(bool injectMouseClickIfApplicable);

        MWWorld::Ptr getPointerTarget() const;

        void setScrollSpeed(float speed);
        void setGuiCursor(int x, int y);

    protected:
        void updateVRPointer(bool disableControls);
        void updateRealisticCombat(float dt);

        void injectChannelValue(MWInput::Actions action, float value);

        int interactiveMessageBox(const std::string& message, const std::vector<std::string>& buttons);

    private:
        osg::observer_ptr<osgViewer::Viewer> mOSGViewer;
        std::unique_ptr<UserPointer> mVRPointer;
        std::unique_ptr<OpenXRInput> mXRInput;
        std::unique_ptr<RealisticCombat::StateMachine> mRealisticCombat;
        //bool mPointerActivation = false;
        bool mPointerLeft = true;
        bool mPointerRight = true;
        bool mDelayedPointerActivate = false;
        bool mDelayedPointerActivateInjectMouseClickIfApplicable = false;
        float mDt = 0.f;
        float mScrollSpeed = 0.f;
        float mScrollPoints = 0.f;
        osg::ref_ptr<osg::Node> mVRAimNode;
    };
}

#endif
