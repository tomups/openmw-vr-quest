local vr = require('openmw.vr')
if not vr.isVr() then
    return {}
end
local menu = require('openmw.menu')
local util = require('openmw.util')
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

return {
    engineHandlers = {
        onVRFrame = function()
            if not initialized then 
                init()
            end
        end,
        onVRRecenter = function()
            -- This script should only manage anything if no game has been loaded
            if menu.getState() ~= menu.STATE.Running then
                -- re-init to update the menu poses
                init()
            end
        end
    },
    interfaceName = 'vrui',
    interface = common.interface,
}