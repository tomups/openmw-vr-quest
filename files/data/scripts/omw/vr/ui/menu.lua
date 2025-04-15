local vr = require('openmw.vr')
if not vr.isVr() then
    return {}
end
local menu = require('openmw.menu')
local util = require('openmw.util')
local I = require('openmw.interfaces')
local common = require('scripts.omw.vr.ui.common')

common.createDerivedSpaces()

-- For some reason, I.UI is not available to MENU scripts, preventing me from properly managing UI based on MODE from this script. 
-- Instead i have to base myself on the knowledge that these and only these modes exist before loading a game.
-- Anything not configured here is configured in the PLAYER script menus.lua
local modes = {"Loading", "LoadingWallpaper", "MainMenu"}
common.setupDefaults(modes)
common.registerSettingsPage()
common.registerSettingsGroup()

local initialized = false
local function init()
    common.updatePoses()
    common.updateModes(modes)
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
    }
}