#ifndef OPENMW_WIDGETS_VIRTUALKEYBOARDMANAGER_H
#define OPENMW_WIDGETS_VIRTUALKEYBOARDMANAGER_H

#include "MyGUI_Singleton.h"
#include <MyGUI_EditBox.h>

namespace Gui
{
    class VirtualKeyboardManager
    {
    public:
        VirtualKeyboardManager();
        virtual ~VirtualKeyboardManager() = default;

        static VirtualKeyboardManager* instance();

        virtual void registerEditBox(MyGUI::EditBox* editBox) = 0;
        virtual void unregisterEditBox(MyGUI::EditBox* editBox) = 0;
    };
}

#endif
