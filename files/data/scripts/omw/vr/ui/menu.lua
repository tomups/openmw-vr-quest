local vr = require('openmw.vr')
if not vr.isVr() then
    return {}
end
local menu = require('openmw.menu')
local util = require('openmw.util')
local core = require('openmw.core')
local I = require('openmw.interfaces')
local common = require('scripts.omw.vr.ui.common')

common.createDerivedSpaces()

common.registerSettingsPage()
common.registerSettingsGroup()

local initialized = false
local function init()
    common.setupDefaults()
    common.updatePoses()
    common.updateLayers()
    initialized = true
end

local function update()
    common.setupDefaults()
    common.updatePoses()
    common.updateLayers()
    common.updateLayerArrangement()
end

local wasKBMouseMode = false
local wasPaused = false
local updateOnce = true
local function onVRFrame()
    if not initialized then
        init()
    end

    local KBMouseMode = I.vrinputs.isKBMouseMode()
    if KBMouseMode ~= wasKBMouseMode then
        updateOnce = true
        wasKBMouseMode = KBMouseMode
    end

    if updateOnce then
        update()
        updateOnce = false
    end

    -- We only want to update the reference poses when the user enters GUI mode. Otherwise, the windows will be actively tracking
    -- them which is weird, uncomfortable, and impractical.
    local isPaused = core.isWorldPaused()
    if (isPaused and not wasPaused) or common.updateVisibleLayers() then
        update()
    end

    wasPaused = isPaused
end

return {
    engineHandlers = {
        onVRFrame = onVRFrame,
        onVRRecenter = update,
    },
    interfaceName = 'vrui',
    interface = common.interface,
    eventHandlers = {
        InterfaceCall = function(data)
            I[data.interface][data.func](unpack(data.arg or {}))
        end
    }
}
