#include "vrbindings.hpp"

#include <components/vr/vr.hpp>

namespace MWLua
{
    sol::table initVRPackage(const Context& context)
    {
        sol::state_view lua = context.sol();
        sol::table api(lua, sol::create);

        api["isVr"] = []() -> bool { return VR::getVR(); };

        return LuaUtil::makeReadOnly(api);
    }
}

