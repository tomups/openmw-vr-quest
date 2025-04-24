#include "vrbindings.hpp"
#include "../mwbase/world.hpp"
#include "../mwvr/vrgui.hpp"
#include "../mwvr/openxrinput.hpp"
#include "../mwvr/vrinputmanager.hpp"
#include <components/lua/utilpackage.hpp>
#include <components/vr/vr.hpp>
#include <components/vr/trackingmanager.hpp>
#include <components/xr/extensions.hpp>
#include <components/xr/session.hpp>
#include <components/xr/interactionprofiles.hpp>

#include <vector>

namespace MWLua
{
    namespace
    {
        // sol's own get_or seems to be buggy and gives compiler errors at e.g. returning a osg::Vec2
        // LuaUtil::Vec2 test = t.get_or("test", LuaUtil::Vec2())
        // fails to compile
        // so i wrote this wrapper to not go insane fetching osg::Vec2 values from a sol::table
        template <typename T>
        T get_or(const sol::table& t, const std::string& key, T&& o)
        {
            sol::optional<T> opt = t.get<sol::optional<T>>(key);
            if (opt)
                return opt.value();
            return o;
        }

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
        sol::table getAvailableReferenceSpaces(const Context& context)
        {
            sol::state_view lua = context.sol();
            sol::table referenceTable(lua, sol::create);
            for (auto reference : VR::Session::instance().getSupportedReferenceSpaceTypes())
                referenceTable[referenceTable.size() + 1] = reference;

            return referenceTable;
        }

        sol::table tableFromPose(const VR::TrackingPose& pose, sol::state_view lua)
        {
            if (!pose.status)
                return sol::nil;

            sol::table t(lua, sol::create);
            t["position"] = pose.pose.position.asMWUnits();
            t["orientation"] = LuaUtil::asTransform(pose.pose.orientation);
            return t;
        }

        Stereo::Pose poseFromTable(sol::optional<sol::table> table)
        {
            Stereo::Pose pose = {};
            if (table)
            {
                pose.position = Stereo::Position::fromMWUnits(get_or<osg::Vec3f>(*table, "position", osg::Vec3f()));
                if (auto transform = table->get<sol::optional<LuaUtil::TransformQ>>("orientation"))
                    pose.orientation = transform->mQ;
                else if (auto transform = table->get<sol::optional<LuaUtil::TransformM>>("orientation"))
                    pose.orientation = transform->mM.getRotate();
            }
            return pose;
        }

        MWVR::LayerConfig layerConfigFromTable(const sol::table& options)
        {
            MWVR::LayerConfig layerConfig;
            layerConfig.opacity = options.get_or("backgroundOpacity", 0);
            layerConfig.center = get_or(options, "center", osg::Vec2());
            layerConfig.extent = get_or(options, "extent", osg::Vec2(1, 1));
            layerConfig.spatialResolution = options.get_or("pixelsPerMeter", 1024);
            auto rttResolution = get_or(options, "rttResolution", osg::Vec2(1024, 1024));
            layerConfig.pixelResolution.x() = static_cast<int>(rttResolution.x());
            layerConfig.pixelResolution.y() = static_cast<int>(rttResolution.y());
            layerConfig.myGUIViewSize = osg::Vec2(1, 1);
            layerConfig.autoSize = options.get_or("autosize", true);
            layerConfig.space = options.get_or<std::string>("space", "");
            layerConfig.intersectable = options.get_or("intersectable", false);
            return layerConfig;
        }
    }

    sol::table initVRPackage(const Context& context)
    {
        sol::state_view lua = context.sol();
        sol::table api(lua, sol::create);
        api["isVr"] = []() -> bool { return VR::getVR(); };

        api["CONTROLLER_PATHS"] = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromVector<std::string>(
            lua, { "/user/hand/left", "/user/hand/right", "/user/gamepad", "/user/treadmill" }));

        api["INTERACTION_VALUE_TYPES"] = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromPairs<std::string, std::string>(
            lua, { { "Boolean", "BOOLEAN" }, { "Float", "FLOAT" }, { "Axis", "AXIS" }, { "Pose", "POSE" } }));

        api["EYES"] = LuaUtil::makeStrictReadOnly(
            LuaUtil::tableFromPairs<std::string, int>(lua, { { "LEFT_EYE", 1 }, { "RIGHT_EYE", 2 } }));

        api["REFERENCE_SPACES"]
            = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromPairs<std::string, VR::ReferenceSpace>(lua,
                {
                    { "View", VR::ReferenceSpace::View },
                    { "Local", VR::ReferenceSpace::Local },
                    { "Stage", VR::ReferenceSpace::Stage },
                }));

        if (!VR::getVR())
            return LuaUtil::makeReadOnly(api);

        MWVR::OpenXRInput& xrinput = MWVR::OpenXRInput::instance();
        api["availableInteractions"] = getAvailableInteractions(context);
        api["availableReferenceSpaces"] = getAvailableReferenceSpaces(context);

        api["isControllerActive"]
            = [](const std::string& path) -> bool { return VR::getControllerActive(VR::stringToXrPath(path)); };
        api["getInteractionProfileOfController"] = [](const std::string& path) -> sol::optional<std::string> {
            auto p = VR::getControllerInteractionProfile(VR::stringToXrPath(path));
            if (!p)
                return sol::nullopt;
            return std::string(VR::xrPathToString(p));
        };

        api["createDerivedSpace"] = sol::overload(
            [&xrinput](const std::string& spaceId, VR::ReferenceSpace referenceSpaceType, const sol::table& pose) {
                xrinput.createDerivedSpace(spaceId, referenceSpaceType, poseFromTable(pose));
            },
            [&xrinput](const std::string& spaceId, const std::string& actionName, const sol::table& pose) {
                xrinput.createDerivedSpace(spaceId, actionName, poseFromTable(pose));
            });

        api["spaceExists"] = [&xrinput](const std::string& spaceId) -> bool { return !!xrinput.getSpace(spaceId); };

        // Cache the actionSet reference since it will never change and outlives lua.
        api["getActionValue"] = [&actionSet = MWVR::OpenXRInput::instance().getActionSet(MWVR::MWActionSet::Actions)](
                                    const std::string& path) { return actionSet.getValue(path); };
        api["locateSpace"] = [&xrinput, lua](const std::string& spaceId, sol::optional<std::string> ref) -> sol::object {
            if (!VR::getLocatingSpacesAllowed())
                throw std::logic_error("locateSpace() is only allowed during onVRFrame()");
            auto space = xrinput.getSpace(spaceId);
            if (!space)
                return sol::nil;
            
            std::shared_ptr<VR::Space> reference
                = xrinput.getSpace(ref.value_or(MWVR::OpenXRInput::DefaultReferenceSpaceLocal));

            return tableFromPose(space->locate(*reference), lua);
        };
        // All known /output/ path values are of type FLOAT.
        // If there is ever a reason to support more, we can expand this with overloads (or use sol::object).
        api["setOutputValue"] = [&actionSet = MWVR::OpenXRInput::instance().getActionSet(MWVR::MWActionSet::Haptics)](
                                    const std::string& path, float value) { actionSet.applyHaptics(path, value); };

        api["locateSpaceInWorld"] = [&xrinput, lua](const std::string& spaceId) -> sol::object {
            if (!VR::getLocatingSpacesAllowed())
                throw std::logic_error("locateSpace() is only allowed during onVRFrame()");
            auto space = xrinput.getSpace(spaceId);
            if (!space)
                return sol::nil;

            return tableFromPose(space->locateInWorld(), lua);
        };

        api["_setGuiPose"] = [&guiManager = MWVR::VRGUIManager::instance()](const std::string& id,
                                 const sol::table& pose) { guiManager.setLayerPose(id, poseFromTable(pose)); };

        api["_setModeConfig"] = [&guiManager = MWVR::VRGUIManager::instance()](
                                   const std::string& mode, const sol::table& options) {
            guiManager.setModeConfig(mode, layerConfigFromTable(options));
        };

        api["_setModePose"] = [&guiManager = MWVR::VRGUIManager::instance()](
                                   const std::string& mode, const sol::table& pose, sol::optional<std::string> window) {
            guiManager.setModePose(mode, poseFromTable(pose), window.value_or(""));
        };

        api["_setLayerConfig"]
            = [&guiManager = MWVR::VRGUIManager::instance()](const std::string& layer, const sol::table& options) {
                  guiManager.setLayerConfig(layer, layerConfigFromTable(options));
              };

        api["_setLayerPose"]
            = [&guiManager = MWVR::VRGUIManager::instance()](const std::string& layer, const sol::table& pose) {
                  guiManager.setLayerPose(layer, poseFromTable(pose));
              };

        api["_setPointerLeft"] = [&inputManager = MWVR::VRInputManager::instance()](
                                     bool enabled) { inputManager.setPointerLeft(enabled); };

        api["_setPointerRight"] = [&inputManager = MWVR::VRInputManager::instance()](
                                      bool enabled) { inputManager.setPointerRight(enabled); };

        api["_pointerActivate"]
            = [&inputManager = MWVR::VRInputManager::instance()](
                  bool injectMouseClickIfApplicable) { inputManager.pointerActivate(injectMouseClickIfApplicable); };

        api["_recenter"] = []() { VR::recenter(); };

        //api[]



        return LuaUtil::makeReadOnly(api);
    }
}
