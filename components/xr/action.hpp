#ifndef XR_ACTION_HPP
#define XR_ACTION_HPP

#include <openxr/openxr.h>

#include <components/vr/constants.hpp>

#include <chrono>
#include <deque>
#include <memory>
#include <string>

#include <osg/Vec2f>

namespace XR
{
    enum class ControlType
    {
        Press, //! Action occurs on pressing a key or a value reaches a cutoff
        LongPress, //! Action occurs when a Press action is held for a certain amount of time
        Hold, //! Action occurs continuously when the key is pressed
        Axis1D, //! Action occurs continuously when the axis is active, and depends on the axis value
        Axis2D, //! Action occurs continuously when the axis is active, and depends on the axis value
    };

    XrPath subActionPath(VR::SubAction subAction);

    /// \brief C++ wrapper for the XrAction type
    class Action
    {
        Action(const Action&) = delete;
        Action& operator=(const Action&) = delete;

    public:
        Action(XrAction action, XrActionType actionType, const std::string& actionName, const std::string& localName);
        ~Action();

        XrAction xrAction() { return mXrAction; }

        bool getFloat2d(XrPath subactionPath, osg::Vec2f& value);
        bool getFloat(XrPath subactionPath, float& value);
        bool getBool(XrPath subactionPath, bool& value);
        bool getPoseIsActive(XrPath subactionPath);
        bool applyHaptics(XrPath subactionPath, float amplitude);

        XrAction mXrAction = XR_NULL_HANDLE;
        XrActionType mType;
        std::string mName;
        std::string mLocalName;
    };

    /// \brief Action for applying haptics
    class HapticsAction
    {
    public:
        HapticsAction(std::shared_ptr<Action> action, VR::SubAction subAction);

        //! Apply vibration at the given amplitude
        void apply(float amplitude);

        //! Convenience
        XrAction xrAction() { return mAction->xrAction(); }

        VR::SubAction subAction() const { return mSubAction; }

    private:
        std::shared_ptr<Action> mAction;
        VR::SubAction mSubAction;
        XrPath mSubActionPath;
        float mAmplitude{ 0.f };
    };

    /// \brief Action for capturing tracking information
    class PoseAction
    {
    public:
        PoseAction(std::shared_ptr<Action> action, VR::SubAction subAction);

        //! Convenience
        XrAction xrAction() { return mAction->xrAction(); }

        //! Action space
        XrSpace xrSpace() { return mXRSpace; }

        VR::SubAction subAction() const { return mSubAction; }

    private:
        std::shared_ptr<Action> mAction;
        VR::SubAction mSubAction;
        XrPath mSubActionPath;
        XrSpace mXRSpace;
    };

    /// \brief Generic action
    /// \sa ButtonPressAction ButtonLongPressAction ButtonHoldAction Axis1DAction
    class InputAction
    {
    public:
        InputAction(int openMWAction, std::shared_ptr<Action> action, VR::SubAction subAction);
        virtual ~InputAction(){}

        //! True if action changed to being released in the last update
        bool isActive() const { return mActive; }

        //! True if activation turned on this update (i.e. onPress)
        bool onActivate() const { return mOnActivate; }

        //! True if activation turned off this update (i.e. onRelease)
        bool onDeactivate() const { return mOnDeactivate; }

        //! OpenMW Action code of this action
        int openMWActionCode() const { return mOpenMWAction; }

        //! Current value of an axis or lever action
        float value() const { return mValue; }

        //! Current 2D value of a 2D axis.
        osg::Vec2f value2d() const { return mValue2d; }

        //! Previous value
        float previousValue() const { return mPrevious; }

        //! Previous 2D value
        osg::Vec2f previousValue2d() const { return mPrevious2d; }

        //! Update internal states. Note that subclasses maintain both mValue and mActivate to allow
        //! axis and press to subtitute one another.
        virtual void update() = 0;

        //! Determines if an action update should be queued
        virtual bool shouldQueue() const = 0;

        //! Convenience
        XrAction xrAction() { return mAction->xrAction(); }

        //! Update and queue action if applicable
        void updateAndQueue(std::deque<const InputAction*>& queue);

        VR::SubAction subAction() const { return mSubAction; }

    protected:
        std::shared_ptr<Action> mAction;
        int mOpenMWAction;
        VR::SubAction mSubAction;
        XrPath mSubActionPath;
        float mValue{ 0.f };
        osg::Vec2f mValue2d{ 0.f, 0.f };
        float mPrevious{ 0.f };
        osg::Vec2f mPrevious2d{ 0.f, 0.f };
        bool mActive{ false };
        bool mOnActivate{ false };
        bool mOnDeactivate{ false };
    };

    //! Action that activates once on release.
    //! Times out if the button is held longer than gHoldDelay.
    class ButtonPressAction : public InputAction
    {
    public:
        using InputAction::InputAction;

        static const XrActionType ActionType = XR_ACTION_TYPE_BOOLEAN_INPUT;

        void update() override;

        bool shouldQueue() const override { return onActivate() || onDeactivate(); }

        bool mPressed{ false };
        std::chrono::steady_clock::time_point mPressTime{};
        std::chrono::steady_clock::time_point mTimeout{};
    };

    //! Action that activates once on a long press
    //! The press time is the same as the timeout for a regular press, allowing keys with double roles.
    class ButtonLongPressAction : public InputAction
    {
    public:
        using InputAction::InputAction;

        static const XrActionType ActionType = XR_ACTION_TYPE_BOOLEAN_INPUT;

        void update() override;

        bool shouldQueue() const override { return onActivate() || onDeactivate(); }

        bool mPressed{ false };
        bool mActivated{ false };
        std::chrono::steady_clock::time_point mPressTime{};
        std::chrono::steady_clock::time_point mTimein{};
    };

    //! Action that is active whenever the button is pressed down.
    //! Useful for e.g. non-toggling sneak and automatically repeating actions
    class ButtonHoldAction : public InputAction
    {
    public:
        using InputAction::InputAction;

        static const XrActionType ActionType = XR_ACTION_TYPE_BOOLEAN_INPUT;

        void update() override;

        bool shouldQueue() const override { return true; }

        bool mPressed{ false };
    };

    class AxisDeadzone
    {
    public:
        void applyDeadzone(float& value);

        void setDeadzoneRadius(float deadzoneRadius);

    private:
        float mActiveRadiusInner{ 0.f };
        float mActiveRadiusOuter{ 1.f };
        float mActiveScale{ 1.f };
    };

    //! Action for axis actions, such as thumbstick axes or certain triggers/squeeze levers.
    //! Float axis are considered active whenever their magnitude is greater than gAxisEpsilon. This is useful
    //! as a touch subtitute on levers without touch.
    class Axis1DAction : public InputAction
    {
    public:
        Axis1DAction(int openMWAction, std::shared_ptr<Action> xrAction, VR::SubAction subAction,
            std::shared_ptr<AxisDeadzone> deadzone);

        static const XrActionType ActionType = XR_ACTION_TYPE_FLOAT_INPUT;

        void update() override;

        bool shouldQueue() const override { return true; }

        std::shared_ptr<AxisDeadzone> mDeadzone;
    };

    //! Action for 2D axis actions, such as thumbsticks and trackpads.
    //! Float axis are considered active whenever their magnitude is greater than gAxisEpsilon. This is useful
    //! as a touch subtitute on levers without touch.
    class Axis2DAction : public InputAction
    {
    public:
        Axis2DAction(int openMWAction, std::shared_ptr<Action> xrAction, VR::SubAction subAction,
            std::shared_ptr<AxisDeadzone> deadzone);

        static const XrActionType ActionType = XR_ACTION_TYPE_VECTOR2F_INPUT;

        void update() override;

        bool shouldQueue() const override { return true; }

        std::shared_ptr<AxisDeadzone> mDeadzone;
    };
}

#endif
