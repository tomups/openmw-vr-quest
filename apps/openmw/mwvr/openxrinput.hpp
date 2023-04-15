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
        GUI = 0,
        Gameplay = 1,
        Tracking = 2,
        Haptics = 3,
    };

    /// \brief Generates and manages OpenXR Actions and ActionSets by generating openxr bindings from a list of
    /// SuggestedBindings structs.
    class OpenXRInput
    {
    public:
        using XrSuggestedBindings = std::vector<XrActionSuggestedBinding>;
        using XrProfileSuggestedBindings = std::map<std::string, XrSuggestedBindings>;

        //! Default constructor, creates two ActionSets: Gameplay and GUI
        OpenXRInput(const std::filesystem::path& xrControllerSuggestionsFile,
            const std::filesystem::path& defaultXrControllerSuggestionsFile);
        void createActionSets();
        void createGameplayActions();
        void createGUIActions();
        void createPoseActions();
        void createHapticActions();

        void readXrControllerSuggestions();

        //! Get the specified actionSet.
        XR::ActionSet& getActionSet(MWActionSet actionSet);

        //! Suggest bindings for the specific actionSet and profile pair. Call things after calling attachActionSets is
        //! an error.
        void suggestBindings(
            MWActionSet actionSet, std::string profile, const XR::SuggestedBindings& mwSuggestedBindings);

        //! Set bindings and attach actionSets to the session.
        void attachActionSets();

        void throwDocumentError(TiXmlElement* element, std::string error);
        std::string requireAttribute(TiXmlElement* element, std::string attribute);
        std::string optionalAttribute(TiXmlElement* element, std::string attribute);
        void readActions(TiXmlElement* element);
        void readInteractionProfile(TiXmlElement* element);
        void readInteractionProfileActionSet(TiXmlElement* element, MWActionSet actionSet, std::string profilePath);

        void setThumbstickDeadzone(float deadzoneRadius);

        int actionCodeFromName(const std::string& name) const;

    protected:
        void createMWAction(MWActionSet actionSet, XR::ControlType controlType, int openMWAction,
            const std::string& actionName, const std::string& localName, std::vector<VR::SubAction> subActions = {});

        std::filesystem::path mXrControllerSuggestionsFile;
        std::filesystem::path mDefaultXrControllerSuggestionsFile;
        std::shared_ptr<XR::AxisDeadzone> mDeadzone{ std::make_shared<XR::AxisDeadzone>() };
        std::map<std::string, std::string> mInteractionProfileLocalNames{};
        std::map<MWActionSet, XR::ActionSet> mActionSets{};
        std::map<XrPath, std::string> mInteractionProfileNames{};
        std::map<std::string, XrPath> mInteractionProfilePaths{};
        std::map<XrPath, XrPath> mActiveInteractionProfiles;
        std::set<std::string> mActionNamesChecklist;
        std::map<std::string, int> mActionNameToActionId;
        XrProfileSuggestedBindings mSuggestedBindings{};
        bool mAttached = false;
    };
}

#endif
