local util = require('openmw.util')
local core = require('openmw.core')
local vr = require('openmw.vr')
local I = require('openmw.interfaces')
local function createDerivedSpaces()

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
            orientation = util.transform.identity,
            --orientation = util.transform.rotateX(math.pi/8)
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
end

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

-- List of windows that need to be arranged when multiple windows are opened    
-- In order of priority (left-to-right)
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

local modeWindowsForArrangement = {
    Interface = { "Stats", "Inventory", "Map", "Magic" },
    Container = { "Inventory", "Container" },
    Companion = { "Inventory", "Companion" },
    Barter = { "Inventory", "Trade" },
    Dialogue = { "Dialogue" },
}

local modeConfig = {}

local function setupDefaults(modes)
    for _, mode in pairs(modes) do
        modeConfig[mode] = createDefaultConfig(true, 0.7, true)
    end

    -- Insert any per-mode configs here

    for mode, config in pairs(modeConfig) do
        I.vrui.setModeConfig(mode, config)
    end

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
end

local function makeUpright(orientation)
    return util.transform.rotateZ(orientation:getYaw())
end

local referenceWorldPose = {
    position = util.vector3(0,0,0),
    orientation = util.transform.identity,
}

local windowRelativePose = {
    position = util.vector3(0, 1, -0.25) * I.vrspaces.UnitsPerMeter,
    orientation = util.transform.identity,
}

local virtualKeyboardRelativePose = {
    position = util.vector3(0, 35, -40),
    orientation = util.transform.rotateX(math.pi/8),
}

local visibleWindowsForArrangement = {}

local function updateWindowArrangement(modes)
    -- Updates all modes that need arrangement
    if #visibleWindowsForArrangement == 0 then 
        return 
    end
    
    local windowPoses = {}
    local step = (-math.pi / 4)
    local span = step * (#visibleWindowsForArrangement - 1)
    local angle = -span / 2 + referenceWorldPose.orientation:getYaw()
    
    for _, window in ipairs(visibleWindowsForArrangement) do
        -- Arrange window at this angle
        
        -- mRotation = osg::Quat{ angle, osg::Z_AXIS };
        local transform = util.transform.rotateZ(angle) * util.transform.rotateZ(windowRelativePose.orientation:getYaw())
        
        local rotatedPosition = transform:apply(windowRelativePose.position)
        
        windowPoses[window] = {
            position = referenceWorldPose.position + rotatedPosition,
            orientation = transform
        }
        
        angle = angle + step
    end
    
    for mode, windows in pairs (modeWindowsForArrangement) do
    end
    for _, mode in pairs(modes) do
        local windows = modeWindowsForArrangement[mode]
        if windows then
            for _, window in pairs(windows) do
                if not windowPoses[window] then
                    print('critical: '..window..' somehow got skipped, its visibility is: '..tostring(I.UI.isWindowVisible(window)))
                else
                    print('Setting mode pose for '..mode..' to '..tostring(windowPoses[window].position))
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

local function updateModes(modes)
    -- Updates all modes that do not need arrangement
    local transform = util.transform.rotateZ(referenceWorldPose.orientation:getYaw() + windowRelativePose.orientation:getYaw())
    local pose = {
        position = referenceWorldPose.position + transform:apply(windowRelativePose.position),
        orientation = transform
    }
    for _, mode in pairs(modes) do
        if not modeWindowsForArrangement[mode] then
            I.vrui.setModePose(mode, pose)
        end
    end
    I.vrui.setDefaultWindowPose(pose)
    
    -- Handle virtual keyboard
    orientation = util.transform.rotateX(math.pi/8)
    transform = util.transform.rotateZ(referenceWorldPose.orientation:getYaw())
    local pose = {
        position = referenceWorldPose.position + transform:apply(virtualKeyboardRelativePose.position),
        orientation = transform * virtualKeyboardRelativePose.orientation
    }
    I.vrui.setVirtualKeyboardPose(pose)
end

local function updatePoses()
    referenceWorldPose = vr.locateSpaceInWorld(I.vrspaces.referenceSpaces.View) or referenceWorldPose
    windowRelativePose = vr.locateSpace('DefaultWindow', I.vrspaces.referenceSpaces.View) or windowRelativePose
end

return {
    createDefaultConfig = createDefaultConfig,
    createDerivedSpaces = createDerivedSpaces,
    setupDefaults = setupDefaults,
    makeUpright = makeUpright,
    updatePoses = updatePoses,
    updateModes = updateModes,
    updateVisibleWindows = updateVisibleWindows,
    updateWindowArrangement = updateWindowArrangement,
}