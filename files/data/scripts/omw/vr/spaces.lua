local vr = require('openmw.vr')
local util = require('openmw.util')
local I = require('openmw.interfaces')

if not vr.isVr() then
    return {}
end

local UnitsPerMeter = 69.99125109
local customSpaces = {
}

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

local referenceSpaces = {
    Local = "/default/reference/local",
    View = "/default/reference/view",
    Stage = "/default/reference/stage",
}

local referenceSpaceToEnum = {
    [referenceSpaces.Local] = vr.REFERENCE_SPACES.Local,
    [referenceSpaces.View] = vr.REFERENCE_SPACES.View,
    [referenceSpaces.Stage] = vr.REFERENCE_SPACES.Stage,
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
    return referenceSpaceToEnum[space] ~= nil 
end

local function isActionSpace(space)
    for _, s in pairs(actionSpaces) do
        if s == space then return true end
    end
    return false
end

local function isReferenceSpaceSupported(space)
    local refSpace = referenceSpaceToEnum[space]
    if not refSpace then
        error('Error: space '..tostring(space)..' is not a reference space', 2)
    end
    return supportedReferenceSpaces[refSpace]
end

local function isActionSpaceSupported(space)
    return supportedActionSpaces[space] ~= nil
end

local function createDerivedSpace(name, reference, pose)
    vr.createDerivedSpace(name, reference, pose)
    customSpaces[name] = name
end

return {
    interfaceName = 'vrspaces',
    ---
    -- 3D menus
    -- @module VRUI

    interface = {
        --- Interface version
        -- @field [parent=#VRUI] #number version
        version = 0,
        UnitsPerMeter = UnitsPerMeter,
        customSpaces = customSpaces,
        actionSpaces = actionSpaces,
        referenceSpaces = referenceSpaces,
        createDerivedSpace = createDerivedSpace,
        isReferenceSpaceSupported = isReferenceSpaceSupported,
        isActionSpaceSupported = isActionSpaceSupported,
        isActionSpace = isActionSpace,
        isReferenceSpace = isReferenceSpace,
        isSpaceActive = function(space) return vr.locateSpace(space) ~= nil end,
        recenterXY = vr._recenterXY,
        recenterZ = vr._recenterZ,
    }
}
