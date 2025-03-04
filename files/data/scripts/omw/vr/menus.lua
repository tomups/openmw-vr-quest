local util = require('openmw.util')
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
    'Trade',
    'Dialogue',
}

local windowToLayer = {
    Stats = 'StatsWindow',
    Inventory = 'InventoryWindow',
    Magic = 'SpellWindow',
    Map = 'MapWindow',
    Trade = 'InventoryCompanionWindow',
    Dialogue = 'DialogueWindow',
}

local function arrangeWindowAtAngle(layer, angle)

    -- print(tostring(layer)..'['..tostring(angle)..']')
    local pose = vr.locateSpace(I.vrspaces.customSpaces.defaultMenu, I.vrspaces.customSpaces.referenceLocal)
    
    -- mRotation = osg::Quat{ angle, osg::Z_AXIS };
    local transform = util.transform.rotateZ(angle)
    
    local rotatedPosition = transform:apply(pose.position)
    
    I.vrui.setLayerPose(layer, {
        position = rotatedPosition - pose.position,
        orientation = transform
    })
end

local function updateWindowArrangement()
    local visibleWindows = {}
    for _, window in ipairs(windowsForArrangement) do
        if I.UI.isWindowVisible(window) then
            visibleWindows[#visibleWindows+1] = window
        end
    end
    
    if #visibleWindows == 0 then 
        return 
    end
    
    local step = (-math.pi / 4)
    local span = step * (#visibleWindows - 1)
    
    local angle = -span / 2
    for _, window in ipairs(windowsForArrangement) do
        local layer = windowToLayer[window]
        arrangeWindowAtAngle(layer, angle)
        angle = angle + step
    end
end

local function onFrame(dt)
end

local function onVRFrame()
    if I.UI.getMode() then
        updateWindowArrangement()
    end
end


print('MODEs:')
for k, v in pairs(I.UI.MODE) do
    print(tostring(k)..': '..tostring(v))    
    for _, w in pairs(I.UI.getWindowsForMode(v)) do
        print(tostring(k)..'  -> '..tostring(w))
    end
end

print('WINDOWs:')
for k, v in pairs(I.UI.WINDOW) do
    print(tostring(k)..': '..tostring(v))
end

return {

    engineHandlers = {
        onFrame = onFrame,
        onVRFrame = onVRFrame,
    },
}
