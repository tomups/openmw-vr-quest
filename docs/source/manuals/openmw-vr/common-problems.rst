###############
Common Problems
###############

Menus keep getting stuck inside things
######################################

:Symptoms:
	Opening a menu often leads to the menu being partially or fully hidden inside objects / NPCs.

:Cause:
	Menus are in 3D and appear at fixed offsets from the player, sometimes causing them to appear inside things. Unfortunately I have not implemented
        any anti-collision systems for 3D menus.

:Fix 1:
	Look around for a clear spot and hold Y to recenter menus, this will move them to where you are looking.

:Fix 2:
	Press Y on your motion controller / hit ESC on your keyboard to close the menu.

Game does not work in Virtual Desktop
#####################################

:Symptoms:
	Trying to launch the game wirelessly using Virtual Desktop does not work.

:Cause:
	Running OpenXR apps through Virtual Desktop only supports the SteamVR OpenXR runtime.

:Fix 1:
	Switch OpenXR runtimes to steamvr (google it).

:Fix 2:
	Use Oculus' Air Link

Game does not work in Oculus Air Link
#####################################

:Symptoms:
	Trying to launch the game wirelessly using Air Link does not work.

:Cause:
	Running OpenXR apps through Air Link only supports the Oculus OpenXR runtime.

:Fix 1:
	Switch OpenXR runtimes to Oculus (in the Oculus app).

:Fix 2:
	Use Virtual Desktop

Right eye is not rendering right
################################

:Symptoms:
	Parts of the scene is missing on the right eye.

:Cause:
	Driver bug.

:Fix:
	Either enable "Disable display lists" or uncheck "OVR_MultiView2" in the launcher.

Bad performance
###############

:Symptoms:
	OpenMW-VR plays with a very low framerate.

:Cause:
	OpenMW-VR is based on OpenMW which is not super well optimized (mostly due to morrowind's assets being archaic).
    VR on top of this can be extremely slow when heavy features like shadows and full water reflections are enabled.

:Fix:
	Reduce shadow complexity or disable shadows altogether. Reduce water shader complexity or disable it altogether.
        Make sure "Use shared shadow maps" is enabled in the launcher (launcher->advanced->VR). Make sure "OVR_MultiView2"
        is enabled in the launcher

My Controller doesn't work
##########################

:Symptoms:
	OpenMW-VR appears in my headset but my motion controllers do nothing or have extremely limited functionality.
        Unable to complete even basic gameplay.

:Cause 1:
	No profile exists for that controller. Check openxrinteractionprofiles.xml (same folder as openmw_vr.exe) and see if there is a profile entry for it.
        If there is none, then OpenMW-VR has not added support for your controller yet. Make an issue at https://gitlab.com/madsbuvi/openmw/-/issues or contact me on discord (check the openmw discord) and i will add it posthaste.

:Cause 2:
    You are using an openxr runtime that does not support your controllers. For example, last I checked SteamVR did not
    support the vive cosmos controllers. If this is the cause then a line in openmw.log will state ``Configuring interaction profile '/interaction_profiles/vendor/controller' (Controller Name)``.
    followed by ``Required extension 'XR_#' not supported. Skipping interaction profile.``.

:Fix:
	Make sure your runtime is up to date. Try switching to your headset/controller's native runtimes, as opposed to a
        non-native runtime like SteamVR.

Audio does not go to my VR Headset
##################################

:Symptoms:
	Audio is not automatically playing in your VR headset instead of your default speakers.

:Cause:
	OpenMW explicitly selects an audio device at startup and sticks with it. By default this is the default speaker.

:Fix 1:
    Set your VR headset as system default speakers. Some headsets have an option to do this automatically.

:Fix 2:
    Enable mirroring desktop audio to your headset / vice versa.
    
:Fix 3:
	Configure the openmw audio device. To do this, add the following lines
        to the end of settings.cfg.

        ::

            [Sound]
            device = #

        Where # is the identifier of your device. You can find the correct identifier by reading openmw.log, look for
        the line ``Enumerated output devices:`` where devices are enumerated.

