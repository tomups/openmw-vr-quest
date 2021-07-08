#ifndef OPENMW_GAME_MWVR_VRVIRTUALKEYBOARD_H
#define OPENMW_GAME_MWVR_VRVIRTUALKEYBOARD_H

#include "../mwgui/windowbase.hpp"

#include <MyGUI_Button.h>
#include "components/widgets/virtualkeyboardmanager.hpp"

#include <map>
#include <memory>

namespace Gui
{
    class VirtualKeyboardManager;
}

namespace MWVR
{
    class VrVirtualKeyboard : public MWGui::WindowBase
    {
    public:

        VrVirtualKeyboard();
        ~VrVirtualKeyboard();

        void onResChange(int w, int h) override;

        void onFrame(float dt) override;

        bool exit() override;

        void open(MyGUI::EditBox* target);

        void close();

        void delegateOnSetFocus(MyGUI::Widget* _sender, MyGUI::Widget* _old);
        void delegateOnLostFocus(MyGUI::Widget* _sender, MyGUI::Widget* _old);

    private:
        void onButtonClicked(MyGUI::Widget* sender);
        void textInput(const std::string& symbol);
        void onEsc();
        void onTab();
        void onCaps();
        void onShift();
        void onBackspace();
        void onReturn();
        void updateMenu();

        MyGUI::Widget* mButtonBox;
        MyGUI::EditBox* mTarget;
        std::map<std::string, MyGUI::Button*> mButtons;
        bool mShift;
        bool mCaps;
    };

    class VirtualKeyboardManager : public Gui::VirtualKeyboardManager
    {
    public:
        VirtualKeyboardManager();

        void registerEditBox(MyGUI::EditBox* editBox) override;
        void unregisterEditBox(MyGUI::EditBox* editBox) override;
        VrVirtualKeyboard& virtualKeyboard() { return *mVk; };

    private:
        std::unique_ptr<VrVirtualKeyboard>   mVk;

        // MyGUI deletes delegates when you remove them from an event.
        // Therefore i need one pair of delegates per box instead of being able to reuse one pair.
        // And i have to set them aside myself to know what to remove from each event.
        // There is an IDelegateUnlink type that might simplify this, but it is poorly documented.
        using IDelegate = MyGUI::EventHandle_WidgetWidget::IDelegate;
        // .first = onSetFocus, .second = onLostFocus
        using Delegates = std::pair<IDelegate*, IDelegate*>;
        std::map<MyGUI::EditBox*, Delegates> mDelegates;
    };
}

#endif
