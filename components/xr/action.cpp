#include "action.hpp"
#include "session.hpp"
#include <components/xr/debug.hpp>
#include <components/xr/instance.hpp>

#include <cassert>

namespace XR
{
    Action::Action(
        XrAction action, XrActionType actionType, const std::string& actionName, const std::string& localName)
        : mXrAction(action)
        , mType(actionType)
        , mName(actionName)
        , mLocalName(localName)
    {
        XR::Debugging::setName(action, "OpenMW XR Action " + actionName);
    }

    Action::~Action()
    {
        if (mXrAction)
        {
            CHECK_XRCMD(xrDestroyAction(mXrAction));
        }
    }

    bool Action::getFloat2d(XrPath subactionPath, osg::Vec2f& value)
    {
        XrActionStateGetInfo getInfo{};
        getInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
        getInfo.action = mXrAction;
        getInfo.subactionPath = subactionPath;

        XrActionStateVector2f xrValue{};
        xrValue.type = XR_TYPE_ACTION_STATE_VECTOR2F;
        CHECK_XRCMD(xrGetActionStateVector2f(XR::Session::instance().xrSession(), &getInfo, &xrValue));

        if (xrValue.isActive)
        {
            value.x() = xrValue.currentState.x;
            value.y() = xrValue.currentState.y;
        }
        return xrValue.isActive;
    }

    bool Action::getFloat(XrPath subactionPath, float& value)
    {
        XrActionStateGetInfo getInfo{};
        getInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
        getInfo.action = mXrAction;
        getInfo.subactionPath = subactionPath;

        XrActionStateFloat xrValue{};
        xrValue.type = XR_TYPE_ACTION_STATE_FLOAT;
        CHECK_XRCMD(xrGetActionStateFloat(XR::Session::instance().xrSession(), &getInfo, &xrValue));

        if (xrValue.isActive)
            value = xrValue.currentState;
        return xrValue.isActive;
    }

    bool Action::getBool(XrPath subactionPath, bool& value)
    {
        XrActionStateGetInfo getInfo{};
        getInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
        getInfo.action = mXrAction;
        getInfo.subactionPath = subactionPath;

        XrActionStateBoolean xrValue{};
        xrValue.type = XR_TYPE_ACTION_STATE_BOOLEAN;
        CHECK_XRCMD(xrGetActionStateBoolean(XR::Session::instance().xrSession(), &getInfo, &xrValue));

        if (xrValue.isActive)
        {
            value = xrValue.currentState;
        }
        return xrValue.isActive;
    }

    // Pose action only checks if the pose is active or not
    bool Action::getPoseIsActive(XrPath subactionPath)
    {
        XrActionStateGetInfo getInfo{};
        getInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
        getInfo.action = mXrAction;
        getInfo.subactionPath = subactionPath;

        XrActionStatePose xrValue{};
        xrValue.type = XR_TYPE_ACTION_STATE_POSE;
        CHECK_XRCMD(xrGetActionStatePose(XR::Session::instance().xrSession(), &getInfo, &xrValue));

        return xrValue.isActive;
    }

    bool Action::applyHaptics(XrPath subactionPath, float amplitude)
    {
        amplitude = std::max(0.f, std::min(1.f, amplitude));

        XrHapticVibration vibration{};
        vibration.type = XR_TYPE_HAPTIC_VIBRATION;
        vibration.amplitude = amplitude;
        vibration.duration = XR_MIN_HAPTIC_DURATION;
        vibration.frequency = XR_FREQUENCY_UNSPECIFIED;

        XrHapticActionInfo hapticActionInfo{};
        hapticActionInfo.type = XR_TYPE_HAPTIC_ACTION_INFO;
        hapticActionInfo.action = mXrAction;
        hapticActionInfo.subactionPath = subactionPath;
        CHECK_XRCMD(xrApplyHapticFeedback(
            XR::Session::instance().xrSession(), &hapticActionInfo, (XrHapticBaseHeader*)&vibration));
        return true;
    }

    //! Delay before a long-press action is activated (and regular press is discarded)
    //! TODO: Make this configurable?
    static std::chrono::milliseconds gActionTime{ 666 };
    //! Magnitude above which an axis action is considered active
    static float gAxisEpsilon{ 0.01f };

    HapticsAction::HapticsAction(std::shared_ptr<Action> action, VR::SubAction subAction)
        : mAction{ std::move(action) }
        , mSubAction(subAction)
        , mSubActionPath(subActionPath(subAction))
    {
    }

    void HapticsAction::apply(float amplitude)
    {
        mAmplitude = std::max(0.f, std::min(1.f, amplitude));
        mAction->applyHaptics(XR_NULL_PATH, mAmplitude);
    }

    PoseAction::PoseAction(std::shared_ptr<Action> action, VR::SubAction subAction)
        : mAction(std::move(action))
        , mSubAction(subAction)
        , mSubActionPath(subActionPath(subAction))
        , mXRSpace{ XR_NULL_HANDLE }
    {
        XrActionSpaceCreateInfo createInfo{};
        createInfo.type = XR_TYPE_ACTION_SPACE_CREATE_INFO;
        createInfo.action = mAction->xrAction();
        createInfo.poseInActionSpace.orientation.w = 1.f;
        createInfo.subactionPath = mSubActionPath;
        CHECK_XRCMD(xrCreateActionSpace(XR::Session::instance().xrSession(), &createInfo, &mXRSpace));
        XR::Debugging::setName(mXRSpace, "OpenMW XR Action Space " + mAction->mName);
    }

    InputAction::InputAction(int openMWAction, std::shared_ptr<Action> action, VR::SubAction subAction)
        : mAction(std::move(action))
        , mOpenMWAction(openMWAction)
        , mSubAction(subAction)
        , mSubActionPath(subActionPath(subAction))
    {
    }

    void InputAction::updateAndQueue(std::deque<const InputAction*>& queue)
    {
        bool old = mActive;
        mPrevious = mValue;
        mPrevious2d = mValue2d;
        update();
        bool changed = old != mActive;
        mOnActivate = changed && mActive;
        mOnDeactivate = changed && !mActive;

        if (shouldQueue())
        {
            queue.push_back(this);
        }
    }

    void ButtonPressAction::update()
    {
        mActive = false;
        bool old = mPressed;
        mAction->getBool(mSubActionPath, mPressed);
        bool changed = old != mPressed;
        if (changed && mPressed)
        {
            mPressTime = std::chrono::steady_clock::now();
            mTimeout = mPressTime + gActionTime;
        }
        if (changed && !mPressed)
        {
            if (std::chrono::steady_clock::now() < mTimeout)
            {
                mActive = true;
            }
        }

        mValue = mPressed ? 1.f : 0.f;
        mValue2d.x() = mValue2d.y() = mValue;
    }

    void ButtonLongPressAction::update()
    {
        mActive = false;
        bool old = mPressed;
        mAction->getBool(mSubActionPath, mPressed);
        bool changed = old != mPressed;
        if (changed && mPressed)
        {
            mPressTime = std::chrono::steady_clock::now();
            mTimein = mPressTime + gActionTime;
            mActivated = false;
        }
        if (mPressed && !mActivated)
        {
            if (std::chrono::steady_clock::now() >= mTimein)
            {
                mActive = mActivated = true;
                mValue = 1.f;
            }
        }
        if (changed && !mPressed)
        {
            mValue = 0.f;
        }
        mValue2d.x() = mValue2d.y() = mValue;
    }

    void ButtonHoldAction::update()
    {
        mAction->getBool(mSubActionPath, mPressed);
        mActive = mPressed;

        mValue = mPressed ? 1.f : 0.f;
        mValue2d.x() = mValue2d.y() = mValue;
    }

    Axis1DAction::Axis1DAction(int openMWAction, std::shared_ptr<XR::Action> action, VR::SubAction subAction,
        std::shared_ptr<AxisDeadzone> deadzone)
        : InputAction(openMWAction, std::move(action), subAction)
        , mDeadzone(deadzone)
    {
    }

    void Axis1DAction::update()
    {
        mActive = false;
        mAction->getFloat(mSubActionPath, mValue);
        mDeadzone->applyDeadzone(mValue);

        if (std::fabs(mValue) > gAxisEpsilon)
            mActive = true;
        else
            mValue = 0.f;

        mValue2d.x() = mValue2d.y() = mValue;
    }

    void AxisDeadzone::applyDeadzone(float& value)
    {
        float sign = std::copysignf(1.f, value);
        float magnitude = std::fabs(value);
        magnitude = std::min(mActiveRadiusOuter, magnitude);
        magnitude = std::max(0.f, magnitude - mActiveRadiusInner);
        value = sign * magnitude * mActiveScale;
    }

    void AxisDeadzone::setDeadzoneRadius(float deadzoneRadius)
    {
        deadzoneRadius = std::min(std::max(deadzoneRadius, 0.0f), 0.5f - 1e-5f);
        mActiveRadiusInner = deadzoneRadius;
        mActiveRadiusOuter = 1.f - deadzoneRadius;
        float activeRadius = mActiveRadiusOuter - mActiveRadiusInner;
        assert(activeRadius > 0.f);
        mActiveScale = 1.f / activeRadius;
    }

    XrPath subActionPath(VR::SubAction subAction)
    {
        const char* cpath = nullptr;
        switch (subAction)
        {
            case VR::SubAction::Gamepad:
                cpath = "/user/gamepad";
                break;
            case VR::SubAction::Head:
                cpath = "user/head";
                break;
            case VR::SubAction::HandLeft:
                cpath = "/user/hand/left";
                break;
            case VR::SubAction::HandRight:
                cpath = "/user/hand/right";
                break;
            default:
                return XR_NULL_PATH;
        }
        XrPath xrpath = 0;
        CHECK_XRCMD(xrStringToPath(XR::Instance::instance().xrInstance(), cpath, &xrpath));
        return xrpath;
    }

    Axis2DAction::Axis2DAction(int openMWAction, std::shared_ptr<Action> xrAction, VR::SubAction subAction,
        std::shared_ptr<AxisDeadzone> deadzone)
        : InputAction(openMWAction, std::move(xrAction), subAction)
        , mDeadzone(deadzone)
    {
    }

    void Axis2DAction::update()
    {
        mActive = false;
        mAction->getFloat2d(mSubActionPath, mValue2d);
        mDeadzone->applyDeadzone(mValue2d.x());
        mDeadzone->applyDeadzone(mValue2d.y());

        float primary = mValue2d.x();
        if (std::fabs(primary) < std::fabs(mValue2d.y()))
            primary = mValue2d.y();

        if (std::fabs(primary) > gAxisEpsilon)
        {
            mActive = true;
            mValue = primary;
        }
        else
        {
            mValue2d.x() = mValue2d.y() = mValue = 0.f;
        }
    }
}
