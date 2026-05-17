local vr = require('openmw.vr')

if not vr.isVr() then
    return {}
end

local ui = require('openmw.ui')
local util = require('openmw.util')
local I = require('openmw.interfaces')
local storage = require('openmw.storage')
local input = require('openmw.input')
local menu = require('openmw.menu')
local core = require('openmw.core')
local async = require('openmw.async')
local common = require('scripts.omw.vr.inputs.common')
local initialized = false

local function init()
    common.registerSettingsGroup()
    common.registerSettingsPage()

    common.setOnInputChangedBoolean(function(path, _, value)
        if not value then
            I._vrinputssettingsrenderer.bindInteraction(path, value)
        end
    end)

    -- Current .RC does not process actions during the main menu.
    -- So until a game has been loaded we have to manually deal with menu bindings.
    common.registerTriggerHandlerForMainMenu('PointerActivate', async:callback(function()
        vr._pointerActivate(true)
    end))

    common.registerTriggerHandlerForMainMenu('MenuBack', async:callback(function()
        -- There isn't a catch-all solution for closing the current menu item from lua.
        -- I.UI.removeMode can only remove modes, not dialogue boxes, the console, or the postprocessing hud.
        -- From VR I need a one click solution, so i am using this placeholder internal function.
        ui._menuBack()
    end))

    common.registerTriggerHandlerForMainMenu('Recenter', async:callback(function()
        print('Recenter')
        I.vrspaces.recenterXY()
    end))
end

-- OpenMW bug: https://gitlab.com/OpenMW/openmw/-/work_items/9115
local menuTriggersHeartBeat = true
input.registerTrigger {
    key = 'MenuTriggerHandlersHeartBeat',
    l10n = 'dummy',
    name = 'MenuTriggerHandlersHeartBeat',
    description = 'MenuTriggerHandlersHeartBeat is used to detect when the engine wipes all menu trigger handlers, leading to the need to re-register them.',
}
local menuTriggersHeartBeatHandler = async:callback(function()
    menuTriggersHeartBeat = true
end)
local menuTriggersHeartBeatListeners = {}

local function registerTriggerHandlers()
    input.registerTriggerHandler('MenuTriggerHandlersHeartBeat', menuTriggersHeartBeatHandler)
end
registerTriggerHandlers()

local function addMenuTriggersHeartBeatListener(listener)
    menuTriggersHeartBeatListeners[#menuTriggersHeartBeatListeners+1] = listener
end

local gameLoaded = false

local function onFrame(dt)
    if not initialized then
        init()
        initialized = true
    end

    -- Only want to process inputs/actions in this script during pre-game.
    if menu.getState() == menu.STATE.NoGame then
        common.onFrame()
    end

    if not menuTriggersHeartBeat then
        print('MENU script trigger handlers wiped, re-registering')
        for _, handler in ipairs(menuTriggersHeartBeatListeners) do
            handler()
        end
        registerTriggerHandlers()
    end
    menuTriggersHeartBeat = false
    input.activateTrigger('MenuTriggerHandlersHeartBeat')
end
return {
    engineHandlers = {
        onFrame = onFrame,
    },

    interfaceName = 'vrinputs',
    ---
    -- vr inputs
    -- @module vrinputs

    interface = {
        --- Interface version
        -- @field [parent=#vrinputs] #number version
        version = 0,
        controllers = common.controllers,
        getInteractionProfileOfController = common.getInteractionProfileOfController,
        getInputValue = vr._getInputValue,
        setOutputValue = vr._setOutputValue,
        isKBMouseMode = common.isKBMouseMode,
        getActiveActionBindings = common.getActiveActionBindings,
        getActiveTriggerBindings = common.getActiveTriggerBindings,
        getInteractionName = common.getInteractionName,
        -- Workaround for OpenMW not processing actions/triggers during main menu.
        addMenuTriggersHeartBeatListener = addMenuTriggersHeartBeatListener
    }
}
