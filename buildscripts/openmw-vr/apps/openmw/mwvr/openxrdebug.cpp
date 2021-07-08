#include "openxrdebug.hpp"
#include "openxrmanagerimpl.hpp"
#include "vrenvironment.hpp"

// The OpenXR SDK's platform headers assume we've included these windows headers
#ifdef _WIN32
#include <Windows.h>
#include <objbase.h>

#elif __linux__
#include <X11/Xlib.h>
#include <GL/glx.h>
#undef None

#else
#error Unsupported platform
#endif

#include <openxr/openxr_platform_defines.h>
#include <openxr/openxr_reflection.h>

void MWVR::VrDebug::setName(uint64_t handle, XrObjectType type, const std::string& name)
{
    auto& xrManager = Environment::get().getManager()->impl();
    if (xrManager.xrExtensionIsEnabled(XR_EXT_DEBUG_UTILS_EXTENSION_NAME))
    {
        XrDebugUtilsObjectNameInfoEXT nameInfo{ XR_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr };
        nameInfo.objectHandle = handle;
        nameInfo.objectType = type;
        nameInfo.objectName = name.c_str();

        static PFN_xrSetDebugUtilsObjectNameEXT setDebugUtilsObjectNameEXT
            = reinterpret_cast<PFN_xrSetDebugUtilsObjectNameEXT>(xrManager.xrGetFunction("xrSetDebugUtilsObjectNameEXT"));
        CHECK_XRCMD(setDebugUtilsObjectNameEXT(xrManager.xrInstance(), &nameInfo));
    }
}
