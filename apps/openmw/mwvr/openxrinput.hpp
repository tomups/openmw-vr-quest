#ifndef OPENXR_INPUT_HPP
#define OPENXR_INPUT_HPP

#include "../mwinput/actions.hpp"

#include "vrinput.hpp"
#include <components/vr/session.hpp>
#include <components/xr/action.hpp>
#include <components/xr/actionset.hpp>

#include <array>
#include <map>
#include <set>
#include <vector>
#include <filesystem>

class TiXmlElement;

namespace MWVR
{
    /// Extension of MWInput's set of actions.
    enum VrActions
    {
        A_VrFirst = MWInput::A_Last + 1,
        A_VrMetaMenu,
        A_ActivateTouch,
        A_HapticsLeft,
        A_HapticsRight,
        A_MenuUpDown,
        A_MenuLeftRight,
        A_MenuSelect,
        A_MenuBack,
        A_Recenter,
        A_RadialMenu,
        A_MovementStick,
        A_UtilityStick,
        A_ToggleSneak,
        A_VrLast
    };

    /// \brief Enumeration of action sets
    enum class MWActionSet
    {
        Actions,
        Pose,
        Haptics,
    };

    /// \brief Generates and manages OpenXR Actions and ActionSets by generating openxr bindings from a list of
    /// SuggestedBindings structs.
    class OpenXRInput : VR::Session::Listener
    {
    public:

        static constexpr const char* DefaultReferenceSpaceLocal = "/default/reference/local";
        static constexpr const char* DefaultReferenceSpaceView = "/default/reference/view";
        static constexpr const char* DefaultReferenceSpaceStage = "/default/reference/stage";
        static constexpr const char* LeftHandAim = "/user/hand/left/input/aim/pose";
        static constexpr const char* LeftHandGrip = "/user/hand/left/input/grip/pose";
        static constexpr const char* RightHandAim = "/user/hand/right/input/aim/pose";
        static constexpr const char* RightHandGrip = "/user/hand/right/input/grip/pose";

        using XrSuggestedBindings = std::vector<XrActionSuggestedBinding>;
        using XrProfileSuggestedBindings = std::map<std::string, XrSuggestedBindings>;

        static OpenXRInput& instance();

        //! Default constructor, creates two ActionSets: Gameplay and GUI
        OpenXRInput();
        void createActionSets();
        void createLuaActions();
        void createPoseActions();
        void createReferenceSpaces();

        //! Get the specified actionSet.
        XR::ActionSet& getActionSet(MWActionSet actionSet);

        //! Set bindings and attach actionSets to the session.
        void attachActionSets();

        void createDerivedSpace(const std::string& id, const std::string& referenceId, Stereo::Pose pose = {});
        void createDerivedSpace(const std::string& id, VR::ReferenceSpace ref, Stereo::Pose pose = {});
        void createActionSpace(const std::string& id, const std::string& actionName);
        void createReferenceSpace(const std::string& id, VR::ReferenceSpace ref);

        std::shared_ptr<VR::Space> getSpace(const std::string& id) const;

    protected:
        void onFrameUpdate(VR::Frame&) override;

        std::map<std::string, std::shared_ptr<VR::Space>> mSpaces{};
        std::map<std::string, std::string> mInteractionProfileLocalNames{};
        std::map<MWActionSet, XR::ActionSet> mActionSets{};
        std::map<XrPath, std::string> mInteractionProfileNames{};
        std::map<std::string, XrPath> mInteractionProfilePaths{};
        std::map<XrPath, XrPath> mActiveInteractionProfiles;
        std::set<std::string> mActionNamesChecklist;
        bool mAttached = false;
    };
}

#endif
