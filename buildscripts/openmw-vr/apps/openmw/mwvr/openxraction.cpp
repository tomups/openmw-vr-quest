#include "openxraction.hpp"
#include "openxrdebug.hpp"
#include "vrenvironment.hpp"
#include "openxrmanagerimpl.hpp"

namespace MWVR
{

    OpenXRAction::OpenXRAction(
        XrAction action,
        XrActionType actionType,
        const std::string& actionName,
        const std::string& localName)
        : mAction(action)
        , mType(actionType)
        , mName(actionName)
        , mLocalName(localName)
    {
        VrDebug::setName(action, "OpenMW XR Action " + actionName);
    };

    OpenXRAction::~OpenXRAction() {
        if (mAction)
        {
            xrDestroyAction(mAction);
        }
    }

    bool OpenXRAction::getFloat(XrPath subactionPath, float& value)
    {
        auto* xr = Environment::get().getManager();
        XrActionStateGetInfo getInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
        getInfo.action = mAction;
        getInfo.subactionPath = subactionPath;

        XrActionStateFloat xrValue{ XR_TYPE_ACTION_STATE_FLOAT };
        CHECK_XRCMD(xrGetActionStateFloat(xr->impl().xrSession(), &getInfo, &xrValue));

        if (xrValue.isActive)
            value = xrValue.currentState;
        return xrValue.isActive;
    }

    bool OpenXRAction::getBool(XrPath subactionPath, bool& value)
    {
        auto* xr = Environment::get().getManager();
        XrActionStateGetInfo getInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
        getInfo.action = mAction;
        getInfo.subactionPath = subactionPath;

        XrActionStateBoolean xrValue{ XR_TYPE_ACTION_STATE_BOOLEAN };
        CHECK_XRCMD(xrGetActionStateBoolean(xr->impl().xrSession(), &getInfo, &xrValue));

        if (xrValue.isActive)
            value = xrValue.currentState;
        return xrValue.isActive;
    }

    // Pose action only checks if the pose is active or not
    bool OpenXRAction::getPoseIsActive(XrPath subactionPath)
    {
        auto* xr = Environment::get().getManager();
        XrActionStateGetInfo getInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
        getInfo.action = mAction;
        getInfo.subactionPath = subactionPath;

        XrActionStatePose xrValue{ XR_TYPE_ACTION_STATE_POSE };
        CHECK_XRCMD(xrGetActionStatePose(xr->impl().xrSession(), &getInfo, &xrValue));

        return xrValue.isActive;
    }

    bool OpenXRAction::applyHaptics(XrPath subactionPath, float amplitude)
    {
        amplitude = std::max(0.f, std::min(1.f, amplitude));

        auto* xr = Environment::get().getManager();
        XrHapticVibration vibration{ XR_TYPE_HAPTIC_VIBRATION };
        vibration.amplitude = amplitude;
        vibration.duration = XR_MIN_HAPTIC_DURATION;
        vibration.frequency = XR_FREQUENCY_UNSPECIFIED;

        XrHapticActionInfo hapticActionInfo{ XR_TYPE_HAPTIC_ACTION_INFO };
        hapticActionInfo.action = mAction;
        hapticActionInfo.subactionPath = subactionPath;
        CHECK_XRCMD(xrApplyHapticFeedback(xr->impl().xrSession(), &hapticActionInfo, (XrHapticBaseHeader*)&vibration));
        return true;
    }
}
