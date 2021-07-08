Getting Started
###############

Installation
************
Navigate to the Installation Guide on the left for help with installing OpenMW-VR.

Before Launching
****************
Before launching, you should run the openmw-launcher from your openmw-vr dir and go to the advaced->VR tab.
Here you should input your real height in meters. There are some other options here that you don't need to care about,
except maybe the melee combat swing speed if the default doesn't work well for you.

In-game settings
****************
In addition to the launcher settings, there are some in-game settings to be aware of. You should look through these
to make sure everything is to your liking.

:Enable mirror texture:
    If disabled, nothing is shown on your pancake monitor.
:Mirror texture eye:
    Determines which of your eyes is shown on your pancake monitor.
:Flip mirror texture order:
    If showing both eyes, flip their order.
:Haptic Feedback:
    If enabled, controllers will vibrate in response to successful hits, and to being hit.
:Hand Directed Movement:
    If enabled, movement is directed by your left hand instead of your current head orientation.
:Seated Play:
    If enabled, tracking is adjusted for seated play.
:Left Hand Hud Position:
    Presents two options for where to position the left hand hud. Two options exist since the one i originally made
    worked very poorly with shields.

Gameplay
********
TODO: This part would probably be best demonstrated with a *brief* in-game video.
The core of the gameplay is the same as pancake OpenMW. The core difference is that instead of pointing and clicking with
a mouse, you'll be pointing and clicking with motion controllers.

:activators:
    To interact with activators of any kind (doors, npcs, chests, etc), enable pointer mode using right grip/squeeze
    and press the trigger when the pointer beam is on an activator.

:text input:
    Whenever a text input dialogue appears, look at your left hand and write your name using the
    virtual keyboard attached to your left hand.

:realistic melee combat:
    Melee combat in openmw-vr is "realistic", meaning you can swing your sword at your foes to hurt them. Currently
    the only way to disambiguate a dice roll miss from an actual miss, is to see if the enemy's healthbar appeared
    on your left-hand hud. There is a minimum sing speed below which swings are not activated, and a maximum swing speed
    above which no benefit is derived. Swing strength scales from minimum to maximum in this interval. Openmw-vr selects
    between chop, slash, and thrust based on how you moved your weapon.

        .. note:: Although there is no explicit option to disable realistic combat, you can effectively disabled it by
            entering unreasonably high values for swing speeds.

:unrealistic melee combat:
    If you prefer not swinging your weapon, point and click combat is possible by holding your weapon towards the enemy
    and pressing the right trigger. This mode does not receive any animation. Note that you must aim your weapon, not
    your hand, at the enemy.

:lockpicks and probes:
    Lockpicks and propes work like unrealistic melee combat. Aim your tool at the door/chest and press use.

:On-touch spells:
    Simply point your hand at your target and press use.

:Ranged combat:
    Ranged combat is a simple point and click. Aiming works the same for all ranged, including spells. No aim assistance
    has been implemented.
