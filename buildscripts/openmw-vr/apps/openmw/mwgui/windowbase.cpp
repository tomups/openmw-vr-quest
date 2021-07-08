#include "windowbase.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_RenderManager.h>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/environment.hpp"

#include <components/widgets/imagebutton.hpp>

#include "draganddrop.hpp"
#include "exposedwindow.hpp"

#ifdef USE_OPENXR
#include "../mwvr/vrgui.hpp"
#include "../mwvr/vrenvironment.hpp"
#endif

using namespace MWGui;

WindowBase::WindowBase(const std::string& parLayout)
  : Layout(parLayout)
{
    mMainWidget->setVisible(false);

    Window* window = mMainWidget->castType<Window>(false);
    if (!window)
        return;

    MyGUI::Button* button = nullptr;
    MyGUI::VectorWidgetPtr widgets = window->getSkinWidgetsByName("Action");
    for (MyGUI::Widget* widget : widgets)
    {
        if (widget->isUserString("SupportDoubleClick"))
            button = widget->castType<MyGUI::Button>();
    }

    if (button)
        button->eventMouseButtonDoubleClick += MyGUI::newDelegate(this, &WindowBase::onDoubleClick);
}

void WindowBase::onTitleDoubleClicked()
{
    if (MyGUI::InputManager::getInstance().isShiftPressed())
        MWBase::Environment::get().getWindowManager()->toggleMaximized(this);
}

void WindowBase::onDoubleClick(MyGUI::Widget *_sender)
{
    onTitleDoubleClicked();
}

void WindowBase::setVisible(bool visible)
{
    bool wasVisible = mMainWidget->getVisible();
    mMainWidget->setVisible(visible);

    if (visible)
        onOpen();
    else if (wasVisible)
        onClose();

#ifdef USE_OPENXR
    // Check that onOpen/onClose didn't reverse the change before forwarding it
    // to the VR GUI manager.
    if (this->isVisible() == visible)
    {
        auto* vrGUIManager = MWVR::Environment::get().getGUIManager();
        if (!vrGUIManager)
            // May end up here before before rendering has been fully set up
            return;
        vrGUIManager->setVisible(this, visible);
    }
#endif
}

bool WindowBase::isVisible()
{
    return mMainWidget->getVisible();
}

void WindowBase::center()
{
    // Centre dialog

    MyGUI::IntSize layerSize = MyGUI::RenderManager::getInstance().getViewSize();
    if (mMainWidget->getLayer())
        layerSize = mMainWidget->getLayer()->getSize();

    MyGUI::IntCoord coord = mMainWidget->getCoord();
    coord.left = (layerSize.width - coord.width)/2;
    coord.top = (layerSize.height - coord.height)/2;
    mMainWidget->setCoord(coord);
}

WindowModal::WindowModal(const std::string& parLayout)
    : WindowBase(parLayout)
{
}

void WindowModal::onOpen()
{
    MWBase::Environment::get().getWindowManager()->addCurrentModal(this); //Set so we can escape it if needed

    MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getKeyFocusWidget();
    MyGUI::InputManager::getInstance ().addWidgetModal (mMainWidget);
    MyGUI::InputManager::getInstance().setKeyFocusWidget(focus);
}

void WindowModal::onClose()
{
    MWBase::Environment::get().getWindowManager()->removeCurrentModal(this);

    MyGUI::InputManager::getInstance ().removeWidgetModal (mMainWidget);
}

NoDrop::NoDrop(DragAndDrop *drag, MyGUI::Widget *widget)
    : mWidget(widget), mDrag(drag), mTransparent(false)
{
}

void NoDrop::onFrame(float dt)
{
    if (!mWidget)
        return;

    MyGUI::IntPoint mousePos = MyGUI::InputManager::getInstance().getMousePosition();

#ifdef USE_OPENXR
    // Since VR mode stretches some windows to full screen, the usual outside condition
    // won't work
    mTransparent = false;
#endif
    if (mDrag->mIsOnDragAndDrop)
    {
        MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getMouseFocusWidget();
        while (focus && focus != mWidget)
        {
            focus = focus->getParent();
        }

        if (focus == mWidget)
        {
            mTransparent = true;
        }
    }
    if (!mWidget->getAbsoluteCoord().inside(mousePos))
        mTransparent = false;

    if (mTransparent)
    {
#ifndef USE_OPENXR
        // These makes focus null, which messes up the logic for VR
        // since i reset mTransparent to false every update.
        // TODO: Is there a cleaner way?
        mWidget->setNeedMouseFocus(false); // Allow click-through
#endif
        setAlpha(std::max(0.13f, mWidget->getAlpha() - dt*5));
    }
    else
    {
#ifndef USE_OPENXR
        mWidget->setNeedMouseFocus(true); // Allow click-through
#endif
        setAlpha(std::min(1.0f, mWidget->getAlpha() + dt*5));
    }
}

void NoDrop::setAlpha(float alpha)
{
    if (mWidget)
        mWidget->setAlpha(alpha);
}

BookWindowBase::BookWindowBase(const std::string& parLayout)
  : WindowBase(parLayout)
{
}

float BookWindowBase::adjustButton (char const * name)
{
    Gui::ImageButton* button;
    WindowBase::getWidget (button, name);
    MyGUI::IntSize requested = button->getRequestedSize();
    float scale = float(requested.height) / button->getSize().height;
    MyGUI::IntSize newSize = requested;
    newSize.width /= scale;
    newSize.height /= scale;
    button->setSize(newSize);

    if (button->getAlign().isRight())
    {
        MyGUI::IntSize diff = (button->getSize() - requested);
        diff.width /= scale;
        diff.height /= scale;
        button->setPosition(button->getPosition() + MyGUI::IntPoint(diff.width,0));
    }

    return scale;
}
