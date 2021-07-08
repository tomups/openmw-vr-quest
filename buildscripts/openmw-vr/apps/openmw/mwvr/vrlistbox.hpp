#ifndef OPENMW_GAME_MWVR_LISTBOX_H
#define OPENMW_GAME_MWVR_LISTBOX_H

#include "../mwgui/windowbase.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_ListBox.h>
#include "components/widgets/virtualkeyboardmanager.hpp"

#include <set>
#include <memory>

namespace Gui
{
    class VirtualKeyboardManager;
}

namespace MWVR
{
    //! A simple dialogue that presents a list of choices. Used as an alternative to combo boxes in VR.
    class VrListBox : public MWGui::WindowModal
    {
    public:
        using Callback = std::function<void(std::size_t)>;

        VrListBox();
        ~VrListBox();

        void open(MyGUI::ComboBox* comboBox, Callback callback);
        void close();

    private:
        void onItemChanged(MyGUI::ListBox* _sender, size_t _index);
        void onCancelButtonClicked(MyGUI::Widget* sender);
        void onOKButtonClicked(MyGUI::Widget* sender);
        void onListAccept(MyGUI::ListBox* sender, size_t pos);
        void accept(size_t index);

        MyGUI::Widget* mCancel;
        MyGUI::Widget* mOK;
        MyGUI::ListBox* mListBox;
        std::size_t mIndex;
        Callback mCallback;
    };
}

#endif
