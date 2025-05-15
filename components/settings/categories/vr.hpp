#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_VR_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_VR_H

#include "components/settings/sanitizerimpl.hpp"
#include "components/settings/settingvalue.hpp"

#include <osg/Math>
#include <osg/Vec2f>
#include <osg/Vec3f>

#include <cstdint>
#include <string>
#include <string_view>

namespace Settings
{
    struct VRCategory : WithIndex
    {
        using WithIndex::WithIndex;

        SettingValue<float> mPlayerHeight { mIndex, "VR", "player height", makeMaxStrictSanitizerFloat(0.1) };
        SettingValue<bool> mMirrorTexture{ mIndex, "VR", "mirror texture" };
        SettingValue<std::string> mMirrorTextureEye{ mIndex, "VR", "mirror texture eye",
            makeEnumSanitizerString({ "left", "right", "both" }) };
        SettingValue<bool> mFlipMirrorTextureOrder{ mIndex, "VR", "flip mirror texture order" };
        SettingValue<std::string> mLeftEyeResolutionX{ mIndex, "VR", "left eye resolution x" };
        SettingValue<std::string> mLeftEyeResolutionY{ mIndex, "VR", "left eye resolution y" };
        SettingValue<std::string> mRightEyeResolutionX{ mIndex, "VR", "right eye resolution x" };
        SettingValue<std::string> mRightEyeResolutionY{ mIndex, "VR", "right eye resolution y" };
        SettingValue<float> mRealisticCombatMinimumSwingVelocity{ mIndex, "VR", "realistic combat minimum swing velocity",
            makeMaxStrictSanitizerFloat(0.1) };
        SettingValue<float> mRealisticCombatMaximumSwingVelocity{ mIndex, "VR", "realistic combat maximum swing velocity",
            makeMaxStrictSanitizerFloat(0.1) };
        SettingValue<bool> mHapticsEnabled{ mIndex, "VR", "haptics enabled" };
        SettingValue<bool> mHandDirectedMovement{ mIndex, "VR", "hand directed movement" };
        SettingValue<float> mHandsOffsetX{ mIndex, "VR", "hands offset x" };
        SettingValue<float> mHandsOffsetY{ mIndex, "VR", "hands offset y" };
        SettingValue<float> mHandsOffsetZ{ mIndex, "VR", "hands offset z" };
        SettingValue<bool> mLeftHandedMode{ mIndex, "VR", "left handed mode" };
        SettingValue<bool> mShow3DCrosshairs{ mIndex, "VR", "show 3D crosshairs" };
        SettingValue<bool> mUseXrLayerForHuds{ mIndex, "VR", "use xr layer for huds" };
    };
    struct VRDebugCategory : WithIndex
    {
        using WithIndex::WithIndex;

        SettingValue<bool> mLogAllOpenxrCalls{ mIndex, "VR Debug", "log all openxr calls" };
        SettingValue<bool> mContinueOnErrors{ mIndex, "VR Debug", "continue on errors" };
        SettingValue<bool> mDisableXR_KHR_opengl_enable{ mIndex, "VR Debug", "disable XR_KHR_opengl_enable" };
        SettingValue<bool> mDisableXR_KHR_D3D11_enable{ mIndex, "VR Debug", "disable XR_KHR_D3D11_enable" };
        SettingValue<bool> mDisableXR_KHR_composition_layer_depth{ mIndex, "VR Debug",
            "disable XR_KHR_composition_layer_depth" };
        SettingValue<bool> mDisableXR_EXT_debug_utils{ mIndex, "VR Debug", "disable XR_EXT_debug_utils" };
        SettingValue<bool> mDisableXR_EXT_hp_mixed_reality_controller{ mIndex, "VR Debug",
            "disable XR_EXT_hp_mixed_reality_controller" };
        SettingValue<bool> mDisableXR_HTC_vive_cosmos_controller_interaction{ mIndex, "VR Debug",
            "disable XR_HTC_vive_cosmos_controller_interaction" };
        SettingValue<bool> mDisableXR_HUAWEI_controller_interaction{ mIndex, "VR Debug",
            "disable XR_HUAWEI_controller_interaction" };
        SettingValue<bool> mDisableXR_MSFT_composition_layer_reprojection{ mIndex, "VR Debug",
            "disable XR_MSFT_composition_layer_reprojection" };
        SettingValue<bool> mDisableXR_FB_space_warp{ mIndex, "VR Debug", "disable XR_FB_space_warp" };
        SettingValue<bool> mXR_EXT_debug_utilsMessageLevelVerbose{ mIndex, "VR Debug",
            "XR_EXT_debug_utils message level verbose" };
        SettingValue<bool> mXR_EXT_debug_utilsMessageLevelInfo{ mIndex, "VR Debug", "XR_EXT_debug_utils message level info" };
        SettingValue<bool> mXR_EXT_debug_utilsMessageLevelWarning{ mIndex, "VR Debug",
            "XR_EXT_debug_utils message level warning" };
        SettingValue<bool> mXR_EXT_debug_utilsMessageLevelError{ mIndex, "VR Debug",
            "XR_EXT_debug_utils message level error" };
        SettingValue<bool> mXR_EXT_debug_utilsMessageTypeGeneral{ mIndex, "VR Debug",
            "XR_EXT_debug_utils message type general" };
        SettingValue<bool> mXR_EXT_debug_utilsMessageTypeValidation{ mIndex, "VR Debug",
            "XR_EXT_debug_utils message type validation" };
        SettingValue<bool> mXR_EXT_debug_utilsMessageTypePerformance{ mIndex, "VR Debug",
            "XR_EXT_debug_utils message type performance" };
        SettingValue<bool> mXR_EXT_debug_utilsMessageTypeConformance{ mIndex, "VR Debug",
            "XR_EXT_debug_utils message type conformance" };
        SettingValue<float> mCharacterBaseHeight{ mIndex, "VR Debug", "character base height",
            makeMaxStrictSanitizerFloat(0.1) };
        SettingValue<bool> mSubmitStencilFormats{ mIndex, "VR Debug", "submit stencil formats" };
        SettingValue<bool> mDisableTextureFormat{ mIndex, "VR Debug", "disable texture format" };
        SettingValue<bool> mSkywindBlasterWorkaround{ mIndex, "VR Debug", "skywind blaster workaround" };
    };
}

#endif
