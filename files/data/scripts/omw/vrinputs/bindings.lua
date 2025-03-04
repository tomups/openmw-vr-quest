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
local function updatePointer()
    if not controlsAllowed() then
        self.controls.pointerLeft = true
        self.controls.pointerRight = true
    end
end

input.bindAction('PointerLeft', async:callback(function(dt, v)
    -- Pointers should always be on during menus
    return v or not controlsAllowed()
end), {})

input.bindAction('PointerRight', async:callback(function(dt, v)
    -- Pointers should always be on during menus
    return v or not controlsAllowed()
end), {})

input.registerActionHandler('PointerLeft', async:callback(function(v)
    self.controls.pointerLeft = v
end))

input.registerActionHandler('PointerRight', async:callback(function(v)
    self.controls.pointerLeft = v
end))

input.registerActionHandler('PointerActivate', async:callback(function(v)
    self.controls.pointerActivate = v
end))


-- Note: 'MetaMenu' and 'MenuBack' are by default bound to the same key.
-- Similar to how ESC by default opens the game menu, OR closes the current menu.
-- The actions are disambiguated by the core.isWorldPaused check.
input.registerActionHandler('MetaMenu', async:callback(function(v)
    if v and not I.UI.getMode() and not core.isWorldPaused() then
        I.UI.addMode(I.UI.MODE.VrMetaMenu)
    end
end))

input.registerActionHandler('MenuBack', async:callback(function(v)
    if v and I.UI.getMode() and core.isWorldPaused() then
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

local function onFrame(dt)
    common.onFrame(dt)
    updatePointer()
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
