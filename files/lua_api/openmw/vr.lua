---
-- `openmw.vr` virtual reality interface.
-- Can be used only by menu scripts and local scripts, that are attached to a player.
-- Much of the API is hidden, and exposed via the @{interface_vrinputs}, @{interface_vrspaces}, and @{interface_vrui} interfaces.
-- @module vr
-- @usage
-- local vr = require('openmw.uvr')
-- if not vr.isVr() then
--   return
-- end

---
-- Available interaction value types in OpenXR
-- @type INTERACTION_VALUE_TYPE
-- @field Boolean BOOLEAN
-- @field Float FLOAT
-- @field Axis AXIS
-- @field Pose POSE

--- A spatial pose, consisting of a position and an orientation
-- @type Pose
-- @field openmw.util#Vector3 position @{openmw.util#Vector3}
-- @field openmw.util#Transform orientation @{openmw.util#Transform}

---
-- Controller path values
-- @field [parent=#vr] #list<#string> controllerPaths

---
-- Interaction value types
-- @field [parent=#vr] #INTERACTION_VALUE_TYPE INTERACTION_VALUE_TYPES

---
-- Supertable of all interactions supported by the runtime.
-- The structure of the table is { interactionProfilePath = { controllerPath = { interactionPath = @{#INTERACTION_VALUE_TYPE},},}, }
-- I recommend printing out the table yourself to see what's in it to understand it better.
-- @field [parent=#vr] #table availableInteractions
-- @usage
-- local I = require('openmw.interfaces')
-- local vr = require('openmw.vr')
-- local function printAvailableInteractions(controller)
--     local profile = I.vrinputs.getInteractionProfileOfController(controller)
--     if not vr.availableInteractions[profile] then
--         print('Profile '..tostring(profile)..' not supported')
--         return
--     end
--     print('Available interactions for '..controller..' controller of '..profile..': ')
--     for k,v in pairs(vr.availableInteractions[profile][controller]) do
--         print(' - '..k..' [type:'..v..']')
--     end
-- end
--
-- printAvailableInteractions(I.vrinputs.controllers.LEFT_HAND)
-- printAvailableInteractions(I.vrinputs.controllers.RIGHT_HAND)
--

---
-- @function [parent=#vr] isVr
-- @return #boolean

---
-- @function [parent=#vr] isLeftHandedMode
-- @return #boolean

---
-- @function [parent=#vr] isControllerActive
-- @param #CONTROLLER_PATH controller 
-- @return #boolean



return nil
