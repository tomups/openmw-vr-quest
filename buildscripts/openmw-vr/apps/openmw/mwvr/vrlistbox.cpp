#include "vrlistbox.hpp"

#include <MyGUI_InputManager.h>
#include <MyGUI_LayerManager.h>
#include <MyGUI_ComboBox.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/statemanager.hpp"

namespace MWVR
{
    VrListBox::VrListBox()
        : WindowModal("openmw_vr_listbox.layout")
        , mOK(nullptr)
        , mCancel(nullptr)
        , mListBox(nullptr)
        , mIndex(MyGUI::ITEM_NONE)
        , mCallback()
    {
        getWidget(mOK, "OkButton");
        getWidget(mCancel, "CancelButton");
        getWidget(mListBox, "ListBox");

        mOK->eventMouseButtonClick += MyGUI::newDelegate(this, &VrListBox::onOKButtonClicked);
        mCancel->eventMouseButtonClick += MyGUI::newDelegate(this, &VrListBox::onCancelButtonClicked);
        mListBox->eventListChangePosition += MyGUI::newDelegate(this, &VrListBox::onItemChanged);
        mListBox->eventListSelectAccept += MyGUI::newDelegate(this, &VrListBox::onListAccept);
    }

    VrListBox::~VrListBox()
    {

    }

    void VrListBox::open(MyGUI::ComboBox* comboBox, Callback callback)
    {
        mCallback = callback;
        mIndex = MyGUI::ITEM_NONE;
        mListBox->removeAllItems();
        mListBox->setIndexSelected(MyGUI::ITEM_NONE);

        for (unsigned i = 0; i < comboBox->getItemCount(); i++)
            mListBox->addItem(comboBox->getItemNameAt(i));

        mListBox->setItemSelect(comboBox->getIndexSelected());

        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mListBox);

        center();
        setVisible(true);
    }

    void VrListBox::close()
    {
        setVisible(false);
    }

    void VrListBox::onItemChanged(MyGUI::ListBox* _sender, size_t _index)
    {
        mIndex = _index;
    }

    void VrListBox::onCancelButtonClicked(MyGUI::Widget* sender)
    {
        close();

        if (mCallback)
            mCallback(MyGUI::ITEM_NONE);
    }

    void VrListBox::onOKButtonClicked(MyGUI::Widget* sender)
    {
        accept(mIndex);
    }

    void VrListBox::onListAccept(MyGUI::ListBox* sender, size_t pos)
    {
        accept(pos);
    }
    void VrListBox::accept(size_t index)
    {
        close();

        if (mCallback)
            mCallback(index);
    }
}
