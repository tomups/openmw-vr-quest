local vr = require('openmw.vr')

return {

    interfaceName = 'MotionControls',
    ---
    -- Motion control interface
    -- Manages motion controller bindings when the motion controller is active.
    -- @module MotionControls

    interface = {
        --- Interface version
        -- @field [parent=#MotionControls] #number version
        version = 0,

        --- Checks if the gamepad cursor is active. If it is active, the left stick can move the cursor, and A will be interpreted as a mouse click.
        -- @function [parent=#MotionControls] isGamepadCursorActive
        -- @return #boolean
        isMotionControlsActive = function()
            return vr._isMotionControlsActive()
        end,
    }
}
