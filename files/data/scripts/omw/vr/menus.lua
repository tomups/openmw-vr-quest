local util = require('openmw.util')
local core = require('openmw.core')
local I = require('openmw.interfaces')
local vr = require('openmw.vr')

if not vr.isVr() then
    return {}
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


--local windowToLayer = {
--    Stats = 'StatsWindow',
 --   Inventory = 'InventoryWindow',
   -- Magic = 'SpellWindow',
 --   Map = 'MapWindow',
   -- Container = 'InventoryCompanionWindow',
   -- Trade = 'InventoryCompanionWindow',
   -- Dialogue = 'DialogueWindow',
--}


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

local modeConfig = {}

for _, mode in pairs(I.UI.MODE) do
    modeConfig[mode] = createDefaultConfig(true, 0.7, true)
end

-- Insert any per-mode configs here

for mode, config in pairs(modeConfig) do
    I.vrui.setModeConfig(mode, config)
end

local referenceWorldPose = {
    position = util.vector3(0,0,0),
    orientation = util.transform.identity,
}

local windowRelativePose = {
    position = util.vector3(0, 1, -0.25) * I.vrspaces.UnitsPerMeter,
    orientation = util.transform.identity,
}

local visibleWindowsForArrangement = {}

local function updateWindowArrangementModes()
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
    for _, mode in pairs(I.UI.modes) do
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

local function onFrame(dt)
end

local function visibleWindowsChanged()
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

local function updateModes()
    -- Updates all modes that do not need arrangement
    local transform = util.transform.rotateZ(referenceWorldPose.orientation:getYaw() + windowRelativePose.orientation:getYaw())
    local pose = {
        position = referenceWorldPose.position + transform:apply(windowRelativePose.position),
        orientation = transform
    }
    for _, mode in pairs(I.UI.modes) do
        if not modeWindowsForArrangement[mode] then
            I.vrui.setModePose(mode, pose)
        end
    end
    I.vrui.setDefaultWindowPose(pose)
end

local wasPaused = false
local initialized = false
local function onVRFrame()
    if not initialized then 
        --init()
        initialized = true
    end
    
    local updatePose = (core.isWorldPaused() and not wasPaused) or (visibleWindowsChanged() and #visibleWindowsForArrangement > 0)
    
    if updatePose then
        -- We only want to update the reference poses when the user enters GUI mode. Otherwise, the windows will be actively tracking
        -- them which is weird, uncomfortable, and impractical.
        referenceWorldPose = vr.locateSpaceInWorld(I.vrspaces.referenceSpaces.View) or referenceWorldPose
        windowRelativePose = vr.locateSpace('DefaultWindow', I.vrspaces.referenceSpaces.View) or windowRelativePose
        updateModes()
        updateWindowArrangementModes()
    end

    wasPaused = core.isWorldPaused()
end

return {

    engineHandlers = {
        onFrame = onFrame,
        onVRFrame = onVRFrame,
    },
}
