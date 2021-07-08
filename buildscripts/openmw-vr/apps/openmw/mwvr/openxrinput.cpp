#include "openxrinput.hpp"

#include "vrenvironment.hpp"
#include "openxrmanager.hpp"
#include "openxrmanagerimpl.hpp"
#include "openxraction.hpp"

#include <openxr/openxr.h>

#include <components/misc/stringops.hpp>

#include <iostream>

namespace MWVR
{

    OpenXRInput::OpenXRInput(std::shared_ptr<AxisAction::Deadzone> deadzone)
    {
        mActionSets.emplace(ActionSet::Gameplay, OpenXRActionSet("Gameplay", deadzone));
        mActionSets.emplace(ActionSet::GUI, OpenXRActionSet("GUI", deadzone));
        mActionSets.emplace(ActionSet::Tracking, OpenXRActionSet("Tracking", deadzone));
        mActionSets.emplace(ActionSet::Haptics, OpenXRActionSet("Haptics", deadzone));


        /*
            // Applicable actions not (yet) included
            A_QuickKey1,
            A_QuickKey2,
            A_QuickKey3,
            A_QuickKey4,
            A_QuickKey5,
            A_QuickKey6,
            A_QuickKey7,
            A_QuickKey8,
            A_QuickKey9,
            A_QuickKey10,
            A_QuickKeysMenu,
            A_QuickLoad,
            A_CycleSpellLeft,
            A_CycleSpellRight,
            A_CycleWeaponLeft,
            A_CycleWeaponRight,
            A_Screenshot, // Generate a VR screenshot?
            A_Console,    // Currently awkward due to a lack of virtual keyboard, but should be included when that's in place
        */

        getActionSet(ActionSet::Gameplay).createMWAction(VrControlType::Press, MWInput::A_GameMenu, "game_menu", "Game Menu");
        getActionSet(ActionSet::Gameplay).createMWAction(VrControlType::Press, A_VrMetaMenu, "meta_menu", "Meta Menu");
        getActionSet(ActionSet::Gameplay).createMWAction(VrControlType::LongPress, A_Recenter, "reposition_menu", "Reposition Menu");
        getActionSet(ActionSet::Gameplay).createMWAction(VrControlType::Press, MWInput::A_Inventory, "inventory", "Inventory");
        getActionSet(ActionSet::Gameplay).createMWAction(VrControlType::Press, MWInput::A_Activate, "activate", "Activate");
        getActionSet(ActionSet::Gameplay).createMWAction(VrControlType::Hold, MWInput::A_Use, "use", "Use");
        getActionSet(ActionSet::Gameplay).createMWAction(VrControlType::Hold, MWInput::A_Jump, "jump", "Jump");
        getActionSet(ActionSet::Gameplay).createMWAction(VrControlType::Press, MWInput::A_ToggleWeapon, "weapon", "Weapon");
        getActionSet(ActionSet::Gameplay).createMWAction(VrControlType::Press, MWInput::A_ToggleSpell, "spell", "Spell");
        getActionSet(ActionSet::Gameplay).createMWAction(VrControlType::Press, MWInput::A_CycleSpellLeft, "cycle_spell_left", "Cycle Spell Left");
        getActionSet(ActionSet::Gameplay).createMWAction(VrControlType::Press, MWInput::A_CycleSpellRight, "cycle_spell_right", "Cycle Spell Right");
        getActionSet(ActionSet::Gameplay).createMWAction(VrControlType::Press, MWInput::A_CycleWeaponLeft, "cycle_weapon_left", "Cycle Weapon Left");
        getActionSet(ActionSet::Gameplay).createMWAction(VrControlType::Press, MWInput::A_CycleWeaponRight, "cycle_weapon_right", "Cycle Weapon Right");
        getActionSet(ActionSet::Gameplay).createMWAction(VrControlType::Hold, MWInput::A_Sneak, "sneak", "Sneak");
        getActionSet(ActionSet::Gameplay).createMWAction(VrControlType::Press, MWInput::A_QuickKeysMenu, "quick_menu", "Quick Menu");
        getActionSet(ActionSet::Gameplay).createMWAction(VrControlType::Axis, MWInput::A_LookLeftRight, "look_left_right", "Look Left Right");
        getActionSet(ActionSet::Gameplay).createMWAction(VrControlType::Axis, MWInput::A_MoveForwardBackward, "move_forward_backward", "Move Forward Backward");
        getActionSet(ActionSet::Gameplay).createMWAction(VrControlType::Axis, MWInput::A_MoveLeftRight, "move_left_right", "Move Left Right");
        getActionSet(ActionSet::Gameplay).createMWAction(VrControlType::Press, MWInput::A_Journal, "journal_book", "Journal Book");
        getActionSet(ActionSet::Gameplay).createMWAction(VrControlType::Press, MWInput::A_QuickSave, "quick_save", "Quick Save");
        getActionSet(ActionSet::Gameplay).createMWAction(VrControlType::Press, MWInput::A_Rest, "rest", "Rest");
        getActionSet(ActionSet::Gameplay).createMWAction(VrControlType::Axis, A_ActivateTouch, "activate_touched", "Activate Touch");
        getActionSet(ActionSet::Gameplay).createMWAction(VrControlType::Press, MWInput::A_AlwaysRun, "always_run", "Always Run");
        getActionSet(ActionSet::Gameplay).createMWAction(VrControlType::Press, MWInput::A_AutoMove, "auto_move", "Auto Move");
        getActionSet(ActionSet::Gameplay).createMWAction(VrControlType::Press, MWInput::A_ToggleHUD, "toggle_hud", "Toggle HUD");
        getActionSet(ActionSet::Gameplay).createMWAction(VrControlType::Press, MWInput::A_ToggleDebug, "toggle_debug", "Toggle the debug hud");

        getActionSet(ActionSet::GUI).createMWAction(VrControlType::Press, MWInput::A_GameMenu, "game_menu", "Game Menu");
        getActionSet(ActionSet::GUI).createMWAction(VrControlType::LongPress, A_Recenter, "reposition_menu", "Reposition Menu");
        getActionSet(ActionSet::GUI).createMWAction(VrControlType::Axis, A_MenuUpDown, "menu_up_down", "Menu Up Down");
        getActionSet(ActionSet::GUI).createMWAction(VrControlType::Axis, A_MenuLeftRight, "menu_left_right", "Menu Left Right");
        getActionSet(ActionSet::GUI).createMWAction(VrControlType::Press, A_MenuSelect, "menu_select", "Menu Select");
        getActionSet(ActionSet::GUI).createMWAction(VrControlType::Press, A_MenuBack, "menu_back", "Menu Back");
        getActionSet(ActionSet::GUI).createMWAction(VrControlType::Hold, MWInput::A_Use, "use", "Use");

        getActionSet(ActionSet::Tracking).createPoseAction(TrackedLimb::LEFT_HAND, "left_hand_pose", "Left Hand Pose");
        getActionSet(ActionSet::Tracking).createPoseAction(TrackedLimb::RIGHT_HAND, "right_hand_pose", "Right Hand Pose");

        getActionSet(ActionSet::Haptics).createHapticsAction(TrackedLimb::RIGHT_HAND, "right_hand_haptics", "Right Hand Haptics");
        getActionSet(ActionSet::Haptics).createHapticsAction(TrackedLimb::LEFT_HAND, "left_hand_haptics", "Left Hand Haptics");


        auto* xr = Environment::get().getManager();
        auto* trackingManager = Environment::get().getTrackingManager();

        auto leftHandPath = trackingManager->stringToVRPath("/user/hand/left/input/aim/pose");
        auto rightHandPath = trackingManager->stringToVRPath("/user/hand/right/input/aim/pose");

        xr->impl().tracker().addTrackingSpace(leftHandPath, getActionSet(ActionSet::Tracking).xrActionSpace(TrackedLimb::LEFT_HAND));
        xr->impl().tracker().addTrackingSpace(rightHandPath, getActionSet(ActionSet::Tracking).xrActionSpace(TrackedLimb::RIGHT_HAND));
    };

    OpenXRActionSet& OpenXRInput::getActionSet(ActionSet actionSet)
    {
        auto it = mActionSets.find(actionSet);
        if (it == mActionSets.end())
            throw std::logic_error("No such action set");
        return it->second;
    }

    void OpenXRInput::suggestBindings(ActionSet actionSet, std::string profilePath, const SuggestedBindings& mwSuggestedBindings)
    {
        getActionSet(actionSet).suggestBindings(mSuggestedBindings[profilePath], mwSuggestedBindings);
    }

    void OpenXRInput::attachActionSets()
    {
        auto* xr = Environment::get().getManager();

        // Suggest bindings before attaching
        for (auto& profile : mSuggestedBindings)
        {
            XrPath profilePath = 0;
            CHECK_XRCMD(
                xrStringToPath(xr->impl().xrInstance(), profile.first.c_str(), &profilePath));
            XrInteractionProfileSuggestedBinding xrProfileSuggestedBindings{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
            xrProfileSuggestedBindings.interactionProfile = profilePath;
            xrProfileSuggestedBindings.suggestedBindings = profile.second.data();
            xrProfileSuggestedBindings.countSuggestedBindings = (uint32_t)profile.second.size();
            CHECK_XRCMD(xrSuggestInteractionProfileBindings(xr->impl().xrInstance(), &xrProfileSuggestedBindings));
            mInteractionProfileNames[profilePath] = profile.first;
            mInteractionProfilePaths[profile.first] = profilePath;
        }

        // OpenXR requires that xrAttachSessionActionSets be called at most once per session.
        // So collect all action sets
        std::vector<XrActionSet> actionSets;
        for (auto& actionSet : mActionSets)
            actionSets.push_back(actionSet.second.xrActionSet());

        // Attach
        XrSessionActionSetsAttachInfo attachInfo{ XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO };
        attachInfo.countActionSets = actionSets.size();
        attachInfo.actionSets = actionSets.data();
        CHECK_XRCMD(xrAttachSessionActionSets(xr->impl().xrSession(), &attachInfo));
    }

    void OpenXRInput::notifyInteractionProfileChanged()
    {
        auto xr = MWVR::Environment::get().getManager();
        xr->impl().xrSession();

        // Unfortunately, openxr does not tell us WHICH profile has changed.
        std::array<std::string, 5> topLevelUserPaths =
        {
            "/user/hand/left",
            "/user/hand/right",
            "/user/head",
            "/user/gamepad",
            "/user/treadmill"
        };

        for (auto& userPath : topLevelUserPaths)
        {
            auto pathIt = mInteractionProfilePaths.find(userPath);
            if (pathIt == mInteractionProfilePaths.end())
            {
                XrPath xrUserPath = XR_NULL_PATH;
                CHECK_XRCMD(
                    xrStringToPath(xr->impl().xrInstance(), userPath.c_str(), &xrUserPath));
                mInteractionProfilePaths[userPath] = xrUserPath;
                pathIt = mInteractionProfilePaths.find(userPath);
            }

            XrInteractionProfileState interactionProfileState{
                XR_TYPE_INTERACTION_PROFILE_STATE
            };

            xrGetCurrentInteractionProfile(xr->impl().xrSession(), pathIt->second, &interactionProfileState);
            if (interactionProfileState.interactionProfile)
            {
                auto activeProfileIt = mActiveInteractionProfiles.find(pathIt->second);
                if (activeProfileIt == mActiveInteractionProfiles.end() || interactionProfileState.interactionProfile != activeProfileIt->second)
                {
                    auto activeProfileNameIt = mInteractionProfileNames.find(interactionProfileState.interactionProfile);
                    Log(Debug::Verbose) << userPath << ": Interaction profile changed to '" << activeProfileNameIt->second << "'";
                    mActiveInteractionProfiles[pathIt->second] = interactionProfileState.interactionProfile;
                }
            }
        }
    }
}
