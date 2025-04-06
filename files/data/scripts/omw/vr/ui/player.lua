local vr = require('openmw.vr')
if not vr.isVr() then
    return {}
end

local util = require('openmw.util')
local core = require('openmw.core')
local I = require('openmw.interfaces')
local common = require('scripts.omw.vr.ui.common')

common.setupDefaults(I.UI.MODE)

local function update()
    common.updatePoses()
    common.updateModes(I.UI.modes)
    common.updateWindowArrangement(I.UI.modes)
end

local wasPaused = false
local initialized = false
local function onVRFrame()
    if not initialized then 
        update()
        initialized = true
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

    engineHandlers = {
        onVRFrame = onVRFrame,
        onVRRecenter = update,
    },
}
