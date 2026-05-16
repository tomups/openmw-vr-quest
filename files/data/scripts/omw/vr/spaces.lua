---
-- VR spaces. 
-- See also documentation of the onVRFrame() engine handler.
-- @module vrspaces

local vr = require('openmw.vr')
local util = require('openmw.util')
local auxUtil = require('openmw_aux.util')
local I = require('openmw.interfaces')

local V3 = util.vector3

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

local function ensurePose(pose)
    pose = auxUtil.shallowCopy(pose or {})
    pose.position = pose.position or V3(0,0,0)
    pose.orientation = pose.orientation or util.transform.identity
    return pose
end

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

local function isDerivedSpace(name)
    return customSpaces[name] ~= nil
end

local function getReferenceSpace(name)
    if not isDerivedSpace(name) then
        error('getReferenceSpace: Can only get origin space for derived spaces')
    end
    return customSpaces[name].reference
end

local function createDerivedSpace(name, reference, pose)
    if not reference then
        error('createDerivedSpace: reference cannot be nil')
    end
    vr._createDerivedSpace(name, reference, pose)
    customSpaces[name] = {
        name = name,
        reference = reference,
        pose = ensurePose(pose),
        offset = {
            position = V3(0,0,0),
            orientation = util.transform.identity,
        }
    }
end

local function offsetDerivedSpace(space, offset)
    local cs = customSpaces[space]
    if not cs then
        error(tostring(space)..' is not an existing custom space')
    end
    offset = ensurePose(offset)
    vr._createDerivedSpace(cs.name, cs.reference, I.vrspaces.poseUtils.add(cs.pose, offset))
    cs.offset = offset
end

local function subtractPose(pose1, pose2)
    local pos1 = pose1.position or util.vector3(0,0,0)
    local ori1 = pose1.orientation or util.transform.identity
    local pos2 = pose2.position or util.vector3(0,0,0)
    local ori2 = pose2.orientation or util.transform.identity
    local ori3 = ori2:inverse() * ori1
    local pos3 = ori2:inverse() * (pos1 - pos2)

    return {
        position = pos3,
        orientation = ori3,
    }
end

local function addPose(pose1, pose2)
    local pos1 = pose1.position or util.vector3(0,0,0)
    local ori1 = pose1.orientation or util.transform.identity
    local pos2 = pose2.position or util.vector3(0,0,0)
    local ori2 = pose2.orientation or util.transform.identity
    local pos3 = pos1 + ori1 * pos2
    local ori3 = ori2 * ori1

    return {
        position = pos3,
        orientation = ori3,
    }
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

        --- Offset a derived space.
        -- Add a fixed offset to a derived space. This is added to the existing offset passed to createDerivedSpace.
        -- This function is used by the spaceOffset setting type to implement user configurable spaces.
        -- @function [parent=#vrspaces]
        -- @param #string name The derived space name. Must be an existing custom space. Action or reference spaces must be true and cannot be offset.
        -- @param #Pose pose
        offsetDerivedSpace = offsetDerivedSpace,

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
        -- @return #boolean
        isActionSpace = isActionSpace,

        --- Check if the given string is the name of a custom space
        -- @function [parent=#vrspaces] isDerivedSpace
        -- @param #string name
        -- @return #boolean
        isDerivedSpace = isDerivedSpace,

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

        poseUtils = {
            subtract = subtractPose,
            add = addPose,
        },
    }
}
