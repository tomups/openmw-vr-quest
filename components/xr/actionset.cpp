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

    ActionSet::ActionSet(const std::string& actionSetName)
        : mActionSet(nullptr)
        , mLocalizedName(actionSetName)
        , mInternalName(Misc::StringUtils::lowerCase(actionSetName))
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

    //void ActionSet::createPoseAction(
    //    const std::string& actionName, const std::string& localName, std::vector<VR::SubAction> subActions)
    //{
    //    auto action = createXRAction(XR_ACTION_TYPE_POSE_INPUT, actionName, localName, subActions);

    //    if (subActions.size() == 0)
    //        subActions = { VR::SubAction::ALL };
    //    for (auto subAction : subActions)
    //    {
    //        mTrackerMap.emplace(subAction, new PoseAction(action, subAction));
    //    }
    //}

    //void ActionSet::createHapticsAction(
    //    const std::string& actionName, const std::string& localName, std::vector<VR::SubAction> subActions)
    //{
    //    auto action = createXRAction(XR_ACTION_TYPE_VIBRATION_OUTPUT, actionName, localName, subActions);

    //    if (subActions.size() == 0)
    //        subActions = { VR::SubAction::ALL };
    //    for (auto subAction : subActions)
    //    {
    //        mHapticsMap.emplace(subAction, new HapticsAction(action, subAction));
    //    }
    //}

    //template <>
    //void ActionSet::createMWAction<Axis1DAction>(int openMWAction, const std::string& actionName,
    //    const std::string& localName, std::vector<VR::SubAction> subActions)
    //{
    //}

    //template <>
    //void ActionSet::createMWAction<Axis2DAction>(int openMWAction, const std::string& actionName,
    //    const std::string& localName, std::vector<VR::SubAction> subActions)
    //{
    //    auto action = createXRAction(
    //        Axis2DAction::ActionType, mInternalName + "_" + actionName, mLocalizedName + " " + localName, subActions);

    //    if (subActions.size() == 0)
    //        subActions = { VR::SubAction::ALL };
    //    for (auto subAction : subActions)
    //    {
    //        mInputActionMap.emplace(
    //            std::pair(openMWAction, subAction), new Axis2DAction(openMWAction, action, subAction, mDeadzone));
    //    }
    //}

    //template <typename A>
    //void ActionSet::createMWAction(int openMWAction, const std::string& actionName, const std::string& localName,
    //    std::vector<VR::SubAction> subActions)
    //{
    //    auto action = createXRAction(
    //        A::ActionType, mInternalName + "_" + actionName, mLocalizedName + " " + localName, subActions);

    //    if (subActions.size() == 0)
    //        subActions = { VR::SubAction::ALL };
    //    for (auto subAction : subActions)
    //    {
    //        mInputActionMap.emplace(std::pair(openMWAction, subAction), new A(openMWAction, action, subAction));
    //    }
    //}

    //void ActionSet::createMWAction(ControlType controlType, int openMWAction, const std::string& actionName,
    //    const std::string& localName, std::vector<VR::SubAction> subActions)
    //{
    //    switch (controlType)
    //    {
    //        case ControlType::Press:
    //            return createMWAction<ButtonPressAction>(openMWAction, actionName, localName, subActions);
    //        case ControlType::LongPress:
    //            return createMWAction<ButtonLongPressAction>(openMWAction, actionName, localName, subActions);
    //        case ControlType::Hold:
    //            return createMWAction<ButtonHoldAction>(openMWAction, actionName, localName, subActions);
    //        case ControlType::Axis1D:
    //            return createMWAction<Axis1DAction>(openMWAction, actionName, localName, subActions);
    //        case ControlType::Axis2D:
    //            return createMWAction<Axis2DAction>(openMWAction, actionName, localName, subActions);
    //        default:
    //            Log(Debug::Warning) << "createMWAction: pose/haptics Not implemented here";
    //    }
    //}

    void ActionSet::createBoolAction(const std::string& actionName, const std::string& localName, const std::string& id)
    {
        mBoolActions.emplace(id, createXRAction(BoolAction::ActionType, actionName, localName));
        mAllInputActions.emplace(id, &mBoolActions.at(id));
    }

    void ActionSet::createAxisAction(const std::string& actionName, const std::string& localName, const std::string& id)
    {
        mAxisActions.emplace(id, createXRAction(AxisAction::ActionType, actionName, localName));
        mAllInputActions.emplace(id, &mAxisActions.at(id));
    }

    void ActionSet::createFloatAction(
        const std::string& actionName, const std::string& localName, const std::string& id)
    {
        mFloatActions.emplace(id, createXRAction(FloatAction::ActionType, actionName, localName));
        mAllInputActions.emplace(id, &mFloatActions.at(id));
    }

    void ActionSet::createPoseAction(const std::string& actionName, const std::string& localName, const std::string& id)
    {
        mPoseActions.emplace(id, createXRAction(PoseAction::ActionType, actionName, localName));
        mAllInputActions.emplace(id, &mPoseActions.at(id));
    }

    void ActionSet::createHapticsAction(
        const std::string& actionName, const std::string& localName, const std::string& id)
    {
        mHapticsActions.emplace(id, createXRAction(HapticsAction::ActionType, actionName, localName));
    }

    std::optional<InputAction::Value> ActionSet::getValue(const std::string& id) const
    {
        return mAllInputActions.at(id)->value();
    }

    void ActionSet::suggestBinding(const std::string& id, const std::string& path)
    {
        mSuggestions.push_back(std::make_pair(id, path));
    }

    XrAction ActionSet::findXrAction(const std::string& id) const
    {
        auto it = mAllInputActions.find(id);
        if (it != mAllInputActions.end())
            return it->second->xrAction();
        return mHapticsActions.at(id).xrAction();
    }

    std::shared_ptr<VR::Space> ActionSet::createActionSpace(
        const std::string& spaceId, const std::string& actionId, Stereo::Pose pose)
    {
        auto space = mPoseActions.at(actionId).createActionSpace(pose);
        XR::Debugging::setName(space->xrSpace(), "OpenMW XR Action Space " + spaceId);
        return space;
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

    std::vector<XrActionSuggestedBinding> ActionSet::suggestBindings()
    {
        std::vector<XrActionSuggestedBinding> suggestions;

        for (auto& suggestion : mSuggestions)
        {
            XrActionSuggestedBinding s = {};
            s.action = findXrAction(suggestion.first);
            if (s.action)
            {
                s.binding = getXrPath(suggestion.second);
                suggestions.emplace_back(s);
            }
        }

        //auto suggest = [&](auto& actions) {
        //    for (auto& action : actions)
        //        suggestions.emplace_back(XrActionSuggestedBinding{ action.second.xrAction(), getXrPath(action.first) });
        //};

        //suggest(mAxisActions);
        //suggest(mBoolActions);
        //suggest(mFloatActions);
        //suggest(mPoseActions);
        //suggest(mHapticsActions);

        return suggestions;

        //std::vector<XrActionSuggestedBinding> suggestedBindings;
        //if (!mTrackerMap.empty())
        //{
        //    suggestedBindings.emplace_back(XrActionSuggestedBinding{
        //        mTrackerMap[VR::SubAction::HandLeft]->xrAction(), getXrPath("/user/hand/left/input/aim/pose") });
        //    suggestedBindings.emplace_back(XrActionSuggestedBinding{
        //        mTrackerMap[VR::SubAction::HandRight]->xrAction(), getXrPath("/user/hand/right/input/aim/pose") });
        //}
        //if (!mHapticsMap.empty())
        //{
        //    suggestedBindings.emplace_back(XrActionSuggestedBinding{
        //        mHapticsMap[VR::SubAction::HandLeft]->xrAction(), getXrPath("/user/hand/left/output/haptic") });
        //    suggestedBindings.emplace_back(XrActionSuggestedBinding{
        //        mHapticsMap[VR::SubAction::HandRight]->xrAction(), getXrPath("/user/hand/right/output/haptic") });
        //};

        //for (auto& mwSuggestedBinding : mwSuggestedBindings)
        //{
        //    auto xrAction = mActionMap.find(mInternalName + "_" + mwSuggestedBinding.action);
        //    if (xrAction == mActionMap.end())
        //    {
        //        Log(Debug::Error) << "ActionSet: Unknown action " << mwSuggestedBinding.action;
        //        continue;
        //    }
        //    suggestedBindings.push_back({ xrAction->second->xrAction(), getXrPath(mwSuggestedBinding.path) });
        //}

        //xrSuggestedBindings.insert(xrSuggestedBindings.end(), suggestedBindings.begin(), suggestedBindings.end());
    }

    std::unique_ptr<XR::Action> ActionSet::createXRAction(XrActionType actionType, const std::string& actionName,
        const std::string& localName)
    {
        XrActionCreateInfo createInfo{};
        createInfo.type = XR_TYPE_ACTION_CREATE_INFO;
        createInfo.actionType = actionType;
        Misc::StringUtils::copyCppStringToCArray(createInfo.actionName, actionName);
        Misc::StringUtils::copyCppStringToCArray(createInfo.localizedActionName, localName);
        createInfo.countSubactionPaths = 0;
        Log(Debug::Verbose) << "Creating action: " << actionName;

        XrAction xrAction = XR_NULL_HANDLE;
        CHECK_XRCMD(xrCreateAction(mActionSet, &createInfo, &xrAction));
        return std::make_unique<XR::Action>(xrAction, actionType, actionName, localName);
    }

    bool ActionSet::update()
    {
        const XrActiveActionSet activeActionSet{ mActionSet, XR_NULL_PATH };
        XrActionsSyncInfo syncInfo{};
        syncInfo.type = XR_TYPE_ACTIONS_SYNC_INFO;
        syncInfo.countActiveActionSets = 1;
        syncInfo.activeActionSets = &activeActionSet;
        bool inputChanged = false;

        auto res = xrSyncActions(XR::Session::instance().xrSession(), &syncInfo);
        if (XR_FAILED(res))
        {
            CHECK_XRRESULT(res, "xrSyncActions");
            return false;
        }
        else if (res == XR_SUCCESS)
        {
            for (auto& action : mAllInputActions)
                inputChanged = inputChanged || action.second->update();
        }
        else
        {
            // Do nothing if xrSyncActions returned XR_SESSION_NOT_FOCUSED or XR_SESSION_LOSS_PENDING
        }

        return inputChanged;
    }

    XrPath ActionSet::getXrPath(const std::string& path) const
    {
        XrPath xrpath = 0;
        CHECK_XRCMD(xrStringToPath(XR::Instance::instance().xrInstance(), path.c_str(), &xrpath));
        return xrpath;
    }

    void ActionSet::applyHaptics(const std::string& id, float intensity)
    {
        if (!mHapticsActions.contains(id))
            throw std::runtime_error("No such haptics: " + id);

        mHapticsActions.at(id).apply(intensity);
    }
}
