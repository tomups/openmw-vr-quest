
#ifndef MWLUA_VRBINDINGS_H
#define MWLUA_VRBINDINGS_H

#include <sol/forward.hpp>

#include "context.hpp"

namespace MWLua
{
    sol::table initVRPackage(const Context&);
}

#endif // MWLUA_VRBINDINGS_H
