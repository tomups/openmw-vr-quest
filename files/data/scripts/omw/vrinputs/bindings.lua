local vr = require('openmw.vr')

if not vr.isVr() then
    return {}
end

local ui = require('openmw.ui')
local util = require('openmw.util')
local I = require('openmw.interfaces')
local storage = require('openmw.storage')
local input = require('openmw.input')
local core = require('openmw.core')
local async = require('openmw.async')
local types = require('openmw.types')
local Player = types.Player
local self = require('openmw.self')
local common = require('scripts.omw.vrinputs.common')

local function controlsAllowed()
    return not core.isWorldPaused()
        and Player.getControlSwitch(self, Player.CONTROL_SWITCH.Controls)
        and not I.UI.getMode()
end

local rightHanded = true
local pointerRight = false
local pointerLeft = false

input.registerTriggerHandler('PointerActivate', async:callback(function(v)
    vr._pointerActivate()
end))


-- Note: 'MetaMenu' and 'MenuBack' are by default bound to the same key.
-- Similar to how ESC by default opens the game menu, OR closes the current menu.
-- The actions are disambiguated by the core.isWorldPaused check.
input.registerTriggerHandler('MetaMenu', async:callback(function()
    if not I.UI.getMode() and not core.isWorldPaused() then
        I.UI.addMode(I.UI.MODE.VrMetaMenu)
    end
end))

input.registerTriggerHandler('MenuBack', async:callback(function()
    if I.UI.getMode() and core.isWorldPaused() then
        -- There isn't a catch-all solution for closing the current menu item from lua.
        -- I.UI.removeMode can only remove modes, not dialogue boxes, the console, or the postprocessing hud.
        -- From VR I need a one click solution, so i am using this placeholder internal function.
        ui._menuBack()
    end
end))

common.setOnInputChangedBoolean(function(path, action)
    if action.valueBoolean then
        if not controlsAllowed() then
            -- The Menu script doesn't process inputs itself, so we need to send
            -- this event to allow input bindings to take place
            types.Player.sendMenuEvent(self, "RecordVRBinding", {path = path})
        end
    end
end)

local function updatePointer()
    local pointerLeft = input.getBooleanActionValue('PointerLeft') and vr.isControllerActive(common.controllers.LEFT_HAND)
    local pointerRight = input.getBooleanActionValue('PointerRight') and vr.isControllerActive(common.controllers.RIGHT_HAND)
    
    if pointerLeft and pointerRight then
        -- Both pointers are being activated at the same time, but i only 
        -- support one pointer at a time
        if rightHanded then 
            pointerLeft = false
        else
            pointerRight = false
        end
    end
    
    if not controlsAllowed() and not (pointerLeft or pointerRight) then
        -- We're in a menu of some sort. At least one pointer should always be active in this mode.
        if rightHanded then
            pointerRight = true
        else
            pointerLeft = true
        end
    end
    
    vr._setPointerLeft(pointerLeft)
    vr._setPointerRight(pointerRight)
    
end

local function onFrame(dt)
    
    common.onFrame(dt)
    updatePointer()
    
    local lookLeft = input.getRangeActionValue('LookLeft')
    local lookRight = input.getRangeActionValue('LookRight')
    
    self.controls.yawChange = (lookRight - lookLeft) * dt
end

return {
    engineHandlers = {
        onFrame = onFrame,
    },
    
    eventHandlers = {
    },

    interfaceName = 'vrinputs',
    ---
    -- 3D menus
    -- @module VRUI

    interface = {
        --- Interface version
        -- @field [parent=#VRUI] #number version
        version = 0,
        
        supportedInteractionPaths = supportedInteractionPaths,
        controllers = common.controllers,
    }
}
