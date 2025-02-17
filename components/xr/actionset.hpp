#ifndef XR_ACTIONSET_HPP
#define XR_ACTIONSET_HPP

#include <openxr/openxr.h>

#include "action.hpp"
#include <components/vr/constants.hpp>
#include <components/vr/space.hpp>

#include <array>
#include <map>
#include <memory>
#include <vector>
#include <optional>

namespace XR
{

    /// \brief Suggest a binding by binding an action to a path on a given hand (left or right).
    struct SuggestedBinding
    {
        std::string path;
        std::string action;
    };

    using SuggestedBindings = std::vector<SuggestedBinding>;

    /// \brief Generates and manages an OpenXR ActionSet and associated actions.
    class ActionSet
    {
        ActionSet(const ActionSet&) = delete;
        void operator=(const ActionSet&) = delete;

    public:
        ActionSet(const std::string& actionSetName);
        ~ActionSet();

        //! Update all actions
        void update();

        //! Apply haptics of the given intensity to the given limb
        void applyHaptics(const std::string& id, float intensity);

        XrActionSet xrActionSet() { return mActionSet; }
        std::vector<XrActionSuggestedBinding> suggestBindings();

        std::shared_ptr<VR::Space> actionSpace(const std::string& id) const;

        void createBoolAction(const std::string& actionName, const std::string& localName, const std::string& id);
        void createAxisAction(const std::string& actionName, const std::string& localName, const std::string& id);
        void createFloatAction(const std::string& actionName, const std::string& localName, const std::string& id);
        void createPoseAction(const std::string& actionName, const std::string& localName, const std::string& id);
        void createHapticsAction(const std::string& actionName, const std::string& localName, const std::string& id);

        std::optional<InputAction::Value> getValue(const std::string& id) const;

        void suggestBinding(const std::string& id, const std::string& path);

        XrAction findXrAction(const std::string& id) const;
        std::shared_ptr<VR::Space> createActionSpace(
            const std::string& spaceId, const std::string& actionId, Stereo::Pose pose = {});

    protected:
        std::unique_ptr<XR::Action> createXRAction(XrActionType actionType, const std::string& actionName,
            const std::string& localName);
        XrPath getXrPath(const std::string& path) const;
        XrActionSet createActionSet(const std::string& name);

        XrActionSet mActionSet{ nullptr };
        std::string mLocalizedName{};
        std::string mInternalName{};

        std::map<std::string, BoolAction> mBoolActions;
        std::map<std::string, FloatAction> mFloatActions;
        std::map<std::string, AxisAction> mAxisActions;
        std::map<std::string, PoseAction> mPoseActions;
        std::map<std::string, HapticsAction> mHapticsActions;
        std::map<std::string, InputAction*> mAllInputActions;
        std::map<std::string, std::shared_ptr<VR::Space>> mActionSpaces;

        std::vector<std::pair<std::string, std::string>> mSuggestions;
    };
}

#endif
