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

local l10nKey = 'OMWVRTutorial'
local l10nContext = core.l10n(l10nKey)

local function getBindingDescription(binding)
    local bindings = I.vrinputs.getActiveTriggerBindings(binding)
    for id, path in pairs(bindings or {}) do
        return I.vrinputs.getInteractionName(path)
    end
    return '('..l10nContext('NotBound')..')'
end

local function getRecenterBindingDescription()
    if I.vrinputs.isKBMouseMode() then
        return l10nContext('MenuBindingDefaultEsc')
    end
    
    return getBindingDescription('Recenter')
end

local function MCTutorialMessages()
    local handKey = 'RightHand'
    local pointerKey = 'PointerRight'
    local handTutorialKey = 'Tutorial_RightHandedMode'
    
    if vr.isLeftHandedMode() then
        handKey = 'LeftHand'
        pointerKey = 'PointerLeft'
        handTutorialKey = 'Tutorial_LeftHandedMode'
    end
    local hand = l10nContext(handKey)
    local pointer = l10nContext(pointerKey)
    
    I.vrui.showMessageInTheVoid(string.format(l10nContext('Tutorial_MC'), hand, pointer))
    I.vrui.showMessageInTheVoid(string.format(l10nContext('Tutorial_RecenterUiStuckInObject'), getRecenterBindingDescription()))
    I.vrui.showMessageInTheVoid(l10nContext('Tutorial_Bindings'))
    I.vrui.showMessageInTheVoid(string.format(l10nContext('Tutorial_PointClickMC'), pointer, getBindingDescription('PointerActivate')))
    I.vrui.showMessageInTheVoid(l10nContext(handTutorialKey))
    
    saveData.hasSeenMCTutorial = true
end

local function KBMTutorialMessages()
    
    I.vrui.showMessageInTheVoid(l10nContext('Tutorial_KBM'))
    I.vrui.showMessageInTheVoid(string.format(l10nContext('Tutorial_RecenterUiStuckInObject'), getRecenterBindingDescription()))
    I.vrui.showMessageInTheVoid(l10nContext('Tutorial_PointClickKBM'))
    
    saveData.hasSeenKBMTutorial = true
end

local initialized = false

local function tutorials()
    if common.uiSection:get('ShowTutorials') then
        if not I.vrinputs.isKBMouseMode() and not saveData.hasSeenMCTutorial then
            MCTutorialMessages()
        end
        if I.vrinputs.isKBMouseMode() and not saveData.hasSeenKBMTutorial then
            KBMTutorialMessages()
        end
    end
end

local function init()
    I.vrspaces.recenterXY() 
    I.vrspaces.recenterZ() 
    tutorials()

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

local function onLoad(data, initData)
    saveData = data or {}
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
