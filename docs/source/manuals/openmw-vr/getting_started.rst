Getting Started
###############

Installation
************
Navigate to the Installation Guide on the left for help with installing OpenMW-VR.

Before Launching
****************
Before launching, you should run the openmw-launcher from your openmw-vr dir and go to the settings->VR tab and explore the options here.

OVR_MultiView2
==============
This feature, if enabled and your GPU supports it, may grant a significant performance boost to openmw-vr. If enabled,
but your GPU does not support it, this setting has no effect. Note that some GPUs have buggy drivers and will have to
disable display lists while using OVR_MultiView2.

Left-Handed Mode
================
As the name implies, enables left handed mode. This is implemented by switching hand trackers and mirroring the meshes.
Default bindings and hand directed movement both account for left handed mode.

In-game settings
****************
In addition to the launcher settings, there are some in-game settings to be aware of. You should look through these
to make sure everything is to your liking.

:Haptic Feedback:
    If enabled, controllers will vibrate in response to successful hits, and to being hit.
:Hand Directed Movement:
    If enabled, movement is directed by your off-hand instead of your current head orientation.
:Show 3D crosshairs:
    If enabled, a 3d line will be drawn to indicate how ranged weapon/spell projectiles will be launched.
:Show mirror texture:
    If disabled, nothing is shown on your computer monitor.
:Mirror texture eye:
    Determines which of your eyes is shown on your computer monitor.
:Flip mirror texture order:
    If showing both eyes, flip their order.
:Hands offset:
    Can be changed to offset hand tracking. Morrowind's assets are not designed for VR and do not have convenient attachment points for trackers.
    I can only give one flat estimate for how to offset the transform, which may vary from model to model. Use this option if you find your hands don't feel properly aligned.
    
Script Settings
***************
Parts of OpenMW-VR has been ported to Lua. Some settings have therefore been moved to the scripts tab of the settings menu.
The scripts tab has two VR entries

OpenMW VR Input settings
========================
Settings related to controls. This is where you can configure Smooth Turn vs Snap Turn. Physical sneak. And input bindings.

OpenMW VR UI settings
=====================
Options for where to place the HUD and Tooltips in space. Note that wrist options will not work in kb+mouse mode and will cause
the HUD/Tooltips to show up in this mode.

Gameplay
********
The core of the gameplay is the same as regular OpenMW. The core difference is that instead of pointing and clicking with
a mouse, you'll be pointing and clicking with motion controllers.

:kb+mouse mode:
    When launching OpenMW-VR, the game starts in kb+mouse mode. Mode automatically switchers to motion controller mode when one or more motion controllers are detected.
    In kb+mouse mode, you are effectively playing the exact same as regular openmw, but with an HMD to control your view. The mouse can still pan your view, but only horizontally.
    Since this mode currently has no snap turning implemented, and panning must be done with the mouse, some users may find it uncomfortable.
    
:motion controller mode:
    In this mode, the game is intended to be fully controlled using the motion controllers. Some keyboard/mouse controls are still available, such as using the keyboard to type
    to text inputs rather than using the virtual keyboard. In this mode you interact with the game world primarily using the pointer (enabled using your dominant hand's grip/squeeze button)
    and the activate button (A for right handed, X for left handed). Melee attacks are done using realistic melee combat by physically swinging your hand, and projectiles are launched in the direction
    you are pointing.

:activators:
    To interact with activators (objects that show a tooltip when pointed at) of any kind (doors, npcs, chests, etc), enable pointer mode using the right grip/squeeze (left grip/squeeze in left handed mode)
    and press the A (X for left handed) when the pointer beam is on an activator.

:realistic melee combat:
    When using motion controllers, melee combat in openmw-vr is "realistic", meaning you can swing your sword at your foes 
    to attack them. Currently the only way to disambiguate a dice roll miss from an actual miss, is to see if the enemy's 
    healthbar appeared on your left-hand hud. There is a minimum sing speed below which swings are not activated, and a maximum 
    swing speed above which no benefit is derived. Swing strength scales from minimum to maximum in this interval. Openmw-vr 
    selects between chop, slash, and thrust based on how you moved your weapon.

        .. note:: Although there is no explicit option to disable realistic combat, you can effectively disabled it by
            entering unreasonably high values for swing speeds. Pointing your weapon towards your enemy and pressing Use will 
            trigger a regular attack, but without any animations. People who prefer not using realistic combat could consider
            using keyboard+mouse mode.

:lockpicks and probes:
    In regular morrowind, lockpick/probes work like an attack. In OpenMW-VR, activate the chest/door with the pointer while having the lockpick/probe in hand instead.

:On-touch spells:
    Simply point your hand at your target and press use.

:Ranged combat:
    Ranged combat is a simple point and click. Aiming works the same for all ranged, including spells. No aim assistance
    has been implemented outside of 3d crosshairs.
