#include "virtualkeyboardmanager.hpp"

namespace Gui
{
    Gui::VirtualKeyboardManager* sInstance = nullptr;

    VirtualKeyboardManager* Gui::VirtualKeyboardManager::instance()
    {
        return sInstance;
    }

    VirtualKeyboardManager::VirtualKeyboardManager()
    {
        if (!sInstance)
            sInstance = this;
        else
            throw std::logic_error("Duplicated Gui::VirtualKeyboardManager singleton");
    }
}
