#include "layout.hpp"

#include <MyGUI_LayoutManager.h>
#include <MyGUI_Widget.h>
#include <MyGUI_Gui.h>
#include <MyGUI_TextBox.h>
#include <MyGUI_Window.h>
#include <MyGUI_OverlappedLayer.h>
#include <MyGUI_SharedLayer.h>

#ifdef USE_OPENXR
#include "../mwvr/vrgui.hpp"
#include "../mwvr/vrenvironment.hpp"
#endif

namespace MWGui
{
    void Layout::initialise(const std::string& _layout, MyGUI::Widget* _parent)
    {
        const std::string MAIN_WINDOW = "_Main";
        mLayoutName = _layout;

        if (mLayoutName.empty())
            mMainWidget = _parent;
        else
        {
            mPrefix = MyGUI::utility::toString(this, "_");
            mListWindowRoot = MyGUI::LayoutManager::getInstance().loadLayout(mLayoutName, mPrefix, _parent);

            const std::string main_name = mPrefix + MAIN_WINDOW;
            for (MyGUI::Widget* widget : mListWindowRoot)
            {
                if (widget->getName() == main_name)
                {
                    mMainWidget = widget;
                    break;
                }
            }
            MYGUI_ASSERT(mMainWidget, "root widget name '" << MAIN_WINDOW << "' in layout '" << mLayoutName << "' not found.");
        }
    }

    void Layout::shutdown()
    {
        setVisible(false);
        MyGUI::Gui::getInstance().destroyWidget(mMainWidget);
        mListWindowRoot.clear();
    }

    void Layout::setCoord(int x, int y, int w, int h)
    {
        mMainWidget->setCoord(x,y,w,h);
    }

    void Layout::setCoordf(float x, float y, float w, float h)
    {
        mMainWidget->setRealCoord(x, y, w, h);
    }

    void Layout::setVisible(bool b)
    {
        mMainWidget->setVisible(b);
#ifdef USE_OPENXR
        auto* vrGUIManager = MWVR::Environment::get().getGUIManager();
        if (!vrGUIManager)
            // May end up here before before rendering has been fully set up
            return;
        vrGUIManager->setVisible(this, b);
#endif
    }

    void Layout::setText(const std::string &name, const std::string &caption)
    {
        MyGUI::Widget* pt;
        getWidget(pt, name);
        static_cast<MyGUI::TextBox*>(pt)->setCaption(caption);
    }

    void Layout::setTitle(const std::string& title)
    {
        MyGUI::Window* window = static_cast<MyGUI::Window*>(mMainWidget);

        if (window->getCaption() != title)
            window->setCaptionWithReplacing(title);
    }

    MyGUI::Widget* Layout::getWidget(const std::string &_name)
    {
        for (MyGUI::Widget* widget : mListWindowRoot)
        {
            MyGUI::Widget* find = widget->findWidget(mPrefix + _name);
            if (nullptr != find)
            {
                return find;
            }
        }
        MYGUI_EXCEPT("widget name '" << _name << "' in layout '" << mLayoutName << "' not found.");
    }

}
