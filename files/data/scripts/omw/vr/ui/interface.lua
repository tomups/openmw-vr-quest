local vr = require('openmw.vr')
local ui = require('openmw.ui')
local util = require('openmw.util')
local I = require('openmw.interfaces')
local storage = require('openmw.storage')
local input = require('openmw.input')
local core = require('openmw.core')
local async = require('openmw.async')

if not vr.isVr() then
    return {}
end

-- We need to manage some layers that haven't been exposed to Lua yet:
-- These are MyGUI layers that i don't want to expose directly to Lua, but i still want them to be manageable from Lua.
-- So i made .set*Config methods in I.vrui for these
-- And one I.vrui.setDefaultWindowConfig which sets a default config for all remaining layers that haven't been exposed to Lua
local secretItems = {
    'Modal',
    'RadialMenu',
    'Windows',
    'Video',
    'InputBlocker',
    'Settings',
    'Console',
    'ListBox',
    'MainMenu',
    'MainMenuBackground',
    'LoadingScreen',
    'LoadingScreenBackground',
    'Debug',
}

return {
    interfaceName = 'vrui',
    ---
    -- 3D menus
    -- @module vrui

    interface = {
        --- Interface version
        -- @field [parent=#vrui] #number version
        version = 0,
        
        --- Set config for all hidden layers/windows that aren't exposed to lua yet
        setDefaultWindowConfig = function(config)
            for _, item in pairs(secretItems) do
                vr._setLayerConfig(item, config)
            end
        end,
        setDefaultWindowPose = function(pose)
            for _, item in pairs(secretItems) do
                vr._setLayerPose(item, pose)
            end
        end,
        
        setModeConfig = function(mode, options) 
            vr._setModeConfig(mode, options) 
        end,
        setModePose = function(mode, pose, window) vr._setModePose(mode, pose, window) end,
        setNotificationConfig = function(config) vr._setLayerConfig('Notification', config) end,
        setNotificationPose = function(pose) vr._setLayerPose('Notification', pose) end,
        setHUDConfig = function(config) vr._setLayerConfig('HUD', config) end,
        setHUDPose = function(pose) vr._setLayerPose('HUD', pose) end,
        setTooltipConfig = function(config) vr._setLayerConfig('Tooltip', config) end,
        setTooltipPose = function(pose) vr._setLayerPose('Tooltip', pose) end,
        setVirtualKeyboardConfig = function(config) vr._setLayerConfig('VirtualKeyboard', config) end,
        setVirtualKeyboardPose = function(pose) vr._setLayerPose('VirtualKeyboard', pose) end,
    }
}
