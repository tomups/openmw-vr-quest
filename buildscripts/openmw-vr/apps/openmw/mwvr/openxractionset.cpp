#include "openxractionset.hpp"
#include "openxrdebug.hpp"

#include "vrenvironment.hpp"
#include "openxrmanager.hpp"
#include "openxrmanagerimpl.hpp"
#include "openxraction.hpp"

#include <openxr/openxr.h>

#include <components/misc/stringops.hpp>

#include <iostream>

// TODO: should implement actual safe strcpy
#ifdef __linux__
#define strcpy_s(dst, src)   int(strcpy(dst, src) != nullptr)
#endif

namespace MWVR
{

    OpenXRActionSet::OpenXRActionSet(const std::string& actionSetName, std::shared_ptr<AxisAction::Deadzone> deadzone)
        : mActionSet(nullptr)
        , mLocalizedName(actionSetName)
        , mInternalName(Misc::StringUtils::lowerCase(actionSetName))
        , mDeadzone(deadzone)
    {
        mActionSet = createActionSet(actionSetName);
        // When starting to account for more devices than oculus touch, this section may need some expansion/redesign.
    };

    void
        OpenXRActionSet::createPoseAction(
            TrackedLimb limb,
            const std::string& actionName,
            const std::string& localName)
    {
        mTrackerMap.emplace(limb, new PoseAction(std::move(createXRAction(XR_ACTION_TYPE_POSE_INPUT, actionName, localName))));
    }

    void
        OpenXRActionSet::createHapticsAction(
            TrackedLimb limb,
            const std::string& actionName,
            const std::string& localName)
    {
        mHapticsMap.emplace(limb, new HapticsAction(std::move(createXRAction(XR_ACTION_TYPE_VIBRATION_OUTPUT, actionName, localName))));
    }

    template<>
    void
        OpenXRActionSet::createMWAction<AxisAction>(
            int openMWAction,
            const std::string& actionName,
            const std::string& localName)
    {
        auto xrAction = createXRAction(AxisAction::ActionType, mInternalName + "_" + actionName, mLocalizedName + " " + localName);
        mActionMap.emplace(actionName, new AxisAction(openMWAction, std::move(xrAction), mDeadzone));
    }

    template<typename A>
    void
        OpenXRActionSet::createMWAction(
            int openMWAction,
            const std::string& actionName,
            const std::string& localName)
    {
        auto xrAction = createXRAction(A::ActionType, mInternalName + "_" + actionName, mLocalizedName + " " + localName);
        mActionMap.emplace(actionName, new A(openMWAction, std::move(xrAction)));
    }


    void
        OpenXRActionSet::createMWAction(
            VrControlType controlType,
            int openMWAction,
            const std::string& actionName,
            const std::string& localName)
    {
        switch (controlType)
        {
        case VrControlType::Press:
            return createMWAction<ButtonPressAction>(openMWAction, actionName, localName);
        case VrControlType::LongPress:
            return createMWAction<ButtonLongPressAction>(openMWAction, actionName, localName);
        case VrControlType::Hold:
            return createMWAction<ButtonHoldAction>(openMWAction, actionName, localName);
        case VrControlType::Axis:
            return createMWAction<AxisAction>(openMWAction, actionName, localName);
        //case VrControlType::Pose:
        //    return createMWAction<PoseAction>(openMWAction, actionName, localName);
        //case VrControlType::Haptic:
        //    return createMWAction<HapticsAction>(openMWAction, actionName, localName);
        default:
            Log(Debug::Warning) << "createMWAction: pose/haptics Not implemented here";
        }
    }


    XrActionSet
        OpenXRActionSet::createActionSet(const std::string& name)
    {
        std::string localized_name = name;
        std::string internal_name = Misc::StringUtils::lowerCase(name);
        auto* xr = Environment::get().getManager();
        XrActionSet actionSet = XR_NULL_HANDLE;
        XrActionSetCreateInfo createInfo{ XR_TYPE_ACTION_SET_CREATE_INFO };
        strcpy_s(createInfo.actionSetName, internal_name.c_str());
        strcpy_s(createInfo.localizedActionSetName, localized_name.c_str());
        createInfo.priority = 0;
        CHECK_XRCMD(xrCreateActionSet(xr->impl().xrInstance(), &createInfo, &actionSet));
        VrDebug::setName(actionSet, "OpenMW XR Action Set " + name);
        return actionSet;
    }

    void OpenXRActionSet::suggestBindings(std::vector<XrActionSuggestedBinding>& xrSuggestedBindings, const SuggestedBindings& mwSuggestedBindings)
    {
        std::vector<XrActionSuggestedBinding> suggestedBindings;
        if (!mTrackerMap.empty())
        {
            suggestedBindings.emplace_back(XrActionSuggestedBinding{ *mTrackerMap[TrackedLimb::LEFT_HAND], getXrPath("/user/hand/left/input/aim/pose") });
            suggestedBindings.emplace_back(XrActionSuggestedBinding{ *mTrackerMap[TrackedLimb::RIGHT_HAND], getXrPath("/user/hand/right/input/aim/pose") });
        }
        if(!mHapticsMap.empty())
        {
            suggestedBindings.emplace_back(XrActionSuggestedBinding{ *mHapticsMap[TrackedLimb::LEFT_HAND], getXrPath("/user/hand/left/output/haptic") });
            suggestedBindings.emplace_back(XrActionSuggestedBinding{ *mHapticsMap[TrackedLimb::RIGHT_HAND], getXrPath("/user/hand/right/output/haptic") });
        };

        for (auto& mwSuggestedBinding : mwSuggestedBindings)
        {
            auto xrAction = mActionMap.find(mwSuggestedBinding.action);
            if (xrAction == mActionMap.end())
            {
                Log(Debug::Error) << "OpenXRActionSet: Unknown action " << mwSuggestedBinding.action;
                continue;
            }
            suggestedBindings.push_back({ *xrAction->second, getXrPath(mwSuggestedBinding.path) });
        }

        xrSuggestedBindings.insert(xrSuggestedBindings.end(), suggestedBindings.begin(), suggestedBindings.end());
    }

    XrSpace OpenXRActionSet::xrActionSpace(TrackedLimb limb)
    {
        return mTrackerMap[limb]->xrSpace();
    }


    std::unique_ptr<OpenXRAction>
        OpenXRActionSet::createXRAction(
            XrActionType actionType,
            const std::string& actionName,
            const std::string& localName)
    {
        std::vector<XrPath> subactionPaths;
        XrActionCreateInfo createInfo{ XR_TYPE_ACTION_CREATE_INFO };
        createInfo.actionType = actionType;
        strcpy_s(createInfo.actionName, actionName.c_str());
        strcpy_s(createInfo.localizedActionName, localName.c_str());

        XrAction action = XR_NULL_HANDLE;
        CHECK_XRCMD(xrCreateAction(mActionSet, &createInfo, &action));
        return std::unique_ptr<OpenXRAction>{new OpenXRAction{ action, actionType, actionName, localName }};
    }

    void
        OpenXRActionSet::updateControls()
    {
        auto* xr = Environment::get().getManager();
        if (!xr->impl().appShouldReadInput())
            return;

        const XrActiveActionSet activeActionSet{ mActionSet, XR_NULL_PATH };
        XrActionsSyncInfo syncInfo{ XR_TYPE_ACTIONS_SYNC_INFO };
        syncInfo.countActiveActionSets = 1;
        syncInfo.activeActionSets = &activeActionSet;
        CHECK_XRCMD(xrSyncActions(xr->impl().xrSession(), &syncInfo));

        mActionQueue.clear();
        for (auto& action : mActionMap)
            action.second->updateAndQueue(mActionQueue);
    }

    XrPath OpenXRActionSet::getXrPath(const std::string& path)
    {
        auto* xr = Environment::get().getManager();
        XrPath xrpath = 0;
        CHECK_XRCMD(xrStringToPath(xr->impl().xrInstance(), path.c_str(), &xrpath));
        return xrpath;
    }

    const Action* OpenXRActionSet::nextAction()
    {
        if (mActionQueue.empty())
            return nullptr;

        const auto* action = mActionQueue.front();
        mActionQueue.pop_front();
        return action;

    }

    Pose
        OpenXRActionSet::getLimbPose(
            int64_t time,
            TrackedLimb limb)
    {
        auto it = mTrackerMap.find(limb);
        if (it == mTrackerMap.end())
        {
            Log(Debug::Error) << "OpenXRActionSet: No such tracker: " << limb;
            return Pose{};
        }

        it->second->update(time);
        return it->second->value();
    }

    void OpenXRActionSet::applyHaptics(TrackedLimb limb, float intensity)
    {
        auto it = mHapticsMap.find(limb);
        if (it == mHapticsMap.end())
        {
            Log(Debug::Error) << "OpenXRActionSet: No such tracker: " << limb;
            return;
        }

        it->second->apply(intensity);
    }
}
