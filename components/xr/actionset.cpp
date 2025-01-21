#include "actionset.hpp"
#include "session.hpp"

#include <openxr/openxr.h>

#include <components/debug/debuglog.hpp>
#include <components/misc/strings/lower.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/xr/debug.hpp>
#include <components/xr/instance.hpp>

#include <iostream>
#include <string.h>

namespace XR
{

    ActionSet::ActionSet(const std::string& actionSetName, std::shared_ptr<AxisDeadzone> deadzone)
        : mActionSet(nullptr)
        , mLocalizedName(actionSetName)
        , mInternalName(Misc::StringUtils::lowerCase(actionSetName))
        , mDeadzone(deadzone)
    {
        mActionSet = createActionSet(actionSetName);
    }

    ActionSet::~ActionSet()
    {
        if (mActionSet)
        {
            CHECK_XRCMD(xrDestroyActionSet(mActionSet));
        }
    }

    void ActionSet::createPoseAction(
        const std::string& actionName, const std::string& localName, std::vector<VR::SubAction> subActions)
    {
        auto action = createXRAction(XR_ACTION_TYPE_POSE_INPUT, actionName, localName, subActions);

        if (subActions.size() == 0)
            subActions = { VR::SubAction::ALL };
        for (auto subAction : subActions)
        {
            mTrackerMap.emplace(subAction, new PoseAction(action, subAction));
        }
    }

    void ActionSet::createHapticsAction(
        const std::string& actionName, const std::string& localName, std::vector<VR::SubAction> subActions)
    {
        auto action = createXRAction(XR_ACTION_TYPE_VIBRATION_OUTPUT, actionName, localName, subActions);

        if (subActions.size() == 0)
            subActions = { VR::SubAction::ALL };
        for (auto subAction : subActions)
        {
            mHapticsMap.emplace(subAction, new HapticsAction(action, subAction));
        }
    }

    template <>
    void ActionSet::createMWAction<Axis1DAction>(int openMWAction, const std::string& actionName,
        const std::string& localName, std::vector<VR::SubAction> subActions)
    {
        auto action = createXRAction(
            Axis1DAction::ActionType, mInternalName + "_" + actionName, mLocalizedName + " " + localName, subActions);

        if (subActions.size() == 0)
            subActions = { VR::SubAction::ALL };
        for (auto subAction : subActions)
        {
            mInputActionMap.emplace(
                std::pair(openMWAction, subAction), new Axis1DAction(openMWAction, action, subAction, mDeadzone));
        }
    }

    template <>
    void ActionSet::createMWAction<Axis2DAction>(int openMWAction, const std::string& actionName,
        const std::string& localName, std::vector<VR::SubAction> subActions)
    {
        auto action = createXRAction(
            Axis2DAction::ActionType, mInternalName + "_" + actionName, mLocalizedName + " " + localName, subActions);

        if (subActions.size() == 0)
            subActions = { VR::SubAction::ALL };
        for (auto subAction : subActions)
        {
            mInputActionMap.emplace(
                std::pair(openMWAction, subAction), new Axis2DAction(openMWAction, action, subAction, mDeadzone));
        }
    }

    template <typename A>
    void ActionSet::createMWAction(int openMWAction, const std::string& actionName, const std::string& localName,
        std::vector<VR::SubAction> subActions)
    {
        auto action = createXRAction(
            A::ActionType, mInternalName + "_" + actionName, mLocalizedName + " " + localName, subActions);

        if (subActions.size() == 0)
            subActions = { VR::SubAction::ALL };
        for (auto subAction : subActions)
        {
            mInputActionMap.emplace(std::pair(openMWAction, subAction), new A(openMWAction, action, subAction));
        }
    }

    void ActionSet::createMWAction(ControlType controlType, int openMWAction, const std::string& actionName,
        const std::string& localName, std::vector<VR::SubAction> subActions)
    {
        switch (controlType)
        {
            case ControlType::Press:
                return createMWAction<ButtonPressAction>(openMWAction, actionName, localName, subActions);
            case ControlType::LongPress:
                return createMWAction<ButtonLongPressAction>(openMWAction, actionName, localName, subActions);
            case ControlType::Hold:
                return createMWAction<ButtonHoldAction>(openMWAction, actionName, localName, subActions);
            case ControlType::Axis1D:
                return createMWAction<Axis1DAction>(openMWAction, actionName, localName, subActions);
            case ControlType::Axis2D:
                return createMWAction<Axis2DAction>(openMWAction, actionName, localName, subActions);
            default:
                Log(Debug::Warning) << "createMWAction: pose/haptics Not implemented here";
        }
    }

    XrActionSet ActionSet::createActionSet(const std::string& name)
    {
        std::string localized_name = name;
        std::string internal_name = Misc::StringUtils::lowerCase(name);
        XrActionSet actionSet = XR_NULL_HANDLE;
        XrActionSetCreateInfo createInfo{};
        createInfo.type = XR_TYPE_ACTION_SET_CREATE_INFO;
        Misc::StringUtils::copyCppStringToCArray(createInfo.actionSetName, internal_name);
        Misc::StringUtils::copyCppStringToCArray(createInfo.localizedActionSetName, localized_name);
        createInfo.priority = 0;
        CHECK_XRCMD(xrCreateActionSet(XR::Instance::instance().xrInstance(), &createInfo, &actionSet));
        XR::Debugging::setName(actionSet, "OpenMW XR Action Set " + name);
        return actionSet;
    }

    void ActionSet::suggestBindings(
        std::vector<XrActionSuggestedBinding>& xrSuggestedBindings, const SuggestedBindings& mwSuggestedBindings)
    {
        std::vector<XrActionSuggestedBinding> suggestedBindings;
        if (!mTrackerMap.empty())
        {
            suggestedBindings.emplace_back(XrActionSuggestedBinding{
                mTrackerMap[VR::SubAction::HandLeft]->xrAction(), getXrPath("/user/hand/left/input/aim/pose") });
            suggestedBindings.emplace_back(XrActionSuggestedBinding{
                mTrackerMap[VR::SubAction::HandRight]->xrAction(), getXrPath("/user/hand/right/input/aim/pose") });
        }
        if (!mHapticsMap.empty())
        {
            suggestedBindings.emplace_back(XrActionSuggestedBinding{
                mHapticsMap[VR::SubAction::HandLeft]->xrAction(), getXrPath("/user/hand/left/output/haptic") });
            suggestedBindings.emplace_back(XrActionSuggestedBinding{
                mHapticsMap[VR::SubAction::HandRight]->xrAction(), getXrPath("/user/hand/right/output/haptic") });
        };

        for (auto& mwSuggestedBinding : mwSuggestedBindings)
        {
            auto xrAction = mActionMap.find(mInternalName + "_" + mwSuggestedBinding.action);
            if (xrAction == mActionMap.end())
            {
                Log(Debug::Error) << "ActionSet: Unknown action " << mwSuggestedBinding.action;
                continue;
            }
            suggestedBindings.push_back({ xrAction->second->xrAction(), getXrPath(mwSuggestedBinding.path) });
        }

        xrSuggestedBindings.insert(xrSuggestedBindings.end(), suggestedBindings.begin(), suggestedBindings.end());
    }

    XrSpace ActionSet::xrActionSpace(VR::SubAction side)
    {
        return mTrackerMap[side]->xrSpace();
    }

    std::shared_ptr<XR::Action> ActionSet::createXRAction(XrActionType actionType, const std::string& actionName,
        const std::string& localName, std::vector<VR::SubAction> subActions)
    {
        std::vector<XrPath> subactionPaths;
        subactionPaths.reserve(subActions.size());
        for (auto subAction : subActions)
            subactionPaths.push_back(subActionPath(subAction));

        XrActionCreateInfo createInfo{};
        createInfo.type = XR_TYPE_ACTION_CREATE_INFO;
        createInfo.actionType = actionType;
        Misc::StringUtils::copyCppStringToCArray(createInfo.actionName, actionName);
        Misc::StringUtils::copyCppStringToCArray(createInfo.localizedActionName, localName);
        createInfo.countSubactionPaths = subactionPaths.size();
        Log(Debug::Verbose) << "Creating action: " << actionName;
        if (createInfo.countSubactionPaths > 0)
        {
            createInfo.subactionPaths = subactionPaths.data();
        }

        XrAction xrAction = XR_NULL_HANDLE;
        CHECK_XRCMD(xrCreateAction(mActionSet, &createInfo, &xrAction));
        auto action = std::make_shared<XR::Action>(xrAction, actionType, actionName, localName);
        mActionMap.emplace(actionName, action);
        return action;
    }

    void ActionSet::updateControls(bool shouldQueueActions)
    {
        mActionQueue.clear();

        const XrActiveActionSet activeActionSet{ mActionSet, XR_NULL_PATH };
        XrActionsSyncInfo syncInfo{};
        syncInfo.type = XR_TYPE_ACTIONS_SYNC_INFO;
        syncInfo.countActiveActionSets = 1;
        syncInfo.activeActionSets = &activeActionSet;

        auto res = xrSyncActions(XR::Session::instance().xrSession(), &syncInfo);
        if (XR_FAILED(res))
        {
            CHECK_XRRESULT(res, "xrSyncActions");
            return;
        }
        else if (res == XR_SUCCESS)
        {
            for (auto& action : mInputActionMap)
                action.second->updateAndQueue(mActionQueue);
            if (!shouldQueueActions)
                mActionQueue.clear();
        }
        else
        {
            // Do nothing if xrSyncActions returned XR_SESSION_NOT_FOCUSED or XR_SESSION_LOSS_PENDING
        }
    }

    XrPath ActionSet::getXrPath(const std::string& path) const
    {
        XrPath xrpath = 0;
        CHECK_XRCMD(xrStringToPath(XR::Instance::instance().xrInstance(), path.c_str(), &xrpath));
        return xrpath;
    }

    const InputAction* ActionSet::nextAction()
    {
        if (mActionQueue.empty())
            return nullptr;

        const auto* action = mActionQueue.front();
        mActionQueue.pop_front();
        return action;
    }

    void ActionSet::applyHaptics(VR::SubAction side, float intensity)
    {
        auto it = mHapticsMap.find(side);
        if (it == mHapticsMap.end())
        {
            Log(Debug::Error) << "ActionSet: No such tracker: " << static_cast<int>(side);
            return;
        }

        it->second->apply(intensity);
    }
}
