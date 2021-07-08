#include "vrmetamenu.hpp"

#include <MyGUI_InputManager.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/statemanager.hpp"

#include "vrenvironment.hpp"
#include "vrinputmanager.hpp"

namespace MWVR
{

    VrMetaMenu::VrMetaMenu(int w, int h)
        : WindowBase("openmw_vr_metamenu.layout")
        , mWidth (w)
        , mHeight (h)
    {
        updateMenu();
    }

    VrMetaMenu::~VrMetaMenu()
    {
        
    }

    void VrMetaMenu::onResChange(int w, int h)
    {
        mWidth = w;
        mHeight = h;

        updateMenu();
    }

    void VrMetaMenu::setVisible (bool visible)
    {
        if (visible)
            updateMenu();

        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mButtons["return"]);

        Layout::setVisible (visible);
    }

    void VrMetaMenu::onFrame(float dt)
    {
    }

    void VrMetaMenu::onConsole()
    {
        if (MyGUI::InputManager::getInstance().isModalAny())
            return;

        MWBase::Environment::get().getWindowManager()->toggleConsole();
    }

    void VrMetaMenu::onGameMenu()
    {
        MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_MainMenu);
    }

    void VrMetaMenu::onJournal()
    {
        MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Journal);
    }

    void VrMetaMenu::onInventory()
    {
        MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Inventory);
    }

    void VrMetaMenu::onRest()
    {
        if (!MWBase::Environment::get().getWindowManager()->getRestEnabled() || MWBase::Environment::get().getWindowManager()->isGuiMode())
            return;

        MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Rest); //Open rest GUI
    }

    void VrMetaMenu::onQuickMenu()
    {
        MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_QuickKeysMenu);
    }

    void VrMetaMenu::onQuickLoad()
    {
        if (!MyGUI::InputManager::getInstance().isModalAny())
            MWBase::Environment::get().getStateManager()->quickLoad();
    }

    void VrMetaMenu::onQuickSave()
    {
        if (!MyGUI::InputManager::getInstance().isModalAny())
            MWBase::Environment::get().getStateManager()->quickSave();
    }

    void VrMetaMenu::onRecenter()
    {
        Environment::get().getInputManager()->requestRecenter(true);
    }

    void VrMetaMenu::close()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(MWGui::GM_VrMetaMenu);
    }

    void VrMetaMenu::onButtonClicked(MyGUI::Widget *sender)
    {
        std::string name = *sender->getUserData<std::string>();
        close();
        if (name == "console")
            onConsole();
        else if (name == "gamemenu")
            onGameMenu();
        else if (name == "journal")
            onJournal();
        else if (name == "inventory")
            onInventory();
        else if (name == "rest")
            onRest();
        else if (name == "quickmenu")
            onQuickMenu();
        else if (name == "quickload")
            onQuickLoad();
        else if (name == "quicksave")
            onQuickSave();       
        else if (name == "recenter")
            onRecenter();
    }

    bool VrMetaMenu::exit()
    {
        return MWBase::Environment::get().getStateManager()->getState() == MWBase::StateManager::State_Running;
    }

    void VrMetaMenu::updateMenu()
    {
        static std::vector<std::string> buttons{ "return", "recenter", "quicksave", "quickload", "console", "inventory", "journal", "rest", "quickmenu", "gamemenu" };

        if(mButtons.empty())
        for (std::string& buttonId : buttons)
        {
            MyGUI::Button* button = nullptr;
            getWidget(button, buttonId);
            if (!button)
                throw std::logic_error( std::string() + "Unable to find button \"" + buttonId + "\"");
            button->eventMouseButtonClick += MyGUI::newDelegate(this, &VrMetaMenu::onButtonClicked);
            button->setUserData(std::string(buttonId));
            button->setVisible(true);
            mButtons[buttonId] = button;
        }
    }
}
