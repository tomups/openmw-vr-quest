#include "virtualkeyboardmanager.hpp"

template<>
Gui::VirtualKeyboardManager* MyGUI::Singleton<Gui::VirtualKeyboardManager>::msInstance = nullptr;

template<>
const char* MyGUI::Singleton<Gui::VirtualKeyboardManager>::mClassTypeName = "Gui::VirtualKeyboardManager";
