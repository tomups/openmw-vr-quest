local vr = require('openmw.vr')
if not vr.isVr() then
    return {}
end

local util = require('openmw.util')
local core = require('openmw.core')
local self = require('openmw.self')
local storage = require('openmw.storage')
local I = require('openmw.interfaces')

local saveData = {

}

local l10nKey = 'OMWVRTutorial'
local l10nContext = core.l10n(l10nKey)
local uiSection = storage.playerSection('UiGroup')

local function getBindingDescription(binding)
    local bindings = I.vrinputs.getActiveTriggerBindings(binding)
    for id, path in pairs(bindings or {}) do
        return I.vrinputs.getInteractionName(path)
    end
    bindings = I.vrinputs.getActiveActionBindings(binding)
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

    local variables = {
        dominantHand = hand,
        activateBinding = getBindingDescription('PointerActivate'),
        pointerBinding = getBindingDescription(pointerKey),
        recenterBinding = getRecenterBindingDescription(),
    }

    print('What: '..tostring(pointerKey))
    
    I.vrui.showMessageInTheVoid(l10nContext('Tutorial_MC', variables))
    I.vrui.showMessageInTheVoid(l10nContext('Tutorial_RecenterUiStuckInObject', variables))
    I.vrui.showMessageInTheVoid(l10nContext('Tutorial_Bindings', variables))
    I.vrui.showMessageInTheVoid(l10nContext('Tutorial_PointClickMC', variables))
    I.vrui.showMessageInTheVoid(l10nContext(handTutorialKey, variables))
    
    saveData.hasSeenMCTutorial = true
end

local function KBMTutorialMessages()
    local variables = {
        recenterBinding = getRecenterBindingDescription(),
    }
    I.vrui.showMessageInTheVoid(l10nContext('Tutorial_KBM', variables))
    I.vrui.showMessageInTheVoid(l10nContext('Tutorial_RecenterUiStuckInObject', variables))
    I.vrui.showMessageInTheVoid(l10nContext('Tutorial_PointClickKBM', variables))
    
    saveData.hasSeenKBMTutorial = true
end

local initialized = false

local function tutorials()
    if uiSection:get('ShowTutorials') then
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

local remoteInterface = {
    'overrideLayerConfig',
    'showMessageInTheVoid',
    'setLayerConfig',
    'setLayerPose',
    'setLayerPickable',
}

local interface = {
    version = 0,
}
for _, func in ipairs(remoteInterface) do
    interface[func] = function(...)
        core.sendMenuEvent(self, 'InterfaceCall', {
            arg = {...},
            interface = 'vrui',
            func = func,
        })
    end
end

return {
    interfaceName = 'vrui',
    interface = interface,
    engineHandlers = {
        onFrame = onFrame,
        onLoad = onLoad,
        onSave = onSave,
    },
    
    eventHandlers = {
        UiModeChanged = function(data)
            updateOnce = true
        end,
    },
}
