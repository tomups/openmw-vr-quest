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
        createReferenceSpaces();
    }

    void OpenXRInput::createActionSets()
    {
        mActionSets.emplace(
            std::piecewise_construct, std::forward_as_tuple(MWActionSet::Actions), std::forward_as_tuple("Lua"));
        mActionSets.emplace(
            std::piecewise_construct, std::forward_as_tuple(MWActionSet::Pose), std::forward_as_tuple("Poses"));
        mActionSets.emplace(
            std::piecewise_construct, std::forward_as_tuple(MWActionSet::Haptics), std::forward_as_tuple("Haptics"));
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
            createActionSpace(path, path);
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

    void OpenXRInput::createReferenceSpaces()
    {
        auto supportedTypes = VR::Session::instance().getSupportedReferenceSpaceTypes();
        createReferenceSpace(DefaultReferenceSpaceLocal, VR::ReferenceSpace::Local);
        createReferenceSpace(DefaultReferenceSpaceView, VR::ReferenceSpace::View);
        // OpenXR Requires that local and view must always be supported, but Stage is optional.
        if (std::find(supportedTypes.begin(), supportedTypes.end(), VR::ReferenceSpace::Stage) != supportedTypes.end())
            createReferenceSpace(DefaultReferenceSpaceStage, VR::ReferenceSpace::Stage);
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

    void OpenXRInput::createDerivedSpace(const std::string& id, const std::string& referenceId, Stereo::Pose pose)
    {
        if (!mSpaces.contains(referenceId))
            throw std::runtime_error("No such space " + referenceId);
        if (mSpaces.contains(id))
        {
            auto space = dynamic_cast<VR::DerivedSpace*>(mSpaces[id].get());
            if (!space)
                throw std::runtime_error("Space " + id + " already exists and is immutable");
            space->setPose(pose);
            space->setReference(mSpaces[referenceId]);
            return;
        }
        mSpaces[id] = std::make_shared<VR::DerivedSpace>(mSpaces.at(referenceId), pose);
    }

    void OpenXRInput::createDerivedSpace(const std::string& id, VR::ReferenceSpace ref, Stereo::Pose pose) 
    {
        if (mSpaces.contains(id))
            throw std::runtime_error("Space " + id + " already exists");
        mSpaces[id] = std::make_shared<VR::DerivedSpace>(VR::Session::instance().getReferenceSpace(ref), pose);
    }

    void OpenXRInput::createActionSpace(const std::string& id, const std::string& actionName) 
    {
        if (mSpaces.contains(id))
            throw std::runtime_error("Space " + id + " already exists");
        mSpaces[id] = mActionSets.at(MWActionSet::Pose).createActionSpace(id, actionName);
    }

    void OpenXRInput::createReferenceSpace(const std::string& id, VR::ReferenceSpace ref) 
    {
        if (mSpaces.contains(id))
            throw std::runtime_error("Space " + id + " already exists");
        mSpaces[id] = VR::Session::instance().getReferenceSpace(ref);
    }

    std::shared_ptr<VR::Space> OpenXRInput::getSpace(const std::string& id) const
    {
        auto it = mSpaces.find(id);
        if (it == mSpaces.end())
            return nullptr;
        return it->second;
    }
    void OpenXRInput::onFrameUpdate(VR::Frame&) 
    {
        mActionSets.at(MWActionSet::Pose).update();
    }
}
