#include "openxrinput.hpp"

#include <openxr/openxr.h>

#include <components/misc/strings/algorithm.hpp>
#include <components/vr/trackingmanager.hpp>
#include <components/xr/debug.hpp>
#include <components/xr/instance.hpp>
#include <components/xr/session.hpp>
#include <components/xr/tracking.hpp>
#include <components/xr/interactionprofiles.hpp>

#include <iostream>
#include <sstream>
#include <set>

#include <extern/oics/tinyxml.h>


namespace
{
    struct Interaction
    {
        enum ValueType
        {
            BOOLEAN,
            FLOAT,
            AXIS,
            POSE
        };

        ValueType valueType;
        std::string path;
        std::optional<std::string> requiredExtension = std::nullopt;
    };
    struct Controller
    {
        std::string path;
        std::optional<std::string> requiredExtension = std::nullopt;
    };
    using Interactions = std::vector<Interaction>;
    using Controllers = std::map<std::string, Interactions>;
    using InteractionProfiles = std::vector<std::pair<std::vector<Controller>, Controllers>>;
    constexpr const char* LEFT_HAND = "/user/hand/left";
    constexpr const char* RIGHT_HAND = "/user/hand/right";
    constexpr const char* BOTH_HANDS = "/user/hand/*";
    constexpr const char* GAMEPAD = "/user/gamepad";

    static Interaction::ValueType ParseValueType(const char* typeStr)
    {
        if (!typeStr)
            return Interaction::BOOLEAN; // default/fallback

        if (std::strcmp(typeStr, "BOOLEAN") == 0)
            return Interaction::BOOLEAN;
        if (std::strcmp(typeStr, "FLOAT") == 0)
            return Interaction::FLOAT;
        if (std::strcmp(typeStr, "AXIS") == 0)
            return Interaction::AXIS;
        if (std::strcmp(typeStr, "POSE") == 0)
            return Interaction::POSE;

        // Unknown or missing -> fallback
        return Interaction::BOOLEAN;
    }

    static const char* ValueTypeToString(Interaction::ValueType valueType)
    {
        switch (valueType)
        {
            case Interaction::BOOLEAN:
                return "BOOLEAN";
            case Interaction::FLOAT:
                return "FLOAT";
            case Interaction::AXIS:
                return "AXIS";
            case Interaction::POSE:
                return "POSE";
            default:
                return "UNKNOWN";
        }
    }

    InteractionProfiles interactionProfiles;

}

namespace MWVR
{
    static OpenXRInput* sXrInput;

    OpenXRInput& OpenXRInput::instance()
    {
        assert(sXrInput);
        return *sXrInput;
    }
    OpenXRInput::OpenXRInput(const std::filesystem::path& xrControllerSuggestionsFile,
        const std::filesystem::path& defaultXrControllerSuggestionsFile)
        : mXrControllerSuggestionsFile(xrControllerSuggestionsFile)
        , mDefaultXrControllerSuggestionsFile(defaultXrControllerSuggestionsFile)
    {
        assert(!sXrInput);
        sXrInput = this;
        createActionSets();
        createLuaActions();
        createPoseActions();
        attachActionSets();
    }

    void OpenXRInput::createActionSets()
    {
        mActionSets.emplace(
            std::piecewise_construct, std::forward_as_tuple(MWActionSet::Actions), std::forward_as_tuple("Lua"));
        mActionSets.emplace(std::piecewise_construct, std::forward_as_tuple(MWActionSet::Tracking),
            std::forward_as_tuple("Tracking"));
    }

    static std::string pathToValidOpenXRActionName(std::string_view name)
    {
        std::string s = std::string(name);
        std::replace(s.begin(), s.end(), '/', '_');
        return s;
    }

    void OpenXRInput::createLuaActions() 
    {
        // Use the list of known interactionProfiles to populate a list of actions
        auto interactionProfiles = XR::getAllKnownInteractionProfiles();
        std::set<std::string> axisActions;
        std::set<std::string> boolActions;
        std::set<std::string> floatActions;
        std::set<std::string> poseActions;
        std::set<std::string> hapticsActions;

        for (auto& ip : interactionProfiles)
        {
            for (auto& ctrl : ip.second)
            {
                for (auto& interaction : ctrl.second)
                {
                    auto path = ctrl.first + interaction.path;
                    switch (interaction.valueType)
                    {
                        case XR::Interaction::ValueType::AXIS:
                            axisActions.insert(path);
                            break;
                        case XR::Interaction::ValueType::BOOLEAN:
                            boolActions.insert(path);
                            break;
                        case XR::Interaction::ValueType::FLOAT:
                            if (path.find("/output/") != std::string::npos)
                                hapticsActions.insert(path);
                            else
                                floatActions.insert(path);
                            break;
                        case XR::Interaction::ValueType::POSE:
                            poseActions.insert(path);
                            break;
                    }
                }
            }
        }

        auto& luaActionsSet = getActionSet(MWActionSet::Actions);
        auto& poseActionsSet = getActionSet(MWActionSet::Pose);
        auto& hapticsActionsSet = getActionSet(MWActionSet::Haptics);

        // For lua actions, the name of the action *is* the intended path path
        for (auto& path : axisActions)
        {
            luaActionsSet.createAxisAction(pathToValidOpenXRActionName(path), path, path);
            luaActionsSet.suggestBinding(path, path);
        }
        for (auto& path : boolActions)
        {
            luaActionsSet.createBoolAction(pathToValidOpenXRActionName(path), path, path);
            luaActionsSet.suggestBinding(path, path);
        }
        for (auto& path : floatActions)
        {
            luaActionsSet.createFloatAction(pathToValidOpenXRActionName(path), path, path);
            luaActionsSet.suggestBinding(path, path);
        }
        for (auto& path : poseActions)
        {
            poseActionsSet.createPoseAction(pathToValidOpenXRActionName(path), path, path);
            poseActionsSet.suggestBinding(path, path);
        }
        for (auto& path : hapticsActions)
        {
            hapticsActionsSet.createHapticsAction(pathToValidOpenXRActionName(path), path, path);
            hapticsActionsSet.suggestBinding(path, path);
        }
    }

    void OpenXRInput::createPoseActions()
    {
        auto& poseActionsSet = getActionSet(MWActionSet::Pose);
        poseActionsSet.createPoseAction("hand_pose_left", "Hand Pose Left", "hand_pose_left");
        poseActionsSet.createPoseAction("hand_pose_right", "Hand Pose Right", "hand_pose_right");
        poseActionsSet.suggestBinding("hand_pose_left", "/user/hand/left/input/aim/pose");
        poseActionsSet.suggestBinding("hand_pose_right", "/user/hand/right/input/aim/pose");
    }

    XR::ActionSet& OpenXRInput::getActionSet(MWActionSet actionSet)
    {
        auto it = mActionSets.find(actionSet);
        if (it == mActionSets.end())
            throw std::logic_error("No such action set");
        return it->second;
    }

    void OpenXRInput::attachActionSets()
    {
        auto& all = XR::getAllKnownInteractionProfiles();
        for (auto& profile : all)
        {
            if (profile.first.requiredExtension && !XR::Extensions::instance().extensionEnabled(profile.first.requiredExtension.value()))
                continue;

            std::set<XrPath> availablePaths;
            for (auto& ctrl : profile.second)
            {
                for (auto& interaction : ctrl.second)
                {
                    XrPath interactionPath = XR_NULL_PATH;
                    CHECK_XRCMD(xrStringToPath(
                        XR::Instance::instance().xrInstance(), (ctrl.first + interaction.path).c_str(), &interactionPath));
                    availablePaths.insert(interactionPath);
                }
            }

            std::vector<XrActionSuggestedBinding> suggestions;
            for (auto& actionSet : mActionSets)
                for (auto& suggestion : actionSet.second.suggestBindings())
                    if (availablePaths.contains(suggestion.binding))
                        suggestions.push_back(suggestion);

            XrPath profilePath = 0;
            CHECK_XRCMD(xrStringToPath(XR::Instance::instance().xrInstance(), profile.first.path.c_str(), &profilePath));
            XrInteractionProfileSuggestedBinding xrProfileSuggestedBindings{};
            xrProfileSuggestedBindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
            xrProfileSuggestedBindings.interactionProfile = profilePath;
            xrProfileSuggestedBindings.suggestedBindings = suggestions.data();
            xrProfileSuggestedBindings.countSuggestedBindings = (uint32_t)suggestions.size();
            CHECK_XRCMD(xrSuggestInteractionProfileBindings(
                XR::Instance::instance().xrInstance(), &xrProfileSuggestedBindings));
            mInteractionProfileNames[profilePath] = profile.first.path;
            mInteractionProfilePaths[profile.first.path] = profilePath;
        }

        // OpenXR requires that xrAttachSessionActionSets be called at most once per session.
        // So collect all action sets
        std::vector<XrActionSet> actionSets;
        for (auto& actionSet : mActionSets)
            actionSets.push_back(actionSet.second.xrActionSet());

        // Attach
        XrSessionActionSetsAttachInfo attachInfo{};
        attachInfo.type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO;
        attachInfo.countActionSets = actionSets.size();
        attachInfo.actionSets = actionSets.data();
        CHECK_XRCMD(xrAttachSessionActionSets(XR::Session::instance().xrSession(), &attachInfo));
    }
}
