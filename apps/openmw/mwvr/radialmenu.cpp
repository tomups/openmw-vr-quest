#include "radialmenu.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>
#include <MyGUI_Gui.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_RenderManager.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwgui/itemwidget.hpp"
#include "../mwgui/quickkeysmenu.hpp"

#include <components/vr/session.hpp>
#include <components/esm3/quickkeys.hpp>

#include "vrutil.hpp"

namespace MWVR
{

    RadialMenu::RadialMenu(int w, int h, MWGui::QuickKeysMenu* qkm)
        : WindowBase("openmw_vr_radial_menu.layout")
        , mWidth(w)
        , mHeight(h)
        , mQkm(qkm)
    {
        initMenu();
        updateMenu();
    }

    RadialMenu::~RadialMenu() {}

    void RadialMenu::onResChange(int w, int h)
    {
        mWidth = w;
        mHeight = h;

        updateMenu();
    }

    void RadialMenu::setVisible(bool visible)
    {
        if (visible)
            updateMenu();

        Layout::setVisible(visible);
    }

    void RadialMenu::onFrame(float dt) {}

    void RadialMenu::close()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(MWGui::GM_RadialMenu);
    }

    void RadialMenu::onButtonClicked(MyGUI::Widget* sender)
    {
        auto userString = sender->getUserString("QuickKey");
        close();
        mQkm->activateQuickKey(std::stoi(std::string(userString)));
    }

    bool RadialMenu::exit()
    {
        return MWBase::Environment::get().getStateManager()->getState() == MWBase::StateManager::State_Running;
    }

    void RadialMenu::initMenu()
    {
        for (uint32_t i = 0; i < 10; i++)
        {
            std::string buttonId = "QuickKey" + std::to_string(i + 1);
            MyGUI::Widget* button = nullptr;
            getWidget(button, buttonId);

            auto size = button->getSize();
            float angleRadian = static_cast<float>(i) * 2.f * osg::PIf / 10.f;
            MyGUI::FloatPoint position = { std::cos(angleRadian) * 150.f, std::sin(angleRadian) * 150.f };

            MyGUI::FloatPoint offset = { static_cast<float>(size.width) / 2.f, static_cast<float>(size.height) / 2.f };

            position += MyGUI::FloatPoint{ 512.f / 2.f, 512.f / 2.f } - offset;

            button->setPosition(MyGUI::IntPoint{ static_cast<int>(position.left), static_cast<int>(position.top) });
            button->eventMouseButtonClick += MyGUI::newDelegate(this, &RadialMenu::onButtonClicked);
            button->setUserString("QuickKey", std::to_string(i + 1));
            button->setVisible(true);
        }
    }

    void RadialMenu::updateMenu()
    {
        for (uint32_t i = 0; i < 10; i++)
        {
            std::string buttonId = "QuickKey" + std::to_string(i + 1);
            MWGui::ItemWidget* button = nullptr;
            getWidget(button, buttonId);
            button->clearUserStrings();
            button->setUserString("QuickKey", std::to_string(i + 1));
            button->setItem(MWWorld::Ptr());

            while (button->getChildCount()) // Destroy number label
                MyGUI::Gui::getInstance().destroyWidget(button->getChildAt(0));

            auto* key = mQkm->keyAt(i);

            for (const auto& userString : key->button->getUserStrings())
            {
                button->setUserString(userString.first, userString.second);
            }

            if (key->type == ESM::QuickKeys::Type::HandToHand)
            {
                MyGUI::ImageBox* image = button->createWidget<MyGUI::ImageBox>(
                    "ImageBox", MyGUI::IntCoord(14, 13, 32, 32), MyGUI::Align::Default);

                image->setImageTexture("icons\\k\\stealth_handtohand.dds");
                image->setNeedMouseFocus(false);
            }
            else if (key->type == ESM::QuickKeys::Type::Unassigned)
            {
                MyGUI::TextBox* textBox = button->createWidgetReal<MyGUI::TextBox>(
                    "SandText", MyGUI::FloatCoord(0, 0, 1, 1), MyGUI::Align::Default);

                textBox->setTextAlign(MyGUI::Align::Center);
                textBox->setCaption(MyGUI::utility::toString(key->index));
                textBox->setNeedMouseFocus(false);
            }
            else
            {
                button->setIcon(key->button->getIcon());
                if (key->button->hasFrame())
                    button->setFrame(key->button->getFrame(), key->button->getFrameCoords());

                MWWorld::Ptr* item = key->button->getUserData<MWWorld::Ptr>(false);
                if (item)
                    button->setUserData(*item);
            }
        }
    }
}
