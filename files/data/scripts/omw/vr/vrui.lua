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

return {
    interfaceName = 'vrui',
    ---
    -- 3D menus
    -- @module VRUI

    interface = {
        --- Interface version
        -- @field [parent=#VRUI] #number version
        version = 0,
    
        setLayerConfig = vr._setGuiLayer,
        setLayerPose = vr._setGuiPose,
    }
}
