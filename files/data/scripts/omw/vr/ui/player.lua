local vr = require('openmw.vr')
if not vr.isVr() then
    return {}
end

local util = require('openmw.util')
local core = require('openmw.core')
local I = require('openmw.interfaces')
local common = require('scripts.omw.vr.ui.common')

local function update()
    common.setupDefaults(I.UI.MODE)
    common.updatePoses()
    common.updateModes(I.UI.modes)
    common.updateWindowArrangement(I.UI.modes)
end

local wasPaused = false
local updateOnce = true
local function onVRFrame()
    if updateOnce then 
        update()
        updateOnce = false
    end
    
    -- We only want to update the reference poses when the user enters GUI mode. Otherwise, the windows will be actively tracking
    -- them which is weird, uncomfortable, and impractical.
    local isPaused = core.isWorldPaused()
    if (isPaused and not wasPaused) or common.updateVisibleWindows() then
        update()
    end

    wasPaused = isPaused
end

return {
    interfaceName = 'vrui',
    interface = common.interface,

    engineHandlers = {
        onVRFrame = onVRFrame,
        onVRRecenter = update,
    },
    
    eventHandlers = {
        UiModeChanged = function(data)
            updateOnce = true
        end,
    },
}
