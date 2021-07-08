#ifndef OPENMW_GAME_MWVR_VRMETAMENU_H
#define OPENMW_GAME_MWVR_VRMETAMENU_H

#include "../mwgui/windowbase.hpp"

#include <MyGUI_Button.h>
#include "components/widgets/box.hpp"

namespace Gui
{
    class ImageButton;
}

namespace VFS
{
    class Manager;
}

namespace MWVR
{
    class VrMetaMenu : public MWGui::WindowBase
    {
            int mWidth;
            int mHeight;

            bool mHasAnimatedMenu;

        public:

            VrMetaMenu(int w, int h);
            ~VrMetaMenu();

            void onResChange(int w, int h) override;

            void setVisible (bool visible) override;

            void onFrame(float dt) override;

            bool exit() override;

        private:
            std::map<std::string, MyGUI::Button*> mButtons{};

            void onButtonClicked (MyGUI::Widget* sender);
            void onConsole();
            void onJournal();
            void onGameMenu();
            void onInventory();
            void onRest();
            void onQuickMenu();
            void onQuickLoad();
            void onQuickSave();
            void onRecenter();
            void close();

            void updateMenu();
    };

}

#endif
