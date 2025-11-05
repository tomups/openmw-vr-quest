#ifndef MWGUI_ALCHEMY_H
#define MWGUI_ALCHEMY_H

#include <memory>
#include <vector>

#include <MyGUI_ComboBox.h>
#include <MyGUI_ControllerItem.h>
#include <MyGUI_ListBox.h>

#include <components/widgets/box.hpp>
#include <components/widgets/numericeditbox.hpp>

#include "itemselection.hpp"
#include "windowbase.hpp"

#include "../mwmechanics/alchemy.hpp"

//## VR_PATCH BEGIN
namespace MWVR
{
    class VrListBox;
}
//## VR_PATCH END

namespace MWGui
{
    class ItemView;
    class ItemWidget;
    class InventoryItemModel;
    class SortFilterItemModel;

    class AlchemyWindow : public WindowBase
    {
    public:
        AlchemyWindow();

        void onOpen() override;

        void onResChange(int, int) override { center(); }

        std::string_view getWindowIdForLua() const override { return "Alchemy"; }

    private:
        static const float sCountChangeInitialPause; // in seconds
        static const float sCountChangeInterval; // in seconds

        std::string mSuggestedPotionName;
        enum class FilterType
        {
            ByName,
            ByEffect
        };
        FilterType mCurrentFilter;

        std::unique_ptr<ItemSelectionDialog> mItemSelectionDialog;

        ItemView* mItemView;
        InventoryItemModel* mModel;
        SortFilterItemModel* mSortModel;

        MyGUI::Button* mCreateButton;
        MyGUI::Button* mCancelButton;

        MyGUI::Widget* mEffectsBox;

        MyGUI::Button* mIncreaseButton;
        MyGUI::Button* mDecreaseButton;
        Gui::AutoSizedButton* mFilterType;
        MyGUI::ComboBox* mFilterValue;
        MyGUI::EditBox* mNameEdit;
        Gui::NumericEditBox* mBrewCountEdit;

        void onCancelButtonClicked(MyGUI::Widget* sender);
        void onCreateButtonClicked(MyGUI::Widget* sender);
        void onIngredientSelected(MyGUI::Widget* sender);
        void onApparatusSelected(MyGUI::Widget* sender);
        void onAccept(MyGUI::EditBox*);
        void onIncreaseButtonPressed(MyGUI::Widget* sender, int left, int top, MyGUI::MouseButton id);
        void onDecreaseButtonPressed(MyGUI::Widget* sender, int left, int top, MyGUI::MouseButton id);
        void onCountButtonReleased(MyGUI::Widget* sender, int left, int top, MyGUI::MouseButton id);
        void onCountValueChanged(int value);
        void onRepeatClick(MyGUI::Widget* widget, MyGUI::ControllerItem* controller);

        void applyFilter(const std::string& filter);
        void initFilter();
        void onFilterChanged(MyGUI::ComboBox* sender, size_t index);
        void onFilterEdited(MyGUI::EditBox* sender);
        void switchFilterType(MyGUI::Widget* sender);
        void updateFilters();

        void addRepeatController(MyGUI::Widget* widget);

        void onIncreaseButtonTriggered();
        void onDecreaseButtonTriggered();

        void onSelectedItem(int index);

        void onItemSelected(MWWorld::Ptr item);
        void onItemCancel();

        void createPotions(int count);

        void update();

        std::unique_ptr<MWMechanics::Alchemy> mAlchemy;

        std::vector<ItemWidget*> mApparatus;
        std::vector<ItemWidget*> mIngredients;

        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;
        void filterListButtonHandler(const SDL_ControllerButtonEvent& arg);
//## VR_PATCH BEGIN
        // MERGETODO: Upstream changed things in this class. Test.
        MyGUI::ComboBox* mFilterCombo;
        MyGUI::EditBox* mFilterEdit;
        MyGUI::Button* mFilterButton;
        MWVR::VrListBox* mFilterListBox = nullptr;
        std::set<std::string> mItemNames;
        std::set<std::string> mItemEffects;

        void onFilterButtonClicked(MyGUI::Widget* _sender);
        const std::set<std::string>& items();
//## VR_PATCH END
    };
}

#endif
