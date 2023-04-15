#include "openxrinput.hpp"

#include <openxr/openxr.h>

#include <components/misc/strings/algorithm.hpp>
#include <components/vr/trackingmanager.hpp>
#include <components/xr/debug.hpp>
#include <components/xr/instance.hpp>
#include <components/xr/session.hpp>
#include <components/xr/tracking.hpp>

#include <iostream>
#include <sstream>

#include <extern/oics/tinyxml.h>

namespace MWVR
{

    OpenXRInput::OpenXRInput(const std::filesystem::path& xrControllerSuggestionsFile,
        const std::filesystem::path& defaultXrControllerSuggestionsFile)
        : mXrControllerSuggestionsFile(xrControllerSuggestionsFile)
        , mDefaultXrControllerSuggestionsFile(defaultXrControllerSuggestionsFile)
    {
        createActionSets();
        createGameplayActions();
        createGUIActions();
        createPoseActions();
        createHapticActions();
        readXrControllerSuggestions();
        attachActionSets();
    }

    void OpenXRInput::createActionSets()
    {
        mActionSets.emplace(std::piecewise_construct, std::forward_as_tuple(MWActionSet::Gameplay),
            std::forward_as_tuple("Gameplay", mDeadzone));
        mActionSets.emplace(
            std::piecewise_construct, std::forward_as_tuple(MWActionSet::GUI), std::forward_as_tuple("GUI", mDeadzone));
        mActionSets.emplace(std::piecewise_construct, std::forward_as_tuple(MWActionSet::Tracking),
            std::forward_as_tuple("Tracking", mDeadzone));
        mActionSets.emplace(std::piecewise_construct, std::forward_as_tuple(MWActionSet::Haptics),
            std::forward_as_tuple("Haptics", mDeadzone));
    }

    void OpenXRInput::createGameplayActions()
    {
        createMWAction(MWActionSet::Gameplay, XR::ControlType::Press, MWInput::A_GameMenu, "game_menu", "Game Menu");
        createMWAction(MWActionSet::Gameplay, XR::ControlType::Press, A_VrMetaMenu, "meta_menu", "Meta Menu");
        createMWAction(
            MWActionSet::Gameplay, XR::ControlType::LongPress, A_Recenter, "reposition_menu", "Reposition Menu");
        createMWAction(MWActionSet::Gameplay, XR::ControlType::Press, MWInput::A_Inventory, "inventory", "Inventory");
        createMWAction(MWActionSet::Gameplay, XR::ControlType::Hold, MWInput::A_Use, "use", "Use");
        createMWAction(MWActionSet::Gameplay, XR::ControlType::Hold, MWInput::A_Jump, "jump", "Jump");
        createMWAction(MWActionSet::Gameplay, XR::ControlType::Press, MWInput::A_ToggleWeapon, "weapon", "Weapon");
        createMWAction(MWActionSet::Gameplay, XR::ControlType::Press, MWInput::A_ToggleSpell, "spell", "Spell");
        createMWAction(MWActionSet::Gameplay, XR::ControlType::Press, MWInput::A_CycleSpellLeft, "cycle_spell_left",
            "Cycle Spell Left");
        createMWAction(MWActionSet::Gameplay, XR::ControlType::Press, MWInput::A_CycleSpellRight, "cycle_spell_right",
            "Cycle Spell Right");
        createMWAction(MWActionSet::Gameplay, XR::ControlType::Press, MWInput::A_CycleWeaponLeft, "cycle_weapon_left",
            "Cycle Weapon Left");
        createMWAction(MWActionSet::Gameplay, XR::ControlType::Press, MWInput::A_CycleWeaponRight, "cycle_weapon_right",
            "Cycle Weapon Right");
        createMWAction(MWActionSet::Gameplay, XR::ControlType::Hold, MWInput::A_Sneak, "sneak", "Sneak");
        createMWAction(
            MWActionSet::Gameplay, XR::ControlType::Press, MWInput::A_QuickKeysMenu, "quick_menu", "Quick Menu");
        createMWAction(
            MWActionSet::Gameplay, XR::ControlType::Press, MWInput::A_Journal, "journal_book", "Journal Book");
        createMWAction(MWActionSet::Gameplay, XR::ControlType::Press, MWInput::A_QuickSave, "quick_save", "Quick Save");
        createMWAction(MWActionSet::Gameplay, XR::ControlType::Press, MWInput::A_Rest, "rest", "Rest");
        createMWAction(MWActionSet::Gameplay, XR::ControlType::Axis1D, A_ActivateTouch, "activate_touched",
            "Activate Touch", { VR::SubAction::HandLeft, VR::SubAction::HandRight });
        createMWAction(MWActionSet::Gameplay, XR::ControlType::Press, MWInput::A_AlwaysRun, "always_run", "Always Run");
        createMWAction(MWActionSet::Gameplay, XR::ControlType::Press, MWInput::A_AutoMove, "auto_move", "Auto Move");
        createMWAction(MWActionSet::Gameplay, XR::ControlType::Press, MWInput::A_ToggleHUD, "toggle_hud", "Toggle HUD");
        createMWAction(MWActionSet::Gameplay, XR::ControlType::Press, MWInput::A_ToggleDebug, "toggle_debug",
            "Toggle the debug hud");
        createMWAction(MWActionSet::Gameplay, XR::ControlType::Press, MWInput::A_ToggleThumbstickAutoRun,
            "toggle_thumbstick_auto_run", "Toggle Thumbstick Auto Run");
        createMWAction(
            MWActionSet::Gameplay, XR::ControlType::Press, A_ToggleSneak, "toggle_sneak", "Toggle Sneak");
        createMWAction(MWActionSet::Gameplay, XR::ControlType::LongPress, A_RadialMenu, "radial_menu", "Radial Menu");
        createMWAction(
            MWActionSet::Gameplay, XR::ControlType::Axis2D, A_MovementStick, "movement_stick", "Movement Stick");
        createMWAction(
            MWActionSet::Gameplay, XR::ControlType::Axis2D, A_UtilityStick, "utility_stick", "Utility Stick");
    }

    void OpenXRInput::createGUIActions()
    {
        createMWAction(MWActionSet::GUI, XR::ControlType::Press, MWInput::A_GameMenu, "game_menu", "Game Menu");
        createMWAction(MWActionSet::GUI, XR::ControlType::LongPress, A_Recenter, "reposition_menu", "Reposition Menu");
        createMWAction(MWActionSet::GUI, XR::ControlType::Axis1D, A_MenuUpDown, "menu_up_down", "Menu Up Down");
        createMWAction(
            MWActionSet::GUI, XR::ControlType::Axis1D, A_MenuLeftRight, "menu_left_right", "Menu Left Right");
        createMWAction(MWActionSet::GUI, XR::ControlType::Press, A_MenuSelect, "menu_select", "Menu Select");
        createMWAction(MWActionSet::GUI, XR::ControlType::Press, A_MenuBack, "menu_back", "Menu Back");
        createMWAction(MWActionSet::GUI, XR::ControlType::Hold, MWInput::A_Use, "use", "Use");
    }

    void OpenXRInput::createPoseActions()
    {
        getActionSet(MWActionSet::Tracking)
            .createPoseAction("hand_pose", "Hand Pose", { VR::SubAction::HandLeft, VR::SubAction::HandRight });

        auto stageUserHandLeftPath = VR::stringToVRPath("/stage/user/hand/left/input/aim/pose");
        auto stageUserHandRightPath = VR::stringToVRPath("/stage/user/hand/right/input/aim/pose");
        auto worldUserHandLeftPath = VR::stringToVRPath("/world/user/hand/left/input/aim/pose");
        auto worldUserHandRightPath = VR::stringToVRPath("/world/user/hand/right/input/aim/pose");

        XR::Session::instance().tracker().setTrackingActionSet(&getActionSet(MWActionSet::Tracking));

        XR::Session::instance().tracker().addTrackingSpace(
            stageUserHandLeftPath, getActionSet(MWActionSet::Tracking).xrActionSpace(VR::SubAction::HandLeft));
        XR::Session::instance().tracker().addTrackingSpace(
            stageUserHandRightPath, getActionSet(MWActionSet::Tracking).xrActionSpace(VR::SubAction::HandRight));
        XR::Session::instance().stageToWorldBinding().bindPaths(worldUserHandLeftPath, stageUserHandLeftPath);
        XR::Session::instance().stageToWorldBinding().bindPaths(worldUserHandRightPath, stageUserHandRightPath);
    }

    void OpenXRInput::createHapticActions()
    {
        getActionSet(MWActionSet::Haptics)
            .createHapticsAction("hand_haptics", "Hand Haptics", { VR::SubAction::HandLeft, VR::SubAction::HandRight });
    }

    void OpenXRInput::readXrControllerSuggestions()
    {
        if (mXrControllerSuggestionsFile.empty())
            throw std::runtime_error("No interaction profiles available (xrcontrollersuggestions.xml not found)");

        Log(Debug::Verbose) << "Reading Input Profile Path suggestions from " << mXrControllerSuggestionsFile;

        TiXmlDocument* xmlDoc = nullptr;
        TiXmlElement* xmlRoot = nullptr;
        xmlDoc = new TiXmlDocument(mXrControllerSuggestionsFile.string().c_str());
        xmlDoc->LoadFile();

        if (xmlDoc->Error())
        {
            std::ostringstream message;
            message << "TinyXml reported an error reading \"" + mXrControllerSuggestionsFile.string() + "\". Row "
                    << (int)xmlDoc->ErrorRow() << ", Col " << (int)xmlDoc->ErrorCol() << ": " << xmlDoc->ErrorDesc();
            Log(Debug::Error) << message.str();
            throw std::runtime_error(message.str());

            delete xmlDoc;
            return;
        }

        xmlRoot = xmlDoc->RootElement();
        if (std::string(xmlRoot->Value()) != "Root")
        {
            Log(Debug::Verbose) << "Error: Invalid xr controllers file. Missing <Root> element.";
            throw std::runtime_error("Error: Invalid xr controllers file. Missing <Root> element.");
            delete xmlDoc;
            return;
        }

        TiXmlElement* actions = xmlRoot->FirstChildElement("Actions");
        if (!actions)
        {
            Log(Debug::Error) << "Error: Invalid xr controllers file. Missing <Actions> element.";
            throw std::runtime_error("Error: Invalid xr controllers file. Missing <Actions> element.");
        }
        readActions(actions);

        TiXmlElement* profile = xmlRoot->FirstChildElement("Profile");
        while (profile)
        {
            readInteractionProfile(profile);
            profile = profile->NextSiblingElement("Profile");
        }
    }

    XR::ActionSet& OpenXRInput::getActionSet(MWActionSet actionSet)
    {
        auto it = mActionSets.find(actionSet);
        if (it == mActionSets.end())
            throw std::logic_error("No such action set");
        return it->second;
    }

    void OpenXRInput::suggestBindings(
        MWActionSet actionSet, std::string profilePath, const XR::SuggestedBindings& mwSuggestedBindings)
    {
        getActionSet(actionSet).suggestBindings(mSuggestedBindings[profilePath], mwSuggestedBindings);
    }

    void OpenXRInput::attachActionSets()
    {
        // Suggest bindings before attaching
        for (auto& profile : mSuggestedBindings)
        {
            XrPath profilePath = 0;
            CHECK_XRCMD(xrStringToPath(XR::Instance::instance().xrInstance(), profile.first.c_str(), &profilePath));
            XrInteractionProfileSuggestedBinding xrProfileSuggestedBindings{};
            xrProfileSuggestedBindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
            xrProfileSuggestedBindings.interactionProfile = profilePath;
            xrProfileSuggestedBindings.suggestedBindings = profile.second.data();
            xrProfileSuggestedBindings.countSuggestedBindings = (uint32_t)profile.second.size();
            CHECK_XRCMD(xrSuggestInteractionProfileBindings(
                XR::Instance::instance().xrInstance(), &xrProfileSuggestedBindings));
            mInteractionProfileNames[profilePath] = profile.first;
            mInteractionProfilePaths[profile.first] = profilePath;
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

    void OpenXRInput::throwDocumentError(TiXmlElement* element, std::string error)
    {
        std::stringstream ss;
        ss << mXrControllerSuggestionsFile << "." << element->Row() << "." << element->Value();
        ss << ": " << error;
        throw std::runtime_error(ss.str());
    }

    std::string OpenXRInput::requireAttribute(TiXmlElement* element, std::string attribute)
    {
        const char* value = element->Attribute(attribute.c_str());
        if (!value)
            throwDocumentError(element, std::string() + "Missing attribute '" + attribute + "'");
        return value;
    }

    std::string OpenXRInput::optionalAttribute(TiXmlElement* element, std::string attribute)
    {
        const char* value = element->Attribute(attribute.c_str());
        if (!value)
            return "";
        return value;
    }

    void OpenXRInput::readActions(TiXmlElement* element)
    {
        TiXmlElement* action = element->FirstChildElement("Action");
        while (action)
        {
            std::string name = requireAttribute(action, "Name");
            mActionNamesChecklist.erase(name);
            action = action->NextSiblingElement("Action");
        }

        if (!mActionNamesChecklist.empty())
        {
            std::vector<std::string> missingActions(mActionNamesChecklist.begin(), mActionNamesChecklist.end());
            std::stringstream ss;
            ss << "Your controller bindings file, " << mXrControllerSuggestionsFile
               << ", is stale. It is missing <Action> entries for the actions: ";
            for (unsigned int i = 0; i < missingActions.size(); i++)
            {
                Log(Debug::Error) << "Xr controller suggestions file does not contain an entry for action "
                                  << missingActions[i];
                if (i > 0)
                    ss << ", ";
                ss << missingActions[i];
            }

            ss << ".";

            if (mXrControllerSuggestionsFile == mDefaultXrControllerSuggestionsFile)
            {
                ss << " You should NOT be editing any files located in your openmw folder. Please reinstall OpenMW-VR "
                      "and read the instructions at the top of xrcontrollersuggestions.xml carefully.";
            }
            else
            {
                ss << " Restore default bindings by deleting or moving the file, and then create new bindings if you "
                      "so wish based on a fresh copy of "
                   << mDefaultXrControllerSuggestionsFile;
            }

            throw std::runtime_error(ss.str());
        }
    }

    void OpenXRInput::readInteractionProfile(TiXmlElement* element)
    {
        std::string interactionProfilePath = requireAttribute(element, "Path");
        mInteractionProfileLocalNames[interactionProfilePath] = requireAttribute(element, "LocalName");

        Log(Debug::Verbose) << "Configuring interaction profile '" << interactionProfilePath << "' ("
                            << mInteractionProfileLocalNames[interactionProfilePath] << ")";

        // Check extension if present
        TiXmlElement* extensionElement = element->FirstChildElement("Extension");
        if (extensionElement)
        {
            std::string extension = requireAttribute(extensionElement, "Name");
            if (!XR::Extensions::instance().extensionEnabled(extension))
            {
                Log(Debug::Verbose) << "  Required extension '" << extension
                                    << "' not supported. Skipping interaction profile.";
                return;
            }
        }

        TiXmlElement* actionSetGameplay = nullptr;
        TiXmlElement* actionSetGUI = nullptr;
        TiXmlElement* child = element->FirstChildElement("ActionSet");
        while (child)
        {
            std::string name = requireAttribute(child, "Name");
            if (name == "Gameplay")
                actionSetGameplay = child;
            else if (name == "GUI")
                actionSetGUI = child;

            child = child->NextSiblingElement("ActionSet");
        }

        if (!actionSetGameplay)
            throwDocumentError(element, "Gameplay action set missing");
        if (!actionSetGUI)
            throwDocumentError(element, "GUI action set missing");

        readInteractionProfileActionSet(actionSetGameplay, MWActionSet::Gameplay, interactionProfilePath);
        readInteractionProfileActionSet(actionSetGUI, MWActionSet::GUI, interactionProfilePath);
        suggestBindings(MWActionSet::Tracking, interactionProfilePath, {});
        suggestBindings(MWActionSet::Haptics, interactionProfilePath, {});
    }

    void OpenXRInput::readInteractionProfileActionSet(
        TiXmlElement* element, MWActionSet actionSet, std::string interactionProfilePath)
    {
        XR::SuggestedBindings suggestedBindings;

        auto runtimeName = XR::Instance::instance().getRuntimeName();

        for (TiXmlElement* child = element->FirstChildElement("Binding"); child != nullptr;
             child = child->NextSiblingElement("Binding"))
        {
            std::string action = requireAttribute(child, "ActionName");
            std::string path = requireAttribute(child, "Path");
            std::string ifRuntime = optionalAttribute(child, "IfRuntime");
            std::string ifNotRuntime = optionalAttribute(child, "IfNotRuntime");

            if (!ifRuntime.empty() && Misc::StringUtils::ciFind(runtimeName, ifRuntime) == std::string::npos)
                continue;
            if (!ifNotRuntime.empty() && Misc::StringUtils::ciFind(runtimeName, ifNotRuntime) != std::string::npos)
                continue;

            suggestedBindings.push_back(XR::SuggestedBinding{ path, action });

            Log(Debug::Debug) << "  " << action << ": " << path;
        }

        suggestBindings(actionSet, interactionProfilePath, suggestedBindings);
    }

    void OpenXRInput::setThumbstickDeadzone(float deadzoneRadius)
    {
        mDeadzone->setDeadzoneRadius(deadzoneRadius);
    }

    int OpenXRInput::actionCodeFromName(const std::string& name) const
    {
        auto it = mActionNameToActionId.find(name);
        if (it != mActionNameToActionId.end())
            return it->second;
        return -1;
    }

    void OpenXRInput::createMWAction(MWActionSet actionSet, XR::ControlType controlType, int openMWAction,
        const std::string& actionName, const std::string& localName, std::vector<VR::SubAction> subActions)
    {
        mActionNameToActionId[actionName] = openMWAction;
        getActionSet(actionSet).createMWAction(controlType, openMWAction, actionName, localName, subActions);
        mActionNamesChecklist.insert(actionName);
    }
}
