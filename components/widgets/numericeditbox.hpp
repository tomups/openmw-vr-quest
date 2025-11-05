#ifndef OPENMW_NUMERIC_EDIT_BOX_H
#define OPENMW_NUMERIC_EDIT_BOX_H

#include <MyGUI_EditBox.h>

//## VR_PATCH BEGIN
#include "box.hpp"
//## VR_PATCH END

namespace Gui
{

    /**
     * @brief A variant of the EditBox that only allows integer inputs
     */
//## VR_PATCH BEGIN
    class NumericEditBox final : public Gui::EditBox
//## VR_PATCH END
    {
        MYGUI_RTTI_DERIVED(NumericEditBox)

    public:
        NumericEditBox()
            : mValue(0)
            , mMinValue(std::numeric_limits<int>::min())
            , mMaxValue(std::numeric_limits<int>::max())
        {
        }

        void initialiseOverride() override;
        void shutdownOverride() override;

        typedef MyGUI::delegates::MultiDelegate<int> EventHandle_ValueChanged;
        EventHandle_ValueChanged eventValueChanged;

        /// @note Does not trigger eventValueChanged
        void setValue(int value);
        int getValue();

        void setMinValue(int minValue);
        void setMaxValue(int maxValue);

    private:
        void onEditTextChange(MyGUI::EditBox* sender);
        void onKeyLostFocus(MyGUI::Widget* newWidget) override;
        void onKeyButtonPressed(MyGUI::KeyCode key, MyGUI::Char character) override;

        int mValue;

        int mMinValue;
        int mMaxValue;
    };

}

#endif
