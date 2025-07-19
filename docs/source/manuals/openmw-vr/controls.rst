Controls
########

    .. note:: Note all bindings from OpenMW are available to motion controllers. Those actions, such as rest, journal, quick save/load, console, and postprocessor hud, are available through the in-game menu instead.

Bindings Menu
*************

A bindings menu now exists in-game. Open up the settings menu -> scripts -> OpenMW VR Input settings.
Note that due to an upstream rendering bug (https://gitlab.com/OpenMW/openmw/-/issues/8451) this screen does not always render correctly.

Bindings show up as two columns, a left column describing the action, and a right column describing the binding for the currently active motion controllers. 
Click the right column using your "3D Pointer Click" binding to initiate rebinding.

    .. note:: In kb+mouse mode, or if using an unknown/unsupported controller, all bindings will be "none" and no bindings will be possible to make.
    
    .. note:: Upon initiating re-binding the existing binding is cleared and that action becomes unbound. The exception is "activate" binding, which is considered critical and cannot be unbound.
    
    .. note:: The current version does NOT do any sanity checking, so it's possible to make conflicting bindings. If this happens and you become unable to navigate menus because of it, navigate to OpenMW VR Input settings menu in KB+Mouse mode and click reset next to "VR Input Bindings".


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

Default bindings are written to be as similar as possible between devices

Special VR Actions
******************

A few of the bindable actions are unique to VR with motion controllers.

:pointer:
    Default bound to the dominant hand's squeeze/grip button. Whenever this control is active, pointer mode is enabled and your finger will point at stuff. Realistic combat is
    disabled in this mode, so avoid bindings that are easy to activate unintentionally.

:Radial Menu:
    Opens a window with all items bound in the regular quick items menu. Since VR has too few available bindings to make quick items available as hotkeys, this menu is the compromise.
		
:Recenter:
    Recenter is a long press action by default assigned the same button as "Menus". To activate recentering, you must **press and hold** the assigned button.
    The recenter action will not only recenter your camera on your character, it will also move all menus to your current position/orientation. This is useful if a menu is hidden by geometry.

        .. note:: The bindable recenter action does a horizontal recenter only. No vertical recenter.

