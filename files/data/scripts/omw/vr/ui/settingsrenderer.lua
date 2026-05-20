local vr = require('openmw.vr')
if not vr.isVr() then
    return {}
end
local util = require('openmw.util')
local input = require('openmw.input')
local async = require('openmw.async')
local I = require('openmw.interfaces')
local core = require('openmw.core')
local ui = require('openmw.ui')
local auxUtil = require('openmw_aux.util')
local buttonConstructor = require('scripts.omw.vr.ui.button')

local recording = nil
local recorderSpace = 'SettingsOffsetRecorderSpace'
local l10n = core.l10n('OMWVRUI')

local normalColor = util.color.commaString(core.getGMST("FontColor_color_normal"))
local V2 = util.vector2
local V3 = util.vector3

local recordingHandlers = {
    require('scripts.omw.vr.ui.defaultRecorderHandler')
}
local function addRecordingHandler(handler)
    recordingHandlers[#recordingHandlers+1] = handler
end

local function makeLabel(pose)
    if pose and pose.position then
        return string.format('%.2f, %.2f, %.2f', pose.position.x, pose.position.y, pose.position.z)
    end
    return 'None'
end

local function getBindingDescription(binding)
    local bindings = I.vrinputs.getActiveTriggerBindings(binding)
    for id, path in pairs(bindings or {}) do
        return I.vrinputs.getInteractionName(path)
    end
    return '('..l10n('NotBound')..')'
end

local function getBaseReferenceSpace(space)
    while not I.vrspaces.isReferenceSpace(space) and not I.vrspaces.isActionSpace(space) do
        space = I.vrspaces.customSpaces[space].reference
    end
    return space
end

local function disable(disabled, layout)
    if disabled then
        return {
            template = I.MWUI.templates.disabled,
            content = ui.content {
                layout,
            },
        }
    else
        return layout
    end
end

local function endRecording(accept)
    local space = recording.space
    local spaceInfo = recording.originalSpaceInfo
    -- Restore the original non-fixed space
    I.vrspaces.createDerivedSpace(space, spaceInfo.reference, spaceInfo.pose)
    if accept then
        recording.accept(recording.offset)
    else
        I.vrspaces.offsetDerivedSpace(space, spaceInfo.offset)
    end
    recording = nil
    auxUtil.callEventHandlers(recordingHandlers, space, false)
end

local function recordingCancel()
    print('Recording cancelled')
    recording.accepted = false
end

local function recordingAccept()
    print('Recording accepted')
    recording.accepted = true
end

local recordingDialogueText = {
    type = ui.TYPE.TextEdit,
    props = {
        text = 'placeholder',
        textSize = 16,
        autoSize = true,
        size = V2(350, 0),
        multiline = true,
        wordWrap = true,
        readOnly = true,
        textColor = normalColor,
    }
}

local recordingAcceptButton = buttonConstructor('OK', recordingAccept)
local recordingCancelButton = buttonConstructor('Cancel', recordingCancel)

recordingAcceptButton.element.layout.props.position = V2(8, 2)
recordingCancelButton.element.layout.props.position = V2(300, 2)

local recordingDialogueButtons = {
    type = ui.TYPE.Widget,
    props = {
        size = V2(360, 30),
    },
    content = ui.content{recordingAcceptButton.element, recordingCancelButton.element}
}

local recordingDialogueFlex = {
    type = ui.TYPE.Flex,
    name = 'recordingVerticalFlex',
    props = {
        autoSize = true,
        arrange = ui.ALIGNMENT.Center,
        position = V2(8, 7),
    },
    content = ui.content{
        recordingDialogueText,
    --    recordingDialogueButtons,
    }
}

local recordingDialogueLayer = 'SpaceOffsetRecording'
ui.layers.insertAfter('MainMenu', recordingDialogueLayer, {interactive = true})
local recordingDialogue = ui.create{
    template = I.MWUI.templates.boxSolidThick,
    content = ui.content{recordingDialogueFlex},
    layer = recordingDialogueLayer,
    props = { visible = false }
}

local buttons = {}

I.Settings.registerRenderer('spaceOffset', function(value, set, arg)
    if not arg or not arg.space then
        error('spaceOffset: Must have an argument with a space and a reference space')
    end
    local space = arg.space
    if type(space) ~= 'string' then error('spaceOffset: space must be a string identifier') end
    if I.vrspaces.isReferenceSpace(space) or I.vrspaces.isActionSpace(space) then
        error('spaceOffset: argument space must be a custom space. setting offsets on built-in spaces is not allowed')
    end
    if not I.vrspaces.isSpace(space) then
        error('spaceOffset: argument space must be an existing custom space.')
    end
    local invalidReferences = {
        [I.vrspaces.referenceSpaces.Local] = true,
        [I.vrspaces.referenceSpaces.Stage] = true,
    }
    if invalidReferences[getBaseReferenceSpace(space)] then
        error('spaceOffset: argument space "'..tostring(space)..'" cannot be located in fixed reference spaces such as Local or Stage. spaceOffsets can only be defined for action spaces and the View reference space')
    end
    local label = makeLabel(value)
    I.vrspaces.offsetDerivedSpace(space, value)
    local cb = function()
        if recording then
            print('Warning: spaceOffset: Attempted to create a new recording while another recording was active')
            return
        end
        recording = {
            space = space,
            accept = function(offset)
                I.vrspaces.offsetDerivedSpace(space, offset)
                set(offset)
            end,
        }
    end

    if buttons[space] then
        -- Settings API has already destroyed this button's element, but the widget still needs to run its own cleanup
        buttons[space]:cleanup()
    end

    buttons[space] = buttonConstructor(label, cb)
    return disable(arg.disabled, buttons[space].element)
end)

local function registerTriggerHandlers()
    input.registerTriggerHandler('PointerActivate', async:callback(function()
        if recording then
            recording.accepted = true
        end
    end))

    input.registerTriggerHandler('MenuBack', async:callback(function()
        if recording then
            recording.accepted = false
        end
    end))
end
registerTriggerHandlers()
-- OpenMW bug: https://gitlab.com/OpenMW/openmw/-/work_items/9115
I.vrinputs.addMenuTriggersHeartBeatListener(registerTriggerHandlers)

local lastInputTypeUsed = 'Keyboard'
local inputTypeToDialogueText = {
    Keyboard = l10n('SpaceOffsetRecordingDialogue2Keyboard'),
    -- Not Used: Mouse = l10n('SpaceOffsetRecordingDialogue2Mouse'),
    Controller = l10n('SpaceOffsetRecordingDialogue2Controller'),
}
local function spaceOffsetRecordingDialogueText(offset)
    local text = l10n('SpaceOffsetRecordingDialogue1', {offset = makeLabel(offset)})
    if I.vrinputs.isKBMouseMode() then
        print('Using: '..lastInputTypeUsed..'!!')
        text = text..'\n'..inputTypeToDialogueText[lastInputTypeUsed]
    else
        text = text..'\n'..l10n('SpaceOffsetRecordingDialogue2MC', {AcceptBinding = getBindingDescription('PointerActivate'), CancelBinding = getBindingDescription('MenuBack')})
    end
    return text
end


local function onVRFrame()
    if not recording then
        if recordingDialogue.layout.props.visible then
            recordingDialogue.layout.props.visible = false
            recordingDialogue:update()
            I.vrui.setLayerPickable(recordingDialogueLayer, false)
        end
        return
    end

    if recording.accepted ~= nil then
        print('Ending recording: '..tostring(recording.accepted))
        endRecording(recording.accepted)
        return
    end

    local space = recording.space

    if not recording.initialized then

        -- Spaces are defined as fixed offsets from their reference space. So it makes no sense to record them by watching how they change
        -- with respect to their defined reference. Instead we need to track how they change within a fixed reference, such as Local.
        recording.origin = I.vrspaces.locateSpace(space, I.vrspaces.referenceSpaces.Local)

        -- Check that the requested space is actually active
        if not recording.origin then
            recording = nil
            error('Cannot change offset of inactive space: '..tostring(space)..', dependent on inactive reference '..tostring(getBaseReferenceSpace(space)))
        end
        auxUtil.callEventHandlers(recordingHandlers, space, true)

        -- To give a live preview of the offset, we need to pause the space being changed.
        -- We do this by redefining it as a space defined in reference to Local and instead tracking a clone of the original
        -- First create a clone of the original
        local spaceInfo = I.vrspaces.customSpaces[space]
        recording.originalSpaceInfo = auxUtil.shallowCopy(spaceInfo)
        I.vrspaces.createDerivedSpace(recorderSpace, spaceInfo.reference, spaceInfo.pose)
        --I.vrspaces.offsetDerivedSpace(recorderSpace, spaceInfo.offset)
        -- Now redefined the target space as a local fixed space
        -- Note that this deletes the offset so we'll need to redefine the offset on cancel
        I.vrspaces.createDerivedSpace(space, I.vrspaces.referenceSpaces.Local, recording.origin)
        recording.initialized = true
    end

    local pose = I.vrspaces.locateSpace(recorderSpace, I.vrspaces.referenceSpaces.Local)
    if not pose then
        error('Failed to locate space '..recorderSpace)
    end
    -- (recording.origin - pose) is the new offset
    recording.offset = I.vrspaces.poseUtils.subtract(recording.origin, pose)

    recordingDialogue.layout.props.visible = true
    recordingDialogueText.props.text = spaceOffsetRecordingDialogueText(recording.offset)
    recordingDialogue:update()
    I.vrui.setLayerPickable(recordingDialogueLayer, true)

    -- To avoid confusion / accidentally activating something unrelated, disable pointers during this dialogue.
    -- We could use yes/no buttons but it might be annoying to keep the desired offset stable while also pointing
    -- and clicking at buttons.
    vr._setPointerLeft(false)
    vr._setPointerRight(false)
end

return {
    engineHandlers = {
        onVRFrame = onVRFrame,
        onKeyPress = function(key)
            lastInputTypeUsed = 'Keyboard'
            if not recording then return end
            if key.code == input.KEY.Escape then
                recordingCancel()
            elseif key.code == input.KEY.Enter then
                recordingAccept()
            end
        end,
        onControllerButtonPress = function(id)
            lastInputTypeUsed = 'Controller'
            if not recording then return end
            if id == input.CONTROLLER_BUTTON.B or id == input.CONTROLLER_BUTTON.Back then
                recordingCancel()
            elseif id == input.CONTROLLER_BUTTON.A or id == input.CONTROLLER_BUTTON.Start then
                recordingAccept()
            end
        end
    },
    interfaceName = 'SpaceOffsetSettingsRenderer',
    -- Used to provide handlers for when the space offset settings renderer
    -- is running for a space. Useful to enable displaying some visual clue to the user.
    interface = {
        version = 0,
        addRecordingHandler = addRecordingHandler,
    }
}
