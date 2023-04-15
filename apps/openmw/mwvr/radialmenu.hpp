#ifndef OPENMW_GAME_MWVR_RADIALMENU_H
#define OPENMW_GAME_MWVR_RADIALMENU_H

#include "../mwgui/windowbase.hpp"
#include "components/widgets/box.hpp"
#include <MyGUI_Button.h>

#include <array>

namespace Gui
{
    class ImageButton;
}

namespace VFS
{
    class Manager;
}

namespace MWGui
{
    class QuickKeysMenu;
}

namespace MWVR
{
    class RadialMenu : public MWGui::WindowBase
    {
        int mWidth;
        int mHeight;
        MWGui::QuickKeysMenu* mQkm;

    public:
        RadialMenu(int w, int h, MWGui::QuickKeysMenu* qkm);
        ~RadialMenu();

        void onResChange(int w, int h) override;

        void setVisible(bool visible) override;

        void onFrame(float dt) override;

        bool exit() override;

    private:
        void onButtonClicked(MyGUI::Widget* sender);
        void close();

        void initMenu();
        void updateMenu();
    };

}

#endif
