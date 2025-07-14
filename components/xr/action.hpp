#ifndef XR_ACTION_HPP
#define XR_ACTION_HPP

#include <openxr/openxr.h>

#include <components/vr/constants.hpp>
#include <components/xr/space.hpp>

#include <chrono>
#include <deque>
#include <memory>
#include <string>
#include <optional>
#include <variant>

#include <osg/Vec2f>

namespace XR
{
    XrPath subActionPath(VR::SubAction subAction);

    /// \brief C++ wrapper for the XrAction type
    class Action
    {
        Action(const Action&) = delete;
        Action& operator=(const Action&) = delete;

    public:
        Action(XrAction action, XrActionType actionType, const std::string& actionName, const std::string& localName);
        ~Action();

        XrAction xrAction() const { return mXrAction; }

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
        static const XrActionType ActionType = XR_ACTION_TYPE_VIBRATION_OUTPUT;

        HapticsAction(std::unique_ptr<Action> action);

        //! Apply vibration at the given amplitude
        void apply(float amplitude);

        //! Convenience
        XrAction xrAction() const { return mAction->xrAction(); }

    private:
        std::unique_ptr<Action> mAction;
        float mAmplitude{ 0.f };
    };

    /// \brief Generic action
    /// \sa ButtonPressAction ButtonLongPressAction ButtonHoldAction Axis1DAction
    class InputAction
    {
    public:
        using Value = std::variant<bool, float>;

        InputAction(std::unique_ptr<Action> action);
        virtual ~InputAction() {}

        //! Convenience
        XrAction xrAction() const { return mAction->xrAction(); }

        //! Convenience
        std::string xrActionName() const { return mAction->mLocalName; }

        std::optional<Value> value() const { return mValue; }

        virtual bool update() = 0;

    protected:
        std::unique_ptr<Action> mAction;
        std::optional<Value> mValue;
    };

    /// \brief Action for capturing tracking information
    class PoseAction : public InputAction
    {
    public:
        static const XrActionType ActionType = XR_ACTION_TYPE_POSE_INPUT;

        PoseAction(std::unique_ptr<Action> action);

        //! Convenience
        XrAction xrAction() const { return mAction->xrAction(); }

        bool update() override;

        std::shared_ptr<Space> createActionSpace(Stereo::Pose pose) const;

    private:
    };

    //! Action that activates once on release.
    //! Times out if the button is held longer than gHoldDelay.
    class BoolAction : public InputAction
    {
    public:
        using InputAction::InputAction;

        static const XrActionType ActionType = XR_ACTION_TYPE_BOOLEAN_INPUT;

        bool update() override;

    private:
        std::chrono::steady_clock::time_point mPressTime{};
        std::chrono::steady_clock::time_point mTimeout{};
    };

    //! Action that activates once on a long press
    //! The press time is the same as the timeout for a regular press, allowing keys with double roles.
    // class ButtonLongPressAction : public InputAction
    //{
    // public:
    //     using InputAction::InputAction;

    //    static const XrActionType ActionType = XR_ACTION_TYPE_BOOLEAN_INPUT;

    //    void update() override;

    //    bool shouldQueue() const override { return onActivate() || onDeactivate(); }

    //    bool mPressed{ false };
    //    bool mActivated{ false };
    //    std::chrono::steady_clock::time_point mPressTime{};
    //    std::chrono::steady_clock::time_point mTimein{};
    //};

    //! Action that is active whenever the button is pressed down.
    //! Useful for e.g. non-toggling sneak and automatically repeating actions
    // class ButtonHoldAction : public InputAction
    //{
    // public:
    //     using InputAction::InputAction;

    //    static const XrActionType ActionType = XR_ACTION_TYPE_BOOLEAN_INPUT;

    //    void update() override;

    //    bool shouldQueue() const override { return true; }

    //    bool mPressed{ false };
    //};

    // class AxisDeadzone
    //{
    // public:
    //     void applyDeadzone(float& value);

    //    void setDeadzoneRadius(float deadzoneRadius);

    // private:
    //     float mActiveRadiusInner{ 0.f };
    //     float mActiveRadiusOuter{ 1.f };
    //     float mActiveScale{ 1.f };
    // };

    //! Action for axis actions, such as thumbstick axes or certain triggers/squeeze levers.
    //! Float axis are considered active whenever their magnitude is greater than gAxisEpsilon. This is useful
    //! as a touch subtitute on levers without touch.
    class FloatAction : public InputAction
    {
    public:
        using InputAction::InputAction;

        static const XrActionType ActionType = XR_ACTION_TYPE_FLOAT_INPUT;

        bool update() override;

    private:
    };

    // Axis action is just a float action that goes from -1 to 1 instead of 0 to 1.
    using AxisAction = FloatAction;
}

#endif
