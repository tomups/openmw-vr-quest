#include "vrbindings.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwvr/vrgui.hpp"
#include "../mwvr/openxrinput.hpp"
#include "../mwvr/vranimation.hpp"
#include "../mwvr/vrutil.hpp"
#include <components/lua/utilpackage.hpp>
#include <components/vr/vr.hpp>
#include <components/vr/trackingmanager.hpp>
#include <components/xr/extensions.hpp>
#include <components/xr/session.hpp>
#include <components/xr/interactionprofiles.hpp>

#include <vector>

namespace MWLua
{

    sol::table getAvailableInteractions(const Context& context)
    {
        sol::state_view lua = context.sol();
        // Gets the complete list of available interactions for the given controller
        auto& all = XR::getAllKnownInteractionProfiles();

        sol::table profileTable(lua, sol::create);
        for (const auto& interactionProfile : all)
        {
            if (interactionProfile.first.requiredExtension.has_value()
                && !XR::Extensions::instance().extensionEnabled(interactionProfile.first.requiredExtension.value()))
                continue;

            sol::table controllerTable(lua, sol::create);
            auto& controllers = interactionProfile.second;
            for (auto& controller : controllers)
            {
                sol::table interactionsMap(lua, sol::create);
                for (auto& interaction : controller.second)
                {
                    if (interaction.requiredExtension.has_value()
                        && !XR::Extensions::instance().extensionEnabled(interaction.requiredExtension.value()))
                        continue;
                    interactionsMap[interaction.path] = XR::ValueTypeToString(interaction.valueType);
                }
                controllerTable[controller.first] = interactionsMap;
            }

            profileTable[interactionProfile.first.path] = controllerTable;
        }

        return LuaUtil::makeReadOnly(profileTable);
    }

    sol::table tableFromPose(const VR::TrackingPose& pose, sol::state_view lua)
    {
        if (!pose.status)
            return sol::nil;

        sol::table t(lua, sol::create);
        t["position"] = pose.pose.position.asMWUnits();
        t["transform"] = LuaUtil::asTransform(pose.pose.orientation);
    }

    sol::table initVRPackage(const Context& context)
    {
        sol::state_view lua = context.sol();
        sol::table api(lua, sol::create);

        api["CONTROLLER_PATHS"] = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromVector<std::string>(
            lua, { "/user/hand/left", "/user/hand/right", "/user/gamepad", "/user/treadmill" }));

        api["INTERACTION_VALUE_TYPES"] = LuaUtil::makeStrictReadOnly(
            LuaUtil::tableFromVector<std::string>(lua, { "BOOLEAN", "FLOAT", "AXIS", "POSE" }));

        api["EYES"] = LuaUtil::makeStrictReadOnly(
            LuaUtil::tableFromPairs<std::string, int>(lua, { { "LEFT_EYE", 1 }, { "RIGHT_EYE", 2 } }));

        api["REFERENCE_SPACES"]
            = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromPairs<std::string, XrReferenceSpaceType>(lua,
                {
                    { "VIEW", XR_REFERENCE_SPACE_TYPE_VIEW },
                    { "LOCAL", XR_REFERENCE_SPACE_TYPE_LOCAL },
                    { "STAGE", XR_REFERENCE_SPACE_TYPE_STAGE },
                }));

        api["isVr"] = []() -> bool { return VR::getVR(); };
        api["isControllerActive"]
            = [](std::string_view path) -> bool { return VR::getControllerActive(VR::stringToVRPath(path)); };
        api["getInteractionProfileForController"] = [](std::string_view path) -> sol::optional<std::string> {
            auto p = VR::getControllerInteractionProfile(VR::stringToVRPath(path));
            if (!p)
                return sol::nullopt;
            return std::string(VR::VRPathToString(p));
        };
        api["getAvailableInteractions"]
            // This table never changes and is fairly big, so I cache it.
            = [interactions = getAvailableInteractions(context)](
                  std::string_view path) -> sol::table { return interactions; };

        // Cache the actionSet reference since it will never change and outlives lua.
        api["getActionValue"] = [&actionSet = MWVR::OpenXRInput::instance().getActionSet(MWVR::MWActionSet::Actions)](
                                    const std::string& path) { return actionSet.getValue(path); };
        api["locatePose"] = sol::overload(
            [&session = XR::Session::instance(),
                &actionSet = MWVR::OpenXRInput::instance().getActionSet(MWVR::MWActionSet::Pose),
                lua](const std::string& path, XrReferenceSpaceType reference) -> sol::object {
                auto referenceSpace = session.getReferenceSpace(reference);
                if (!referenceSpace)
                    throw std::runtime_error("Invalid reference space: " + std::to_string(reference));
                auto space = actionSet.xrActionSpace(path);
                if (!space)
                    throw std::runtime_error("Invalid action space: " + path);
                return tableFromPose(session.locateSpace(space, referenceSpace), lua);
            },
            [&session = XR::Session::instance(),
                &actionSet = MWVR::OpenXRInput::instance().getActionSet(MWVR::MWActionSet::Pose),
                lua](const std::string& path, const std::string& referencePath) {
                auto referenceSpace = actionSet.xrActionSpace(referencePath);
                if (!referenceSpace)
                    throw std::runtime_error("Invalid reference action space: " + referencePath);
                auto space = actionSet.xrActionSpace(path);
                if (!space)
                    throw std::runtime_error("Invalid action space: " + path);
                return tableFromPose(session.locateSpace(space, referenceSpace), lua);
            });
        api["locateReference"] = 
            [&session = XR::Session::instance(),
                &actionSet = MWVR::OpenXRInput::instance().getActionSet(MWVR::MWActionSet::Pose),
                  lua](XrReferenceSpaceType toLocate, XrReferenceSpaceType relativeTo) -> sol::object {
            auto spaceToLocate = session.getReferenceSpace(toLocate);
            if (!toLocate)
                throw std::runtime_error("Invalid reference space: " + std::to_string(toLocate));
            auto spaceRelativeTo = session.getReferenceSpace(relativeTo);
            if (!relativeTo)
                throw std::runtime_error("Invalid reference space: " + std::to_string(relativeTo));

                return tableFromPose(session.locateSpace(spaceToLocate, spaceRelativeTo), lua);
            };

        // All known /output/ path values are of type FLOAT.
        // If there is ever a reason to support more, we can expand this with overloads (or use sol::object).
        api["setOutputValue"] = [&actionSet = MWVR::OpenXRInput::instance().getActionSet(MWVR::MWActionSet::Haptics)](
                                    const std::string& path, float value) { actionSet.applyHaptics(path, value); };

        api["_setGuiPose"] = [&guiManager = MWVR::VRGUIManager::instance()](const std::string& id,
                                 const osg::Vec3f& position, const LuaUtil::TransformQ& transform) {
            guiManager.setLayerPose(id, { Stereo::Position::fromMWUnits(position), transform.mQ });
        };

        api["_setGuiSpace"] = sol::overload([&guiManager = MWVR::VRGUIManager::instance()](const std::string& id, XrReferenceSpaceType space) {
            guiManager.setLayerRef(id, XR::Session::instance().getReferenceSpace(space));
            },
            [&guiManager = MWVR::VRGUIManager::instance(),
                &actionSet = MWVR::OpenXRInput::instance().getActionSet(MWVR::MWActionSet::Pose)](
                const std::string& id, const std::string& name) {
                guiManager.setLayerRef(id, actionSet.xrActionSpace(name));
            }
            );

        //api[]



        return LuaUtil::makeReadOnly(api);
    }
}
