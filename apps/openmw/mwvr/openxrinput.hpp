#ifndef OPENXR_INPUT_HPP
#define OPENXR_INPUT_HPP

#include "../mwinput/actions.hpp"

#include "vrinput.hpp"
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
    class OpenXRInput
    {
    public:
        using XrSuggestedBindings = std::vector<XrActionSuggestedBinding>;
        using XrProfileSuggestedBindings = std::map<std::string, XrSuggestedBindings>;

        static OpenXRInput& instance();

        //! Default constructor, creates two ActionSets: Gameplay and GUI
        OpenXRInput(const std::filesystem::path& xrControllerSuggestionsFile,
            const std::filesystem::path& defaultXrControllerSuggestionsFile);
        void createActionSets();
        void createLuaActions();
        void createPoseActions();

        //! Get the specified actionSet.
        XR::ActionSet& getActionSet(MWActionSet actionSet);

        //! Set bindings and attach actionSets to the session.
        void attachActionSets();

    protected:

        std::filesystem::path mXrControllerSuggestionsFile;
        std::filesystem::path mDefaultXrControllerSuggestionsFile;
        //std::shared_ptr<XR::AxisDeadzone> mDeadzone{ std::make_shared<XR::AxisDeadzone>() };
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
