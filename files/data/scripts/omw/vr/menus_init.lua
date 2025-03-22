local menu = require('openmw.menu')
local util = require('openmw.util')
local vr = require('openmw.vr')
local I = require('openmw.interfaces')

if not vr.isVr() then
    return {}
end

I.vrspaces.createDerivedSpace(
    'DefaultWindow', 
    I.vrspaces.referenceSpaces.View, 
    {
        position = util.vector3(0, 1, -0.25) * I.vrspaces.UnitsPerMeter,
        orientation = util.transform.identity,
    }
)

I.vrspaces.createDerivedSpace(
    'DefaultVirtualKeyboard', 
    I.vrspaces.referenceSpaces.View, 
    {
        position = util.vector3(0, 35, -40),
        orientation = util.transform.rotateX(math.pi/8)
    }
)

I.vrspaces.createDerivedSpace(
    'LeftWristInner', 
    I.vrspaces.actionSpaces.LeftHandAim,
    {
        position = util.vector3(0.09, -0.200, -0.033) * I.vrspaces.UnitsPerMeter,
        orientation = util.transform.rotate(math.pi/2, util.vector3(0, 0, 1))
    }
)

I.vrspaces.createDerivedSpace(
    'LeftWristTop', 
    I.vrspaces.actionSpaces.LeftHandAim,
    {
        position = util.vector3(0.0, -0.200, 0.066) * I.vrspaces.UnitsPerMeter
    }
)

I.vrspaces.createDerivedSpace(
    'RightWristInner', 
    I.vrspaces.actionSpaces.LeftHandAim,
    {
        position = util.vector3(-0.09, -0.200, -0.033) * I.vrspaces.UnitsPerMeter,
        orientation = util.transform.rotate(-math.pi/2, util.vector3(0, 0, 1))
    }
)

I.vrspaces.createDerivedSpace(
    'RightWristTop', 
    I.vrspaces.actionSpaces.LeftHandAim,
    {
        position = util.vector3(0.0, -0.200, 0.066) * I.vrspaces.UnitsPerMeter
    }
)

I.vrspaces.createDerivedSpace(
    'HUDTopLeft', 
    I.vrspaces.referenceSpaces.View,
    {
        position = util.vector3(-12, 30, 6)
    }
)

I.vrspaces.createDerivedSpace(
    'HUDTopRight', 
    I.vrspaces.referenceSpaces.View,
    {
        position = util.vector3(12, 30, 6)
    }
)

I.vrspaces.createDerivedSpace(
    'HUDBottomLeft', 
    I.vrspaces.referenceSpaces.View,
    {
        position = util.vector3(-12, 30, -18)
    }
)

I.vrspaces.createDerivedSpace(
    'HUDBottomRight', 
    I.vrspaces.referenceSpaces.View,
    {
        position = util.vector3(12, 30, -18)
    }
)

I.vrspaces.createDerivedSpace(
    'HUDMessage', 
    I.vrspaces.referenceSpaces.View,
    {
        position = util.vector3(0, 30, -3)
    }
)

local function createDefaultConfig(intersectable, backgroundOpacity, autosize)
    return {
        backgroundOpacity = backgroundOpacity,
        center = util.vector2(0,0),
        extent = util.vector2(1,1),
        pixelsPerMeter = 1024,
        pixelResolution = util.vector2(1024, 1024),
        autosize = autosize,
        space = I.vrspaces.customSpaces.defaultMenu,
        intersectable = intersectable,
    }
end

-- For some reason, I.UI is not available to MENU scripts, preventing me from properly managing UI based on MODE 
-- before a game has been loaded.
-- However we know for a fact that the only modes that exist before loading a game are these
I.vrui.setModeConfig("Loading", createDefaultConfig(true, 0.7, true))
I.vrui.setModeConfig("LoadingWallpaper", createDefaultConfig(true, 0.7, true))
I.vrui.setModeConfig("MainMenu", createDefaultConfig(true, 0.7, true))
--local configWindow = createDefaultConfig(true, 0.7, true)
--configWindow.extent = util.vector2(0.818, 0.818)


-- Anything not configured here is configured in the PLAYER script menus.lua

local configVirtualKeyboard = createDefaultConfig(true, 0.7, true)
--configVirtualKeyboard.space = 'DefaultVirtualKeyboard'
configVirtualKeyboard.extent = util.vector2(0.25, 0.25)
I.vrui.setVirtualKeyboardConfig(configVirtualKeyboard)

local configNotification = createDefaultConfig(false, 0, false)
configNotification.space = 'DefaultWindow'
I.vrui.setNotificationConfig(configNotification)

local configHUD = createDefaultConfig(true, 0, true)
configHUD.extent = util.vector2(0.033, 0.033) -- Hud is generally very close to the viewer, and so must have a very small extent
configHUD.center = util.vector2(0, 0.5) -- Offset center so that the HUD stays in-place when it expands with spell effect icons
configHUD.space = 'LeftWristTop'
I.vrui.setHUDConfig(configHUD)

local configTooltip = createDefaultConfig(true, 0, false)
configTooltip.extent = util.vector2(0.33, 0.33)
configTooltip.space = 'RightWristTop'
I.vrui.setTooltipConfig(configTooltip)

local configDefault = createDefaultConfig(true, 0.7, true)
I.vrui.setDefaultWindowConfig(configDefault)

local function init()

    local defaultWindowPose = vr.locateSpaceInWorld('DefaultWindow')
    -- Managing these will need me to set a "default" pose for each mode, rather than a per-window config
    I.vrui.setModePose("Loading", defaultWindowPose)
    I.vrui.setModePose("LoadingWallpaper", defaultWindowPose)
    I.vrui.setModePose("MainMenu", defaultWindowPose)
    local defaultVirtualkeyboardPose = vr.locateSpaceInWorld('DefaultVirtualKeyboard')
    I.vrui.setVirtualKeyboardPose(defaultVirtualkeyboardPose)
    I.vrui.setDefaultWindowPose(defaultWindowPose)
end

local initialized = false
return {
    engineHandlers = {
        onVRFrame = function()
            if not initialized then 
                print('menus_init initialized from onVRFrame')
                init() 
                initialized = true
            end
        end,
        onVRRecenter = function()
            if menu.getState() ~= menu.STATE.Running then
                if not initialized then
                    print('menus_init initialized from onVRRecenter')
                end
                -- re-init to update the menu poses
                init()
                initialized = true
            end
        end
    }
}