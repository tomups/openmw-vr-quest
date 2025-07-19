---
-- VR spaces. 
-- See also documentation of the onVRFrame() engine handler.
-- @module vrspaces

local vr = require('openmw.vr')
local util = require('openmw.util')
local I = require('openmw.interfaces')

if not vr.isVr() then
    return {}
end

--- A spatial pose, consisting of a position and an orientation
-- @type Pose
-- @field openmw.util#Vector3 position @{openmw.util#Vector3}
-- @field openmw.util#Transform orientation @{openmw.util#Transform}

local unitsPerMeter = 69.99125109
local customSpaces = {
}

--- Names of ActionSpace type spaces. These are trackers besides the HMD.
-- @type ActionSpace
-- @field #string LeftHandAim /user/hand/left/input/aim/pose
-- @field #string LeftHandGrip /user/hand/left/input/grip/pose
-- @field #string LeftHandPalm /user/hand/left/input/palm_ext/pose
-- @field #string LeftHandPinch /user/hand/left/input/pinch_ext/pose
-- @field #string LeftHandPoke /user/hand/left/input/poke_ext/pose
-- @field #string RightHandAim /user/hand/right/input/aim/pose
-- @field #string RightHandGrip /user/hand/right/input/grip/pose
-- @field #string RightHandPalm /user/hand/right/input/palm_ext/pose
-- @field #string RightHandPinch /user/hand/right/input/pinch_ext/pose
-- @field #string RightHandPoke /user/hand/right/input/poke_ext/pose
local actionSpaces = {
    LeftHandAim = "/user/hand/left/input/aim/pose",
    LeftHandGrip = "/user/hand/left/input/grip/pose",
    LeftHandPalm = "/user/hand/left/input/palm_ext/pose",
    LeftHandPinch = "/user/hand/left/input/pinch_ext/pose",
    LeftHandPoke = "/user/hand/left/input/poke_ext/pose",
    RightHandAim = "/user/hand/right/input/aim/pose",
    RightHandGrip = "/user/hand/right/input/grip/pose",
    RightHandPalm = "/user/hand/right/input/palm_ext/pose",
    RightHandPinch = "/user/hand/right/input/pinch_ext/pose",
    RightHandPoke = "/user/hand/right/input/poke_ext/pose",
}

--- Names of reference type spaces. These are HMD spaces that act as origins for other spaces.
-- @type ReferenceSpace
-- @field #string Local Provides a gravity-locked origin around the initial position of the headest.
-- @field #string View The origin is the HMD itself, and will be constantly moving with the player.
-- @field #string Stage A gravity-locked, stable stage defined by the implementation. May not be available, so check its availability before use.
local referenceSpaces = {
    Local = "/default/reference/local",
    View = "/default/reference/view",
    Stage = "/default/reference/stage",
}

local supportedReferenceSpaces = {}
for _, refSpace in pairs(vr.availableReferenceSpaces) do
    print('Supporting reference space: '..refSpace)
    supportedReferenceSpaces[refSpace] = true
end

local supportedActionSpaces = {}
for profile, controllers in pairs(vr.availableInteractions) do
    for controller, interactions in pairs(controllers) do
        for interaction, interactionType in pairs(interactions) do
            if interactionType == vr.INTERACTION_VALUE_TYPES.Pose then
                supportedActionSpaces[controller..interaction] = controller..interaction
            end
        end
    end
end

local function isReferenceSpace(space) 
    for _, v in pairs(referenceSpaces) do
        if v == space then
            return true
        end
    end
    return false
end

local function isActionSpace(space)
    for _, s in pairs(actionSpaces) do
        if s == space then return true end
    end
    return false
end

local function isReferenceSpaceSupported(space)
    return supportedReferenceSpaces[space]
end

local function isActionSpaceSupported(space)
    return supportedActionSpaces[space] ~= nil
end

local function createDerivedSpace(name, reference, pose)
    vr._createDerivedSpace(name, reference, pose)
    customSpaces[name] = name
end

return {
    interfaceName = 'vrspaces',

    interface = {
        --- Interface version
        -- @field [parent=#vrspaces] #number version
        version = 0,

        --- Constant used to convert between MW units and meters
        -- @field [parent=#vrspaces] #number unitsPerMeter
        unitsPerMeter = unitsPerMeter,

        --- Table of existing custom spaces
        -- @field [parent=#vrspaces] #table customSpaces
        customSpaces = customSpaces,

        --- Table of existing ${#ActionSpace}. *Aim and *Grip spaces will always be supported, but *Palm, *Pinch, and *Poke may not be depending on the VR runtime. Always check if these are supported before trying to use them.
        -- @field [parent=#vrspaces] #table actionSpaces
        actionSpaces = actionSpaces,

        --- Table of ${#ReferenceSpace}. Local and View will always be supported, but Stage may not. Check before using Stage.
        -- @field [parent=#vrspaces] #table referenceSpaces
        referenceSpaces = referenceSpaces,

        --- Create a derived space.
        -- A derived space is a space with some fixed offset from a given space.
        -- @function [parent=#vrspaces] createDerviedSpace
        -- @param #string name The name to give the derived space. If not unique, will override the existing derived space. Can not be used to override reference spaces or action spaces.
        -- @param #string reference The name of the space to derive from. Reference may itself be a derived space.
        -- @param #Pose pose The offset from the reference.
        createDerivedSpace = createDerivedSpace,

        --- Check if a reference space is supported.
        -- @function [parent=#vrspaces] isReferenceSpaceSupported
        -- @param #ReferenceSpace name The name of the reference space
        isReferenceSpaceSupported = isReferenceSpaceSupported,

        --- Check if an action space is supported
        -- @function [parent=#vrspaces] isActionSpaceSupported
        -- @param #ActionSpace name The name of the action space
        isActionSpaceSupported = isActionSpaceSupported,

        --- Check if the given string is the name of an action space
        -- @function [parent=#vrspaces] isActionSpace
        -- @param #string name
        isActionSpace = isActionSpace,

        --- Check if the given string is the name of a reference space
        -- @function [parent=#vrspaces] isReferenceSpace
        -- @param #string name
        isReferenceSpace = isReferenceSpace,

        --- Check if the given string is the name of a space that is currently active.
        -- @function [parent=#vrspaces] isSpaceActive
        -- @param #string name
        isSpaceActive = function(space) return vr.locateSpace(space) ~= nil end,

        --- Check if the given string is the name of a space
        -- @function [parent=#vrspaces] isSpace
        -- @param #string name
        isSpace = function(space) return vr._spaceExists(space) ~= nil end,

        --- Locate a space in relative terms.
        -- @function [parent=#vrspaces] locateSpace
        -- @param #string name Name of the space to locate
        -- @param #string reference (Optional) Name of the space to locate in reference to. Defaults to @{#ReferenceSpace.Local}
        -- @return #Pose
        locateSpace = function(space, ref) return vr._locateSpace(space, ref) end,

        --- Locate a space in the game world.
        -- @function [parent=#vrspaces] locateSpaceInWorld
        -- @param #string name Name of the space to locate
        -- @return #Pose
        locateSpaceInWorld = function(space) return vr._locateSpaceInWorld(space) end,

        --- Request a horizontal recenter
        -- @function [parent=#vrspaces] recenterXY
        recenterXY = vr._recenterXY,
        --- Request a vertical recenter.
        -- Note that this will reset physical sneak reference height, and is best only used at the request of the user.
        -- @function [parent=#vrspaces] recenterZ
        recenterZ = vr._recenterZ,
    }
}
