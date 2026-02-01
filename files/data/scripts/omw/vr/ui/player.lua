local vr = require('openmw.vr')
if not vr.isVr() then
    return {}
end

local util = require('openmw.util')
local core = require('openmw.core')
local I = require('openmw.interfaces')
local common = require('scripts.omw.vr.ui.common')

local saveData = {

}

local function getRecenterBindingDescription()

    if I.vrinputs.isKBMouseMode() then
        return "Menu binding (default Esc)"
    end
    local bindings = I.vrinputs.getActiveTriggerBindings('Recenter')
    for id, path in pairs(bindings or {}) do
        return I.vrinputs.getInteractionName(path)
    end
    return "(Recenter not bound on any active controller)"
end

local function tutorialMessage()
    local bindingDescription = getRecenterBindingDescription()
    local message = 'In VR, the UI is 3D and may clip into objects and NPCs. When this happens, look away from the object/NPC and perform a recenter by long pressing '..bindingDescription..' to move the interface'
    I.UI.showInteractiveMessage(message, {})
    saveData.hasSeenTutorial = true
end

local initialized = false

local function init()
    I.vrspaces.recenterXY() 
    I.vrspaces.recenterZ() 
    if not saveData.hasSeenTutorial then
        tutorialMessage()
    end

    initialized = true
end

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

local function onFrame()
    if not initialized then
        init()
    end
end

local function onLoad(saveData, initData)
    saveData = saveData or {}
end

local function onSave()
    return saveData
end

return {
    interfaceName = 'vrui',
    interface = common.interface,

    engineHandlers = {
        onFrame = onFrame,
        onVRFrame = onVRFrame,
        onVRRecenter = update,
        onLoad = onLoad,
        onSave = onSave,
    },
    
    eventHandlers = {
        UiModeChanged = function(data)
            updateOnce = true
        end,
    },
}
