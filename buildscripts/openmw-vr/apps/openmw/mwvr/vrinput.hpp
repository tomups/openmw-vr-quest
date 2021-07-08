#ifndef VR_INPUT_HPP
#define VR_INPUT_HPP

#include "vrviewer.hpp"
#include "openxraction.hpp"

#include "../mwinput/inputmanagerimp.hpp"

namespace MWVR
{
    /// Extension of MWInput's set of actions.
    enum VrActions
    {
        A_VrFirst = MWInput::A_Last + 1,
        A_VrMetaMenu,
        A_ActivateTouch,
        A_HapticsLeft,
        A_HapticsRight,
        A_HandPoseLeft,
        A_HandPoseRight,
        A_MenuUpDown,
        A_MenuLeftRight,
        A_MenuSelect,
        A_MenuBack,
        A_Recenter,
        A_VrLast
    };

    enum class VrControlType
    {
        Press,
        LongPress,
        Hold,
        Pose,
        Haptic,
        Axis
    };

    /// \brief Suggest a binding by binding an action to a path on a given hand (left or right).
    struct SuggestedBinding
    {
        std::string path;
        std::string action;
    };

    using SuggestedBindings = std::vector<SuggestedBinding>;

    /// \brief Enumeration of action sets
    enum class ActionSet
    {
        GUI = 0,
        Gameplay = 1,
        Tracking = 2,
        Haptics = 3,
    };

    /// \brief Action for applying haptics
    class HapticsAction
    {
    public:
        HapticsAction(std::unique_ptr<OpenXRAction> xrAction) : mXRAction{ std::move(xrAction) } {};

        //! Apply vibration at the given amplitude
        void apply(float amplitude);

        //! Convenience
        operator XrAction() { return *mXRAction; }

    private:
        std::unique_ptr<OpenXRAction> mXRAction;
        float mAmplitude{ 0.f };
    };

    /// \brief Action for capturing tracking information
    class PoseAction
    {
    public:
        PoseAction(std::unique_ptr<OpenXRAction> xrAction);

        //! Current value of an axis or lever action
        Pose value() const { return mValue; }

        //! Previous value
        Pose previousValue() const { return mPrevious; }

        //! Convenience
        operator XrAction() { return *mXRAction; }

        //! Update pose value
        void update(long long time);

        //! Action space
        XrSpace xrSpace() { return mXRSpace; }

    private:
        std::unique_ptr<OpenXRAction> mXRAction;
        XrSpace mXRSpace;
        Pose mValue{};
        Pose mPrevious{};
    };

    /// \brief Generic action
    /// \sa ButtonPressAction ButtonLongPressAction ButtonHoldAction AxisAction
    class Action
    {
    public:
        Action(int openMWAction, std::unique_ptr<OpenXRAction> xrAction) : mXRAction(std::move(xrAction)), mOpenMWAction(openMWAction) {}
        virtual ~Action() {};

        //! True if action changed to being released in the last update
        bool isActive() const { return mActive; };

        //! True if activation turned on this update (i.e. onPress)
        bool onActivate() const { return mOnActivate; }

        //! True if activation turned off this update (i.e. onRelease)
        bool onDeactivate() const { return mOnDeactivate; }

        //! OpenMW Action code of this action
        int openMWActionCode() const { return mOpenMWAction; }

        //! Current value of an axis or lever action
        float value() const { return mValue; }

        //! Previous value
        float previousValue() const { return mPrevious; }

        //! Update internal states. Note that subclasses maintain both mValue and mActivate to allow
        //! axis and press to subtitute one another.
        virtual void update() = 0;

        //! Determines if an action update should be queued
        virtual bool shouldQueue() const = 0;

        //! Convenience
        operator XrAction() { return *mXRAction; }

        //! Update and queue action if applicable
        void updateAndQueue(std::deque<const Action*>& queue);

    protected:
        std::unique_ptr<OpenXRAction> mXRAction;
        int mOpenMWAction;
        float mValue{ 0.f };
        float mPrevious{ 0.f };
        bool mActive{ false };
        bool mOnActivate{ false };
        bool mOnDeactivate{ false };
    };

    //! Action that activates once on release.
    //! Times out if the button is held longer than gHoldDelay.
    class ButtonPressAction : public Action
    {
    public:
        using Action::Action;

        static const XrActionType ActionType = XR_ACTION_TYPE_BOOLEAN_INPUT;

        void update() override;

        virtual bool shouldQueue() const override { return onActivate() || onDeactivate(); }

        bool mPressed{ false };
        std::chrono::steady_clock::time_point mPressTime{};
        std::chrono::steady_clock::time_point mTimeout{};
    };

    //! Action that activates once on a long press
    //! The press time is the same as the timeout for a regular press, allowing keys with double roles.
    class ButtonLongPressAction : public Action
    {
    public:
        using Action::Action;

        static const XrActionType ActionType = XR_ACTION_TYPE_BOOLEAN_INPUT;

        void update() override;

        virtual bool shouldQueue() const override { return onActivate() || onDeactivate(); }

        bool mPressed{ false };
        bool mActivated{ false };
        std::chrono::steady_clock::time_point mPressTime{};
        std::chrono::steady_clock::time_point mTimein{};
    };

    //! Action that is active whenever the button is pressed down.
    //! Useful for e.g. non-toggling sneak and automatically repeating actions
    class ButtonHoldAction : public Action
    {
    public:
        using Action::Action;

        static const XrActionType ActionType = XR_ACTION_TYPE_BOOLEAN_INPUT;

        void update() override;

        virtual bool shouldQueue() const override { return mActive || onDeactivate(); }

        bool mPressed{ false };
    };

    //! Action for axis actions, such as thumbstick axes or certain triggers/squeeze levers.
    //! Float axis are considered active whenever their magnitude is greater than gAxisEpsilon. This is useful
    //! as a touch subtitute on levers without touch.
    class AxisAction : public Action
    {
    public:
        class Deadzone
        {
        public:
            void applyDeadzone(float& value);

            void setDeadzoneRadius(float deadzoneRadius);

        private:
            float mActiveRadiusInner{ 0.f };
            float mActiveRadiusOuter{ 1.f };
            float mActiveScale{ 1.f };
        };

    public:
        AxisAction(int openMWAction, std::unique_ptr<OpenXRAction> xrAction, std::shared_ptr<AxisAction::Deadzone> deadzone);

        static const XrActionType ActionType = XR_ACTION_TYPE_FLOAT_INPUT;

        void update() override;

        virtual bool shouldQueue() const override { return mActive || onDeactivate(); }

        std::shared_ptr<AxisAction::Deadzone> mDeadzone;
    };
}

#endif
