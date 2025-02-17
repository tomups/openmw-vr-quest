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
    namespace
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
            t["transform"] = LuaUtil::asTransform(pose.pose.orientation);
            return t;
        }

        Stereo::Pose poseFromTable(sol::optional<sol::table> table)
        {
            Stereo::Pose pose = {};
            if (table)
            {
                pose.position = table->get_or("position", pose.position);
                sol::optional<LuaUtil::TransformQ> transform
                    = table->get<sol::optional<LuaUtil::TransformQ>>("rotation");
                if (transform)
                    pose.orientation = transform->mQ;
            }
            return pose;
        }

        MWVR::VRAnimation* getPlayerAnimation()
        {
            auto ptr = MWBase::Environment::get().getWorld()->getPlayerPtr();
            auto* anim = MWBase::Environment::get().getWorld()->getAnimation(ptr);
            if (!anim)
                throw std::runtime_error("Player does not have an animation yet");
            return static_cast<MWVR::VRAnimation*>(anim);
        }

        // sol's own get_or is buggy and gives compiler errors at e.g. returning a osg::Vec2
        // LuaUtil::Vec2 test = t.get_or("test", LuaUtil::Vec2())
        // fails to compile
        // so i wrote this wrapper to not go insane fetching osg::Vec2 from a sol::table
        template <typename T>
        T get_or(const sol::table& t, const std::string& key, T&& o)
        {
            sol::optional<T> opt = t.get<sol::optional<T>>(key);
            if (opt)
                return opt.value();
            return o;
        }
    }

    sol::table initVRPackage(const Context& context)
    {
        sol::state_view lua = context.sol();
        sol::table api(lua, sol::create);
        api["isVr"] = []() -> bool { return VR::getVR(); };

        if (!VR::getVR())
            return LuaUtil::makeReadOnly(api);

        MWVR::OpenXRInput& xrinput = MWVR::OpenXRInput::instance();

        api["CONTROLLER_PATHS"] = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromVector<std::string>(
            lua, { "/user/hand/left", "/user/hand/right", "/user/gamepad", "/user/treadmill" }));

        api["INTERACTION_VALUE_TYPES"] = LuaUtil::makeStrictReadOnly(
            LuaUtil::tableFromVector<std::string>(lua, { "BOOLEAN", "FLOAT", "AXIS", "POSE" }));

        api["EYES"] = LuaUtil::makeStrictReadOnly(
            LuaUtil::tableFromPairs<std::string, int>(lua, { { "LEFT_EYE", 1 }, { "RIGHT_EYE", 2 } }));

        api["REFERENCE_SPACES"]
            = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromPairs<std::string, int>(lua,
                {
                    { "VIEW", XR_REFERENCE_SPACE_TYPE_VIEW },
                    { "LOCAL", XR_REFERENCE_SPACE_TYPE_LOCAL },
                    { "STAGE", XR_REFERENCE_SPACE_TYPE_STAGE },
                }));

        api["isControllerActive"]
            = [](const std::string& path) -> bool { return VR::getControllerActive(VR::stringToXrPath(path)); };
        api["getInteractionProfileForController"] = [](const std::string& path) -> sol::optional<std::string> {
            auto p = VR::getControllerInteractionProfile(VR::stringToXrPath(path));
            if (!p)
                return sol::nullopt;
            return std::string(VR::xrPathToString(p));
        };
        api["getAvailableInteractions"]
            // This table never changes and is fairly big, so I cache it.
            = [interactions = getAvailableInteractions(context)](
                  std::string_view path) -> sol::table { return interactions; };
        api["getAvailableReferenceSpaces"]
            // This table never changes and is fairly big, so I cache it.
            = [references = getAvailableReferenceSpaces(context)](
                  std::string_view path) -> sol::table { return references; };

        api["createDerivedSpace"] = sol::overload(
            [&xrinput](const std::string& spaceId, VR::ReferenceSpace referenceSpaceType, const sol::table& pose) {
                xrinput.createReferenceSpace(spaceId, referenceSpaceType, poseFromTable(pose));
            },
            [&xrinput](const std::string& spaceId, const std::string& actionName, const sol::table& pose) {
                xrinput.createActionSpace(spaceId, actionName, poseFromTable(pose));
            });

        api["spaceExists"] = [&xrinput](const std::string& spaceId) -> bool { return !!xrinput.getSpace(spaceId); };

        // Cache the actionSet reference since it will never change and outlives lua.
        api["getActionValue"] = [&actionSet = MWVR::OpenXRInput::instance().getActionSet(MWVR::MWActionSet::Actions)](
                                    const std::string& path) { return actionSet.getValue(path); };
        api["locateSpace"] = [&xrinput, lua](const std::string& spaceId, sol::optional<std::string> ref) -> sol::object {
            auto space = xrinput.getSpace(spaceId);
            if (!space)
                return sol::nil;
            
            std::shared_ptr<VR::Space> reference
                = xrinput.getSpace(ref.value_or(MWVR::OpenXRInput::DefaultReferenceSpaceLocal));

            return tableFromPose(space->locate(reference), lua);
        };
        // All known /output/ path values are of type FLOAT.
        // If there is ever a reason to support more, we can expand this with overloads (or use sol::object).
        api["setOutputValue"] = [&actionSet = MWVR::OpenXRInput::instance().getActionSet(MWVR::MWActionSet::Haptics)](
                                    const std::string& path, float value) { actionSet.applyHaptics(path, value); };

        api["locateSpaceInWorld"] = [&xrinput, lua](const std::string& spaceId) -> sol::object {
            auto space = xrinput.getSpace(spaceId);
            if (!space)
                return sol::nil;

            return tableFromPose(space->locateInWorld(), lua);
        };

        api["attachBoneToSpace"] = 
            [&xrinput](const std::string& bone, const std::string& spaceId, sol::table options) {
            auto space = xrinput.getSpace(spaceId);
            auto anim = getPlayerAnimation();
            auto pose = poseFromTable(options.get<sol::optional<sol::table>>("pose"));
            anim->attachBoneToSpace(bone, space, pose);
            };

        api["_setGuiPose"] = [&guiManager = MWVR::VRGUIManager::instance()](const std::string& id,
                                 const sol::table& pose) {
                  guiManager.setLayerPose(id, poseFromTable(pose));
        };

        api["_setGuiLayer"]
            = [&guiManager = MWVR::VRGUIManager::instance()](std::string_view layer, const sol::table& options) {
                  MWVR::LayerConfig config;
                  config.priority = options.get_or("priority", 0);
                  // I frankly gave up trying to get sol::table to return an osg::Vec2. All i get are insane compiler errors from a simple `osg::Vec2 asdf = get_or(key, osg::Vec2())`
                  config.opacity = options.get_or("backgroundOpacity", 0);
                  config.center = get_or(options, "centerX", osg::Vec2());
                  config.extent = get_or(options, "extentX", osg::Vec2(1, 1));
                  config.spatialResolution = options.get_or("pixelsPerMeter", 1024);
                  config.pixelResolution.x() = options.get_or("pixelResolutionWidth", 1024);
                  config.pixelResolution.y() = options.get_or("pixelResolutionHeight", 1024);
                  config.myGUIViewSize = osg::Vec2(1, 1);
                  config.autoSize = options.get_or("autosize", true);
                  config.space = options.get_or("space", MWVR::VRGUIManager::DefaultUiSpace);
                  config.extraLayers = "popup";
                  config.intersectable = options.get_or("intersectable", true);
              };

        //api[]



        return LuaUtil::makeReadOnly(api);
    }
}
