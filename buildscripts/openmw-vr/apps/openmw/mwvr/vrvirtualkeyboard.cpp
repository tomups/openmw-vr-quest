#include "vrvirtualkeyboard.hpp"

#include <MyGUI_InputManager.h>
#include <MyGUI_LayerManager.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/statemanager.hpp"

namespace MWVR
{
    VirtualKeyboardManager::VirtualKeyboardManager()
        : mVk(new VrVirtualKeyboard)
    {

    }

    void VirtualKeyboardManager::registerEditBox(MyGUI::EditBox* editBox)
    {
        IDelegate* onSetFocusDelegate = newDelegate(mVk.get(), &VrVirtualKeyboard::delegateOnSetFocus);
        IDelegate* onLostFocusDelegate = newDelegate(mVk.get(), &VrVirtualKeyboard::delegateOnLostFocus);
        editBox->eventKeySetFocus += onSetFocusDelegate;
        editBox->eventKeyLostFocus += onLostFocusDelegate;

        mDelegates[editBox] = Delegates(onSetFocusDelegate, onLostFocusDelegate);
    }

    void VirtualKeyboardManager::unregisterEditBox(MyGUI::EditBox* editBox)
    {
        auto it = mDelegates.find(editBox);
        if (it != mDelegates.end())
        {
            editBox->eventKeySetFocus -= it->second.first;
            editBox->eventKeyLostFocus -= it->second.second;
            mDelegates.erase(it);
        }
    }


    static const char* mClassTypeName;

    VrVirtualKeyboard::VrVirtualKeyboard()
        : WindowBase("openmw_vr_virtual_keyboard.layout")
        , mButtonBox(nullptr)
        , mTarget(nullptr)
        , mButtons()
        , mShift(false)
        , mCaps(false)
    {
        getWidget(mButtonBox, "ButtonBox");
        mMainWidget->setNeedKeyFocus(false);
        mButtonBox->setNeedKeyFocus(false);
        updateMenu();
    }

    VrVirtualKeyboard::~VrVirtualKeyboard()
    {

    }

    void VrVirtualKeyboard::onResChange(int w, int h)
    {
        updateMenu();
    }

    void VrVirtualKeyboard::onFrame(float dt)
    {
    }

    void VrVirtualKeyboard::open(MyGUI::EditBox* target)
    {
        updateMenu();

        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(target);
        mTarget = target;
        setVisible(true);
    }

    void VrVirtualKeyboard::close()
    {
        setVisible(false);
        mTarget = nullptr;
    }

    void VrVirtualKeyboard::delegateOnSetFocus(MyGUI::Widget* _sender, MyGUI::Widget* _old)
    {
        if (_sender->getUserString("VirtualKeyboard") != "false")
            open(static_cast<MyGUI::EditBox*>(_sender));
    }

    void VrVirtualKeyboard::delegateOnLostFocus(MyGUI::Widget* _sender, MyGUI::Widget* _new)
    {
        close();
    }

    void VrVirtualKeyboard::onButtonClicked(MyGUI::Widget* sender)
    {
        assert(mTarget);
        MyGUI::InputManager::getInstance().setKeyFocusWidget(mTarget);

        std::string name = *sender->getUserData<std::string>();

        if (name == "Esc")
            onEsc();
        if (name == "Tab")
            onTab();
        if (name == "Caps")
            onCaps();
        if (name == "Shift")
            onShift();
        else
            mShift = false;
        if (name == "Back")
            onBackspace();
        if (name == "Return")
            onReturn();
        if (name == "Space")
            textInput(" ");
        if (name == "->")
            textInput("->");
        if (name.length() == 1)
            textInput(name);

        updateMenu();
    }

    void VrVirtualKeyboard::textInput(const std::string& symbol)
    {
        MyGUI::UString ustring(symbol);
        MyGUI::UString::utf32string utf32string = ustring.asUTF32();
        for (MyGUI::UString::utf32string::const_iterator it = utf32string.begin(); it != utf32string.end(); ++it)
            MyGUI::InputManager::getInstance().injectKeyPress(MyGUI::KeyCode::None, *it);
    }

    void VrVirtualKeyboard::onEsc()
    {
        close();
    }

    void VrVirtualKeyboard::onTab()
    {
        MyGUI::InputManager::getInstance().injectKeyPress(MyGUI::KeyCode::Tab);
        MyGUI::InputManager::getInstance().injectKeyRelease(MyGUI::KeyCode::Tab);
    }

    void VrVirtualKeyboard::onCaps()
    {
        mCaps = !mCaps;
    }

    void VrVirtualKeyboard::onShift()
    {
        mShift = !mShift;
    }

    void VrVirtualKeyboard::onBackspace()
    {
        MyGUI::InputManager::getInstance().injectKeyPress(MyGUI::KeyCode::Backspace);
        MyGUI::InputManager::getInstance().injectKeyRelease(MyGUI::KeyCode::Backspace);
    }

    void VrVirtualKeyboard::onReturn()
    {
        MyGUI::InputManager::getInstance().injectKeyPress(MyGUI::KeyCode::Return);
        MyGUI::InputManager::getInstance().injectKeyRelease(MyGUI::KeyCode::Return);
    }

    bool VrVirtualKeyboard::exit()
    {
        close();
        return true;
    }

    void VrVirtualKeyboard::updateMenu()
    {
        // TODO: Localization?
        static std::vector<std::string> row1{ "`",  "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "Back" };
        static std::vector<std::string> row2{ "Tab",   "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "Return" };
        static std::vector<std::string> row3{ "Caps",  "a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'", "\\", "->" };
        static std::vector<std::string> row4{ "Shift", "z", "x", "c", "v", "b", "n", "m", ",", ".", "/", "Space" };
        std::map<std::string, std::string> shiftMap;
        shiftMap["1"] = "!";
        shiftMap["2"] = "@";
        shiftMap["3"] = "#";
        shiftMap["4"] = "$";
        shiftMap["5"] = "%";
        shiftMap["6"] = "^";
        shiftMap["7"] = "&";
        shiftMap["8"] = "*";
        shiftMap["9"] = "(";
        shiftMap["0"] = ")";
        shiftMap["-"] = "_";
        shiftMap["="] = "+";
        shiftMap["\\"] = "|";
        shiftMap[","] = "<";
        shiftMap["."] = ">";
        shiftMap["/"] = "?";
        shiftMap[";"] = ":";
        shiftMap["'"] = "\"";
        shiftMap["["] = "{";
        shiftMap["]"] = "}";
        shiftMap["`"] = "~";

        std::vector< std::vector< std::string > > rows{ row1, row2, row3, row4 };

        int sideSize = 50;
        int margin = 10;
        int xmax = 0;
        int ymax = 0;

        if (mButtons.empty())
        {
            int y = margin;
            for (auto& row : rows)
            {
                int x = margin;
                for (std::string& buttonId : row)
                {
                    int width = sideSize + 10 * (buttonId.length() - 1);
                    MyGUI::Button* button = mButtonBox->createWidget<MyGUI::Button>(
                        "MW_Button", MyGUI::IntCoord(x, y, width, sideSize), MyGUI::Align::Default, buttonId);
                    button->eventMouseButtonClick += MyGUI::newDelegate(this, &VrVirtualKeyboard::onButtonClicked);
                    button->setUserData(std::string(buttonId));
                    button->setVisible(true);
                    button->setFontHeight(32);
                    button->setCaption(buttonId);
                    button->setNeedKeyFocus(false);
                    mButtons[buttonId] = button;
                    x += width + margin;
                }
                y += sideSize + margin;
            }
        }

        for (auto& row : rows)
        {
            for (std::string& buttonId : row)
            {
                auto* button = mButtons[buttonId];
                xmax = std::max(xmax, button->getAbsoluteRect().right);
                ymax = std::max(ymax, button->getAbsoluteRect().bottom);

                if (buttonId.length() == 1)
                {
                    auto caption = buttonId;
                    if (mShift ^ mCaps)
                        caption[0] = std::toupper(caption[0]);
                    else
                        caption[0] = std::tolower(caption[0]);
                    button->setCaption(caption);
                    button->setUserData(caption);
                }

                if (mShift)
                {
                    auto it = shiftMap.find(buttonId);
                    if (it != shiftMap.end())
                    {
                        button->setCaption(it->second);
                        button->setUserData(it->second);
                    }
                }
            }
        }

        std::cout << xmax << ", " << ymax << std::endl;

        setCoord(0, 0, xmax + margin, ymax + margin);
        mButtonBox->setCoord(0, 0, xmax + margin, ymax + margin);
        //mButtonBox->setCoord (margin, margin, width, height);
        mButtonBox->setVisible(true);
    }
}
