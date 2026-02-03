---
-- vr ui
-- @module vrui
--

--- Configuration of a VR 3D GUI quad. Informs the engine where to place a layer of the GUI within the game world, and at what size.
-- @type GuiConfig
-- @field #number backgroundOpacity Default: 0
-- @field openmw.util#Vector2 center Defines where within a layer is considered the center. x and y values range from -0.5 to 0.5. The geometry will be positioned with its center at the assigned coordinate. This most importantly affects the direction auto-sized layers will grow. For example, the HUD has a default center of util.vector2(0, 0.5),  causing it to grow left as new magic effects are added, keeping all elements in place.
-- @field openmw.util#Vector2 extent Defines the spatial size of a layer in meters. Is ignored for auto-sized layers.
-- @field #number pixelsPerMeter Defines the spatial size of auto-sized layers. Is ignored for non-auto-sized layers.
-- @field #boolean autosize Defines whether this layer should be auto-sized or not
-- @field #boolean intersectable Defines whether or not this layer should be intersectable for VR pointers. If this is false, the pointer will fall through this layer.
-- @field #string space (Optional) name of a space to actively track. If set, the UI element will actively track this space, updating its pose every frame.
--

--- A spatial pose, consisting of a position and an orientation
-- @type Pose
-- @field openmw.util#Vector3 position @{openmw.util#Vector3}
-- @field openmw.util#Transform orientation @{openmw.util#Transform}
--

-- For some reason, i cannot inline documentation statements with the corresponding function. The documentation build parents everything to table if i do...
-- This doesn't happen for any of the other interfaces...

--- Interface version
-- @field [parent=#vrui] #number version

--- Set config for all hidden layers/windows that aren't exposed to lua yet
-- @function [parent=#vrui] setDefaultWindowConfig
-- @param #GuiConfig config

--- Set world pose for all hidden layers/windows that aren't exposed to lua yet
-- @function [parent=#vrui] setDefaultWindowPose
-- @param #Pose pose

--- Set config for all windows/layers of the given mode.
-- @function [parent=#vrui] setModeConfig
-- @param #string mode see ${UI.MODE}
-- @param #GuiConfig config

--- Set pose for the given window of the given mode.
-- @function [parent=#vrui] setModePose
-- @param #string mode see ${UI.MODE}
-- @param #Pose pose
-- @param #string window see ${UI.WINDOW}

--- Set config for notifications
-- @function [parent=#vrui] setNotificationConfig
-- @param #GuiConfig config

--- Set pose for notifications
-- @function [parent=#vrui] setNotificationPose
-- @param #Pose pose

--- Set Config for the HUD
-- @function [parent=#vrui] setHUDConfig
-- @param #GuiConfig config

--- Set pose for the HUD
-- @function [parent=#vrui] setHUDPose
-- @param #Pose pose

--- Set Config for Tooltips
-- @function [parent=#vrui] setTooltipConfig
-- @param #GuiConfig config

--- Set pose for Tooltips
-- @function [parent=#vrui] setTooltipPose
-- @param #Pose pose

--- Set Config for the virtual keyboard
-- @function [parent=#vrui] setVirtualKeyboardConfig
-- @param #GuiConfig config

--- Set pose for the virtual keyboard
-- @function [parent=#vrui] setVirtualKeyboardPose
-- @param #Pose pose

--- Override configs for a given mode. Stops this script from processing that mode, allowing you to change it however you wish without this script interfering.
-- @function [parent=#vrui] overrideModeConfig
-- @param #string mode ${UI.MODE}
-- @param #boolean override Set to true to override config. Set to false to stop overriding.

local async = require('openmw.async')
local core = require('openmw.core')
local I = require('openmw.interfaces')
local storage = require('openmw.storage')
local util = require('openmw.util')
local vr = require('openmw.vr')

local function createDerivedSpaces()
    I.vrspaces.createDerivedSpace(
        'DefaultWindow',
        I.vrspaces.referenceSpaces.View,
        {
            position = util.vector3(0, 1, -0.25) * I.vrspaces.unitsPerMeter,
            orientation = util.transform.identity,
        }
    )
    I.vrspaces.createDerivedSpace(
        'DialogueWindow',
        I.vrspaces.referenceSpaces.View,
        {
            position = util.vector3(0, 1, -0.45) * I.vrspaces.unitsPerMeter,
            orientation = util.transform.identity,
        }
    )

    I.vrspaces.createDerivedSpace(
        'DefaultNotification',
        I.vrspaces.referenceSpaces.View,
        {
            position = util.vector3(0, 0.8, -0.25) * I.vrspaces.unitsPerMeter,
            orientation = util.transform.identity,
        }
    )

    I.vrspaces.createDerivedSpace(
        'DefaultVirtualKeyboard',
        I.vrspaces.referenceSpaces.View,
        {
            position = util.vector3(0, 35, -40),
            orientation = util.transform.identity,
        }
    )

    I.vrspaces.createDerivedSpace(
        'LeftWristInner',
        I.vrspaces.actionSpaces.LeftHandAim,
        {
            position = util.vector3(0.09, -0.200, -0.033) * I.vrspaces.unitsPerMeter,
            orientation = util.transform.rotate(math.pi/2, util.vector3(0, 0, 1))
        }
    )

    I.vrspaces.createDerivedSpace(
        'LeftWristTop',
        I.vrspaces.actionSpaces.LeftHandAim,
        {
            position = util.vector3(0.0, -0.200, 0.066) * I.vrspaces.unitsPerMeter
        }
    )

    I.vrspaces.createDerivedSpace(
        'RightWristInner',
        I.vrspaces.actionSpaces.RightHandAim,
        {
            position = util.vector3(-0.09, -0.200, -0.033) * I.vrspaces.unitsPerMeter,
            orientation = util.transform.rotate(-math.pi/2, util.vector3(0, 0, 1))
        }
    )

    I.vrspaces.createDerivedSpace(
        'RightWristTop',
        I.vrspaces.actionSpaces.RightHandAim,
        {
            position = util.vector3(0.0, -0.200, 0.066) * I.vrspaces.unitsPerMeter
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
end

local HUDpaces = {
    'LeftWristInner',
    'LeftWristTop',
    'RightWristInner',
    'RightWristTop',
    'HUDTopLeft',
    'HUDTopRight',
    'HUDBottomLeft',
    'HUDBottomRight',
}

local function createDefaultConfig(intersectable, backgroundOpacity, autosize)
    return {
        backgroundOpacity = backgroundOpacity,
        center = util.vector2(0,0),
        extent = util.vector2(1,1),
        pixelsPerMeter = 1024,
        pixelResolution = util.vector2(1024, 1024),
        autosize = autosize,
        intersectable = intersectable,
    }
end

--- List of windows that need to be arranged when multiple windows are opened
--- In order of priority (left-to-right)
local windowsForArrangement = {
    'Stats',
    'Inventory',
    'Magic',
    'Map',
    'Companion',
    'Container',
    'Trade',
    'Dialogue',
}

local modeConfigOverridden = {}

local modeWindowsForArrangement = {
    Interface = { "Stats", "Inventory", "Map", "Magic" },
    Container = { "Inventory", "Container" },
    Companion = { "Inventory", "Companion" },
    Barter = { "Inventory", "Trade" },
    Dialogue = { "Dialogue" },
}

local spaceForMode = {
    Default = 'DefaultWindow',
    Dialogue = 'DialogueWindow',
}

local modeConfig = {}

local settingsPageKey = 'OMWVRUI'
local l10nKey = settingsPageKey
local uiGroupKey = 'UiGroup'
local uiSection = storage.playerSection(uiGroupKey)
local spacesGroupKey = 'SpacesGroup'
local spacesSection = storage.playerSection(spacesGroupKey)

local function registerSettingsPage()
    I.Settings.registerPage({
        key = settingsPageKey,
        l10n = l10nKey,
        name = 'UiPage',
        description = 'UiPageDescription',
    })
end

local function boolSetting(key, default, argument)
    return {
        key = key,
        renderer = 'checkbox',
        name = key,
        description = key .. 'Description',
        default = default,
        argument = argument,
    }
end

local function floatSetting(key, default, argument)
    return {
        key = key,
        renderer = 'number',
        name = key,
        description = key .. 'Description',
        default = default,
        argument = argument,
    }
end

local function selectSetting(key, items, default)
    if not default then default = items[1] end
    return {
        key = key,
        renderer = 'select',
        name = key,
        description = key .. 'Description',
        default = default,
        argument = {
            l10n = l10nKey,
            items = items
        }
    }
end

local function registerSettingsGroup()
    I.Settings.registerGroup({
        key = spacesGroupKey,
        page = settingsPageKey,
        l10n = l10nKey,
        name = spacesGroupKey,
        permanentStorage = true,
        settings = {
            selectSetting('HUDSpace', HUDpaces),
            selectSetting('TooltipSpace', HUDpaces),
        },
    })
    I.Settings.registerGroup({
        key = uiGroupKey,
        page = settingsPageKey,
        l10n = l10nKey,
        name = uiGroupKey,
        permanentStorage = true,
        settings = {
            boolSetting('ShowTutorials', true),
        },
    })
end

local configHUD = createDefaultConfig(true, 0, true)
configHUD.extent = util.vector2(0.033, 0.033)
configHUD.center = util.vector2(0, 0.5)
configHUD.space = 'LeftWristTop'

local configTooltip = createDefaultConfig(true, 0, true)
configTooltip.extent = util.vector2(0.033, 0.033)
configTooltip.space = 'RightWristTop'

local function updateSpacesSettings()
    configHUD.space = spacesSection:get('HUDSpace')
    I.vrui.setHUDConfig(configHUD)

    configTooltip.space = spacesSection:get('TooltipSpace')
    I.vrui.setTooltipConfig(configTooltip)
end
spacesSection:subscribe(async:callback(updateSpacesSettings))


local function setupDefaults(modes)
    updateSpacesSettings()
    for _, mode in pairs(modes or {}) do
        modeConfig[mode] = createDefaultConfig(true, 0.7, true)
    end

    if modeConfig.Book then
        modeConfig.Book.backgroundOpacity = 0
    end
    if modeConfig.Journal then
        modeConfig.Journal.backgroundOpacity = 0
    end
    if modeConfig.Scroll then
        modeConfig.Scroll.backgroundOpacity = 0
    end

    for mode, config in pairs(modeConfig) do
        if not modeConfigOverridden[mode] then
            I.vrui.setModeConfig(mode, config)
        end
    end

    local configVirtualKeyboard = createDefaultConfig(true, 0.7, true)
    configVirtualKeyboard.extent = util.vector2(0.25, 0.25)
    I.vrui.setVirtualKeyboardConfig(configVirtualKeyboard)

    local configNotification = createDefaultConfig(false, 0, false)
    configNotification.space = 'DefaultNotification'
    I.vrui.setNotificationConfig(configNotification)

    local configDefault = createDefaultConfig(true, 0.7, true)
    I.vrui.setDefaultWindowConfig(configDefault)
end

local function makeUpright(orientation)
    return util.transform.rotateZ(orientation:getYaw())
end

local referenceWorldPose = {
    position = util.vector3(0,0,0),
    orientation = util.transform.identity,
}

local windowRelativePose = {}
local function getWindowRelativePose(mode)
    return windowRelativePose[mode] or windowRelativePose['Default']
end
local function updateWindowRelativePoses()
    for mode, space in pairs(spaceForMode) do
        windowRelativePose[mode] = I.vrspaces.locateSpace(space, I.vrspaces.referenceSpaces.View) or windowRelativePose[mode]
    end
end

local virtualKeyboardRelativePose = {
    position = util.vector3(0, 35, -40),
    orientation = util.transform.rotateX(math.pi/8),
}

local visibleWindowsForArrangement = {}

local function updateWindowArrangement(modes)
    local mode = I.UI.getMode()
    if #visibleWindowsForArrangement == 0 or not mode then
        return
    end

    local windowPoses = {}
    local step = (-math.pi / 4)
    local span = step * (#visibleWindowsForArrangement - 1)
    local angle = -span / 2 + referenceWorldPose.orientation:getYaw()
    for _, window in ipairs(visibleWindowsForArrangement) do
        local transform = util.transform.rotateZ(angle) * util.transform.rotateZ(getWindowRelativePose(mode).orientation:getYaw())
        local rotatedPosition = transform:apply(getWindowRelativePose(mode).position)

        windowPoses[window] = {
            position = referenceWorldPose.position + rotatedPosition,
            orientation = transform
        }

        angle = angle + step
    end

    for _, mode in pairs(modes) do
        local windows = modeWindowsForArrangement[mode]
        if windows and not modeConfigOverridden[mode] then
            for _, window in pairs(windows) do
                if windowPoses[window] then
                    I.vrui.setModePose(mode, windowPoses[window], window)
                end
            end
        end
    end
end

local function updateVisibleWindows()
    local old = visibleWindowsForArrangement
    visibleWindowsForArrangement = {}

    for _, window in ipairs(windowsForArrangement) do
        if I.UI.isWindowVisible(window) then
            visibleWindowsForArrangement[#visibleWindowsForArrangement+1] = window
        end
    end

    if #old ~= #visibleWindowsForArrangement then
        return true
    end
    for i, window in ipairs(old) do
        if visibleWindowsForArrangement[i] ~= window then
            return true
        end
    end
    return false
end

local function computeModePose(mode)
    local transform = util.transform.rotateZ(referenceWorldPose.orientation:getYaw() + getWindowRelativePose(mode).orientation:getYaw())
    return {
        position = referenceWorldPose.position + transform:apply(getWindowRelativePose(mode).position),
        orientation = transform
    }
end

local function computeVirtualKeyboardPose()
    local orientation = util.transform.rotateX(math.pi/8)
    local transform = util.transform.rotateZ(referenceWorldPose.orientation:getYaw())
    return {
        position = referenceWorldPose.position + transform:apply(virtualKeyboardRelativePose.position),
        orientation = transform * virtualKeyboardRelativePose.orientation
    }
end

local function updateModes(modes)
    -- Update all the modes that did not get updated by arrangement.
    for _, mode in pairs(modes) do
        if not modeConfigOverridden[mode] then
            I.vrui.setModePose(mode, computeModePose(mode))
        end
    end
    I.vrui.setDefaultWindowPose(computeModePose(nil))
    I.vrui.setVirtualKeyboardPose(computeVirtualKeyboardPose())
end

local function updatePoses()
    referenceWorldPose = I.vrspaces.locateSpaceInWorld(I.vrspaces.referenceSpaces.View) or referenceWorldPose
    updateWindowRelativePoses()
end

local function overrideModeConfig(mode, v)
    modeConfigOverridden[mode] = v
end

--- We need to manage some layers that haven't been exposed to Lua yet:
--- These are MyGUI layers that i don't want to expose directly to Lua, but i still want them to be manageable from Lua.
--- So i made .set*Config methods in I.vrui for these
--- And one I.vrui.setDefaultWindowConfig which sets a default config for all remaining layers that haven't been exposed to Lua
local secretItems = {
    'Modal',
    'RadialMenu',
    'Windows',
    'Video',
    'InputBlocker',
    'Settings',
    'Console',
    'ListBox',
    'MainMenu',
    'MainMenuBackground',
    'LoadingScreen',
    'LoadingScreenBackground',
    'Debug',
}

local interface =
{
    version = 0,

    setDefaultWindowConfig = function(config)
        for _, item in pairs(secretItems) do
            vr._setLayerConfig(item, config)
        end
    end,

    setDefaultWindowPose = function(pose)
        for _, item in pairs(secretItems) do
            vr._setLayerPose(item, pose)
        end
    end,

    setModeConfig = function(mode, options)
        vr._setModeConfig(mode, options)
    end,

    setModePose = function(mode, pose, window) vr._setModePose(mode, pose, window) end,

    setNotificationConfig = function(config) vr._setLayerConfig('Notification', config) end,

    setNotificationPose = function(pose) vr._setLayerPose('Notification', pose) end,

    setHUDConfig = function(config) vr._setLayerConfig('HUD', config) end,

    setHUDPose = function(pose) vr._setLayerPose('HUD', pose) end,

    setTooltipConfig = function(config) vr._setLayerConfig('Tooltip', config) end,

    setTooltipPose = function(pose) vr._setLayerPose('Tooltip', pose) end,

    setVirtualKeyboardConfig = function(config) vr._setLayerConfig('VirtualKeyboard', config) end,

    setVirtualKeyboardPose = function(pose) vr._setLayerPose('VirtualKeyboard', pose) end,

    overrideModeConfig = overrideModeConfig,
    
    showMessageInTheVoid = function(message) vr._showMessageInTheVoid(message) end,
}

return {
    createDefaultConfig = createDefaultConfig,
    createDerivedSpaces = createDerivedSpaces,
    setupDefaults = setupDefaults,
    makeUpright = makeUpright,
    updatePoses = updatePoses,
    updateModes = updateModes,
    updateVisibleWindows = updateVisibleWindows,
    updateWindowArrangement = updateWindowArrangement,
    registerSettingsPage = registerSettingsPage,
    registerSettingsGroup = registerSettingsGroup,
    interface = interface,
    uiSection = uiSection,
    spacesSection = spacesSection,
}
