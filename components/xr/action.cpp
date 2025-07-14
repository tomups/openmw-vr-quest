#include "action.hpp"
#include "session.hpp"
#include <components/xr/debug.hpp>
#include <components/xr/instance.hpp>
#include <components/xr/typeconversion.hpp>

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
    //static std::chrono::milliseconds gActionTime{ 666 };
    //! Magnitude above which an axis action is considered active
    //static float gAxisEpsilon{ 0.01f };

    HapticsAction::HapticsAction(std::unique_ptr<Action> action)
        : mAction{ std::move(action) }
    {
    }

    void HapticsAction::apply(float amplitude)
    {
        mAmplitude = std::max(0.f, std::min(1.f, amplitude));
        mAction->applyHaptics(XR_NULL_PATH, mAmplitude);
    }

    PoseAction::PoseAction(std::unique_ptr<Action> action)
        : InputAction(std::move(action))
    {
    }

    bool PoseAction::update()
    {
        if (mAction->getPoseIsActive(XR_NULL_PATH))
            mValue = true;
        else
            mValue = std::nullopt;
        return false;
    }

    std::shared_ptr<Space> PoseAction::createActionSpace(Stereo::Pose pose) const
    {
        XrSpace space = XR_NULL_HANDLE;
        XrActionSpaceCreateInfo createInfo{};
        createInfo.type = XR_TYPE_ACTION_SPACE_CREATE_INFO;
        createInfo.action = mAction->xrAction();
        createInfo.poseInActionSpace = XR::toXR(pose);
        createInfo.subactionPath = XR_NULL_PATH;
        CHECK_XRCMD(xrCreateActionSpace(XR::Session::instance().xrSession(), &createInfo, &space));
        if (space)
            return std::make_shared<Space>(space);
        return nullptr;
    }

    InputAction::InputAction(std::unique_ptr<Action> action)
        : mAction(std::move(action))
    {
    }

    bool BoolAction::update()
    {
        bool value = false;
        bool ret = false;
        bool active = mAction->getBool(XR_NULL_PATH, value);
        if (active)
        {
            ret = !std::get<bool>(mValue.value_or(false)) && value;
            mValue = value;
        }
        else
            mValue = std::nullopt;
        return ret;
    }

    bool FloatAction::update()
    {
        float value = 0.f;
        bool ret = false;
        bool active = mAction->getFloat(XR_NULL_PATH, value);
        if (active)
        {
            ret = (std::get<float>(mValue.value_or(0.f)) < 0.6f) && (value > 0.6f);
            mValue = value;
        }
        else
            mValue = std::nullopt;
        return ret;
    }

    //void AxisDeadzone::applyDeadzone(float& value)
    //{
    //    float sign = std::copysignf(1.f, value);
    //    float magnitude = std::fabs(value);
    //    magnitude = std::min(mActiveRadiusOuter, magnitude);
    //    magnitude = std::max(0.f, magnitude - mActiveRadiusInner);
    //    value = sign * magnitude * mActiveScale;
    //}

    //void AxisDeadzone::setDeadzoneRadius(float deadzoneRadius)
    //{
    //    deadzoneRadius = std::min(std::max(deadzoneRadius, 0.0f), 0.5f - 1e-5f);
    //    mActiveRadiusInner = deadzoneRadius;
    //    mActiveRadiusOuter = 1.f - deadzoneRadius;
    //    float activeRadius = mActiveRadiusOuter - mActiveRadiusInner;
    //    assert(activeRadius > 0.f);
    //    mActiveScale = 1.f / activeRadius;
    //}

    //Axis2DAction::Axis2DAction(int openMWAction, std::shared_ptr<Action> xrAction,
    //    std::shared_ptr<AxisDeadzone> deadzone)
    //    : InputAction(openMWAction, std::move(xrAction))
    //    , mDeadzone(deadzone)
    //{
    //}

    //void Axis2DAction::update()
    //{
    //    mActive = false;
    //    mAction->getFloat2d(XR_NULL_PATH, mValue2d);
    //    mDeadzone->applyDeadzone(mValue2d.x());
    //    mDeadzone->applyDeadzone(mValue2d.y());

    //    float primary = mValue2d.x();
    //    if (std::fabs(primary) < std::fabs(mValue2d.y()))
    //        primary = mValue2d.y();

    //    if (std::fabs(primary) > gAxisEpsilon)
    //    {
    //        mActive = true;
    //        mValue = primary;
    //    }
    //    else
    //    {
    //        mValue2d.x() = mValue2d.y() = mValue = 0.f;
    //    }
    //}
}
