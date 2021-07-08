Controls
########

TODO: It would be useful to include a **brief** gameplay video demonstrating the VR interface here.

Default Bindings
****************

Default bindings exist for the following controllers:
 - Oculus Touch Controllers
 - Index Knuckles
 - Vive Wands
 - Microsoft Motion Controllers
 - Hp Mixed Reality Controllers
 - Huawei Controllers
 - Vive Cosmos Controllers

Oculus Touch controllers default bindings:
    .. image:: ../../../controller_graphics/Oculus_Touch.png
       :width: 600
Valve Index controllers default bindings:
    .. image:: ../../../controller_graphics/Valve_Index.png
       :width: 600

Similar graphics for other controllers will be added if people with those bindings contribute any.
Default controls for other controllers are written to be similar to the touch and index controls.

Rebinding
*********
Currently there is no user friendly rebinding system available.
Some OpenXR runtimes may offer their own rebinding service similar to what SteamVR already offers for OpenVR applications.

If you can't rely on that, you'll have to edit
the bindings manually. To do so find the *xrcontrollersuggestions.xml* file in your openmw root folder **and copy it to
documents/my games/openmw/** or your platform's equivalent, and edit the new copy.
The file itself contains a brief explanation for how to edit the file.

    .. note:: You are kindly requested not to edit the xrcontrollersuggestions.xml file that resides in the installation folder. If you do you will not receive any support until you restore the original by reinstalling.

    .. note:: If your OpenXR runtime sports its own rebinding service, it's possible that changes to xrcontrollersuggestions.xml will have no effect and you'll be forced to use that service.

Actions Sets
************

Actions are divided into two bindable sets: Gameplay, and GUI.

The GUI action set is active whenever you are in a menu and gameplay is paused.
The gameplay action set is active whenever not in a menu. Bindings may therefore overlap between the two.

Gameplay:
 - reposition_menu (aka recenter)
 - meta_menu
 - sneak
 - always_run
 - jump
 - spell (aka ready_spell)
 - weapon (aka ready_weapon)
 - rest
 - inventory
 - activate
 - activate_touched
 - auto_move
 - use
 - move_left_right
 - move_forward_backward
 - look_left_right

GUI:
 - menu_up_down
 - menu_left_right
 - menu_select
 - menu_back
 - use
 - game_menu
 - reposition_menu

Most of these actions are self-explanatory with a few exceptions.

:activate_touched:
    Whenever this control is active, pointer mode is enabled and your finger will point at stuff. Realistic combat is
    disabled in this mode, so avoid bindings that are easy to activate unintentionally.

:use:
    The Use action in VR combines the Active and Use action of pancake OpenMW. When pointer mode is active, Use will
    activate whatever you are pointing at. When pointer mode is not active, Use will use the readied tool/spell in the
    direction you are orienting it.
		
:Recenter:
    The special action Recenter is by default assigned the same button as "Meta Menu". To activate recentering, you must **press and hold** the assigned button.
    The recenter action has differing behaviour depending on whether you are currently in a menu, seated play, or standing play.

        .. note:: In the xrcontrollersuggestions.xml file the recenter action is labeled "reposition_menu".

Recenter (Menus)
****************

If recentering while navigating some menu, all windows are moved to center on your current location/orientation.
This is useful if a menu ended up inside of some geometry.

Recenter (Standing play)
************************

If recentering while in standing play, your view is moved to the location of your character **horizontally**

Recenter (Seated play)
**********************

If recentering while in seated play, your view is moved to the location of your character **horizontally and vertically**.