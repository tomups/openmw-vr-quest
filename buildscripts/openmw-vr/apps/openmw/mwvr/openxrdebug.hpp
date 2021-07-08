#ifndef OPENXR_DEBUG_HPP
#define OPENXR_DEBUG_HPP

#include <openxr/openxr.h>

#include <string>

namespace MWVR
{
    namespace VrDebug
    {
        //! Translates an OpenXR object to the associated XrObjectType enum value
        template<typename T> XrObjectType getObjectType(T t);

        //! Associates a name with an OpenXR symbol if XR_EXT_debug_utils is enabled
        template<typename T> void setName(T t, const std::string& name);

        //! Associates a name with an OpenXR symbol if XR_EXT_debug_utils is enabled
        void setName(uint64_t handle, XrObjectType type, const std::string& name);
    }
}

template<typename T> inline void MWVR::VrDebug::setName(T t, const std::string& name)
{
    setName(reinterpret_cast<uint64_t>(t), getObjectType(t), name);
}

template<> inline XrObjectType MWVR::VrDebug::getObjectType<XrInstance>(XrInstance)
{
    return XR_OBJECT_TYPE_INSTANCE;
}

template<> inline XrObjectType MWVR::VrDebug::getObjectType<XrSession>(XrSession)
{
    return XR_OBJECT_TYPE_SESSION;
}

template<> inline XrObjectType MWVR::VrDebug::getObjectType<XrSpace>(XrSpace)
{
    return XR_OBJECT_TYPE_SPACE;
}

template<> inline XrObjectType MWVR::VrDebug::getObjectType<XrActionSet>(XrActionSet)
{
    return XR_OBJECT_TYPE_ACTION_SET;
}

template<> inline XrObjectType MWVR::VrDebug::getObjectType<XrAction>(XrAction)
{
    return XR_OBJECT_TYPE_ACTION;
}

template<> inline XrObjectType MWVR::VrDebug::getObjectType<XrDebugUtilsMessengerEXT>(XrDebugUtilsMessengerEXT)
{
    return XR_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT;
}

template<> inline XrObjectType MWVR::VrDebug::getObjectType<XrSpatialAnchorMSFT>(XrSpatialAnchorMSFT)
{
    return XR_OBJECT_TYPE_SPATIAL_ANCHOR_MSFT;
}

template<> inline XrObjectType MWVR::VrDebug::getObjectType<XrHandTrackerEXT>(XrHandTrackerEXT)
{
    return XR_OBJECT_TYPE_HAND_TRACKER_EXT;
}

template<typename T> inline XrObjectType MWVR::VrDebug::getObjectType(T t)
{
    return XR_OBJECT_TYPE_UNKNOWN;
}

#endif 
