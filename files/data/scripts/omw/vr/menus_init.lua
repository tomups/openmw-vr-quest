local util = require('openmw.util')
local vr = require('openmw.vr')
local I = require('openmw.interfaces')

if not vr.isVr() then
    return {}
end

local function createDefaultConfig(priority, intersectable, backgroundOpacity, autosize)
    return {
        priority = priority,
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

I.vrspaces.createDerivedSpace(
    'DefaultWindow', 
    I.vrspaces.referenceSpaces.Local, 
    {
        position = util.vector3(0, 1, -0.25) * I.vrspaces.UnitsPerMeter
    }
)

I.vrspaces.createDerivedSpace(
    'DefaultVirtualKeyboard', 
    I.vrspaces.referenceSpaces.Local, 
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

local configLoadingScreen = createDefaultConfig(0, true, 0.7, false)
local configMainMenu = createDefaultConfig(2, true, 0.7, true)
local configVideo = createDefaultConfig(3, true, 0.7, true)
local configSettings = createDefaultConfig(3, true, 0.7, true)
local configJournalBooks = createDefaultConfig(2, true, 0, false)
local configWindows = createDefaultConfig(4, true, 0.7, true)
local configConsole = createDefaultConfig(5, true, 0.7, true)
local configNotification = createDefaultConfig(7, false, 0, false)
local configModal = createDefaultConfig(10, true, 0, true)
local configListBox = createDefaultConfig(11, true, 0.7, true)
local configRadialmenu = createDefaultConfig(8, true, 0, true)
local configVirtualKeyboard = createDefaultConfig(11, true, 0.7, true)
configVirtualKeyboard.space = 'DefaultVirtualKeyboard'
configVirtualKeyboard.extent = util.vector2(0.25, 0.25)
local configHUD = createDefaultConfig(0, true, 0, true)
configHUD.extent = util.vector2(0.033, 0.033) -- Hud is generally very close to the viewer, and so must have a very small extent
configHUD.center = util.vector2(0, 0.5) -- Offset center so that the HUD stays in-place when it expands with spell effect icons
configHUD.space = 'LeftWristTop'
local configTooltip = createDefaultConfig(0, true, 0, false)
configTooltip.extent = util.vector2(0.33, 0.33)

-- These windows will be shown side-by-side in order of their priority
local configWindow = createDefaultConfig(0, true, 0.7, true)
--local configStatsWindow = createDefaultConfig(0, true, 0.7, true)
--local configInventoryWindow = createDefaultConfig(1, true, 0.7, true)
--local configSpellWindow = createDefaultConfig(2, true, 0.7, true)
--local configMapWindow = createDefaultConfig(3, true, 0.7, true)
--local configInventoryCompanionWindow = createDefaultConfig(4, true, 0.7, true)
--local configDialogueWindow = createDefaultConfig(5, true, 0.7, true)
configWindow.space = 'DefaultWindow'
configWindow.extent = util.vector2(0.818, 0.818)


local configs = {
    LoadingScreen = configLoadingScreen,
    Notification = configNotification,
    HUD = configHUD,
    Tooltip = configTooltip,
    JournalBooks = configJournalBooks,
    Modal = configModal,
    RadialMenu = configRadialMenu,
    Windows = configWindows,
    Video = configVideo,
    InputBlocker = configVideo,
    MainMenu = configMainMenu,
    Settings = configSettings,
    Console = configConsole,
    VirtualKeyboard = configVirtualKeyboard,
    ListBox = configListBox,
    StatsWindow = configWindow,
    InventoryWindow = configWindow,
    SpellWindow = configWindow,
    MapWindow = configWindow,
    InventoryCompanionWindow = configWindow,
    DialogueWindow = configWindow,
}

for layer, config in pairs(configs) do
    I.vrui.setLayerConfig(layer, config)
end
