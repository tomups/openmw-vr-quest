#ifndef XR_ACTIONSET_HPP
#define XR_ACTIONSET_HPP

#include <openxr/openxr.h>

#include "action.hpp"
#include <components/vr/constants.hpp>

#include <array>
#include <map>
#include <memory>
#include <vector>

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
        ActionSet(const std::string& actionSetName, std::shared_ptr<AxisDeadzone> deadzone);
        ~ActionSet();

        //! Update all controls and queue any actions
        void updateControls(bool shouldQueueActions);

        //! Get next action from queue (repeat until null is returned)
        const InputAction* nextAction();

        //! Apply haptics of the given intensity to the given limb
        void applyHaptics(VR::SubAction side, float intensity);

        XrActionSet xrActionSet() { return mActionSet; }
        void suggestBindings(
            std::vector<XrActionSuggestedBinding>& xrSuggestedBindings, const SuggestedBindings& mwSuggestedBindings);

        XrSpace xrActionSpace(VR::SubAction side);

        void createMWAction(ControlType controlType, int openMWAction, const std::string& actionName,
            const std::string& localName, std::vector<VR::SubAction> subActions = {});
        void createPoseAction(
            const std::string& actionName, const std::string& localName, std::vector<VR::SubAction> subActions = {});
        void createHapticsAction(
            const std::string& actionName, const std::string& localName, std::vector<VR::SubAction> subActions = {});

    protected:
        template <typename A>
        void createMWAction(int openMWAction, const std::string& actionName, const std::string& localName,
            std::vector<VR::SubAction> subActions = {});
        std::shared_ptr<XR::Action> createXRAction(XrActionType actionType, const std::string& actionName,
            const std::string& localName, std::vector<VR::SubAction> subActions);
        XrPath getXrPath(const std::string& path) const;
        XrActionSet createActionSet(const std::string& name);

        XrActionSet mActionSet{ nullptr };
        std::string mLocalizedName{};
        std::string mInternalName{};
        std::map<std::string, std::shared_ptr<Action>> mActionMap;
        std::map<std::pair<int, VR::SubAction>, std::unique_ptr<InputAction>> mInputActionMap;
        std::map<VR::SubAction, std::unique_ptr<PoseAction>> mTrackerMap;
        std::map<VR::SubAction, std::unique_ptr<HapticsAction>> mHapticsMap;
        std::deque<const InputAction*> mActionQueue{};
        std::shared_ptr<AxisDeadzone> mDeadzone;
    };
}

#endif
