#ifndef OPENMW_WIDGETS_VIRTUALKEYBOARDMANAGER_H
#define OPENMW_WIDGETS_VIRTUALKEYBOARDMANAGER_H

#include <MyGUI_EditBox.h>
#include "MyGUI_Singleton.h"

namespace Gui
{
    class VirtualKeyboardManager :
        public MyGUI::Singleton<VirtualKeyboardManager>
    {
    public:
        virtual void registerEditBox(MyGUI::EditBox* editBox) = 0;
        virtual void unregisterEditBox(MyGUI::EditBox* editBox) = 0;
    };
}

#endif
