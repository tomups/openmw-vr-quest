local vr = require('openmw.vr')

if not vr.isVr() then
    return {}
end

local ui = require('openmw.ui')
local util = require('openmw.util')
local I = require('openmw.interfaces')
local storage = require('openmw.storage')
local input = require('openmw.input')
local menu = require('openmw.menu')
local core = require('openmw.core')
local async = require('openmw.async')
local common = require('scripts.omw.vr.inputs.common')
local l10nContext = core.l10n(common.l10nKey)
local inputTypes = {
    action = input.actions,
    trigger = input.triggers,
}

local activeProfiles = {}

local function getActive(paths)
    for _, controller in pairs (common.controllers) do
        local profile = I.vrinputs.getInteractionProfileOfController(controller)
        if profile and paths[profile] and paths[profile][controller] then
            return paths[profile][controller]
        end
    end
    return nil
end

local function bindingLabel(isRecording, binding, id)
    local res = nil
    if isRecording then
        res = l10nContext('Recording')
    elseif binding and binding.paths then
        local activePath = getActive(binding.paths)
        if activePath then 
            res = common.getInteractionName(activePath) 
        end
    end
    
    if not res then 
        res = l10nContext('None')
    end
    return res
end

local bindingsVersion = 4
local bindingsVersionMinSupported = 4
local bindingSection = common.bindingSection
local userBindingsSection = common.userBindingsSection
local controlsSection = common.controlsSection
local version = bindingSection:get('version') or 0
if version < bindingsVersion then
    if version < bindingsVersionMinSupported then
        userBindingsSection:reset()
        bindingSection:reset()
        print('Motion Controller bindings have been reset due to: version changed')
        ui.showMessage('Motion Controller bindings have been reset due to: version changed')
    else
    -- Do any conversion here
    end
    --bindingSection:set('version', bindingsVersion)
end

local recording = nil
local justRecorded = false

local function clearActive(paths)
    for _, controller in pairs (common.controllers) do
        local profile = I.vrinputs.getInteractionProfileOfController(controller)
        if profile and paths[profile] then
            paths[profile][controller] = nil
        end
    end
end

local function setActivePath(paths, path)
    local targetController = common.controllers.RIGHT_HAND
    local otherController = common.controllers.LEFT_HAND
    if string.find(path, '/left/') then
        local targetController = common.controllers.LEFT_HAND
        local otherController = common.controllers.RIGHT_HAND
    end
    local profile = I.vrinputs.getInteractionProfileOfController(targetController)
    if not profile then
        print('Warning: Tried to bind to a non-existent controller')
        return
    end
    paths[profile] = paths[profile] or {}
    paths[profile][targetController] = path
    -- Only allow one binding on each profile
    paths[profile][otherController] = nil
end

I.Settings.registerRenderer('inputBindingVR', function(value, set, arg)
    if type(value) ~= 'table' then error('inputBindingVR: must have a table default value, got '..tostring(type(value))..' instead') end
    if type(value.id) ~= 'string' then error('inputBindingVR: default value table must contain a string id value') end
    if not value.type then error('inputBindingVR: default value table must contain a string type value') end
    if not inputTypes[value.type] then error('inputBindingVR: type must be "action" or "trigger"') end
    if not value.key then error('inputBindingVR: default value table must contain a string type key') end
    local info = inputTypes[value.type][value.key]
    if not info then print(string.format('inputBindingVR: %s %s not found', value.type, value.key)) return end
   
    local l10n = core.l10n(info.l10n)

    -- TODO: It would be nice to include actionSet/longPress information
    -- directly in the menu so users don't have to guess what conflicts and what doesn't.
    --local name = {
    --    template = I.MWUI.templates.textNormal,
    --    props = {
    --        text = l10n(info.name),
    --    },
    --}

    --local description = {
    --    template = I.MWUI.templates.textNormal,
    --    props = {
    --        text = l10n(info.description),
    --    },
    --}

    local binding = bindingSection:getCopy(value.id)
    if not binding and value.paths then
        binding = {}
        binding.key = value.key
        binding.type = value.type
        binding.actionSets = value.actionSets
        if not binding.actionSets or #binding.actionSets == 0 then
            binding.actionSets = common.ActionSetTemplates.Always
        end
        binding.paths = value.paths
        binding.long = value.long
        if binding.long == nil then
            binding.long = false
        end
        bindingSection:set(value.id, binding)
        print(tostring(value.id)..': set to defaults')
    end
    local label = bindingLabel(recording and recording.value.id == value.id, binding, value.id)

    local recorder = {
        template = I.MWUI.templates.textNormal,
        props = {
            text = label,
        },
        events = {
            -- Although we're in VR. The VR pointer fakes mouse clicks so we're still
            -- listening for a mouseclick here.
            mouseClick = async:callback(function()
                if recording ~= nil then return end
                if binding ~= nil and not value.required then 
                    -- clear any bound paths in active profiles
                    clearActive(binding.paths)
                    bindingSection:set(value.id, binding) 
                end
                recording = {
                    value = value,
                    refresh = function() set(value) end,
                }
                recording.refresh()
            end),
        },
    }

    local row = {
        type = ui.TYPE.Flex,
        props = {
            horizontal = true,
        },
        content = ui.content {
            --name,
            --{ props = { size = util.vector2(10, 0) } },
            recorder,
        },
    }
    local column = {
        type = ui.TYPE.Flex,
        content = ui.content {
            row,
            --description,
        },
    }

    return column
end)

local function tableHasValueInCommon(t1, t2)
    for _, v1 in pairs(t1) do
        for _, v2 in pairs(t2) do
            if v1 == v2 then
                return true
            end
        end
    end
    return false
end

local function checkForConflict(binding1, binding2)
    --An actionSet with no action sets in common is never
    --a conflict
    if not tableHasValueInCommon(binding1.actionSets, binding2.actionSets) then
        return false
    end

    --Bindings with action sets in common can still be salvaged if they are both triggers
    --and one is a long press while the other is a short press.
    if binding1.type == 'trigger' and binding2.type == 'trigger'
            and binding1.long ~= binding2.long then
        return false
    end

    return true
end

local function findConflicts(path, rebindingId, rebinding)
    local actionSets = rebinding.actionSets
    local long = rebinding.long
    local conflicts = {}
    for id, binding in pairs(bindingSection:asTable()) do
        if id ~= 'version' and id ~= rebindingId then
            if getActive(binding.paths) == path then
                if checkForConflict(rebinding, binding) then
                    conflicts[#conflicts+1] = id
                end
            end
        end
    end
    if #conflicts > 0 then
        return conflicts
    end
    return nil
end

local function bindInteraction(path)
    if recording == nil then return end
    
    if path:sub(-5) == 'touch' then
        -- Before binding touch, we need a system to disambiguate touch from click otherwise
        -- it's impossible to bind click without binding touch.
        return
    end

    local binding = bindingSection:getCopy(recording.value.id)
    local conflicts = findConflicts(path, recording.value.id, binding)
    if conflicts then
        local label = common.getInteractionName(path)
        for _, conflict in pairs(conflicts) do
            -- TODO: Dig up the proper name of the binding and use its l10n context
            print(l10nContext("Unable to bind to {Input}, conflicts with existing {Binding}", {Input = label, Binding = conflict}))
            ui.showMessage(l10nContext("Unable to bind to {Input}, conflicts with existing {Binding}", {Input = label, Binding = conflict}))
        end
    else
        print('No conflicts')
        binding.paths = binding.paths or {}
        setActivePath(binding.paths, path)
        bindingSection:set(recording.value.id, binding)
    end

    local refresh = recording.refresh
    recording = nil
    refresh()
    -- Notify input handler that we just finished recording an input.
    justRecorded = true
end

local initialized = false
local function init()
    common.registerSettingsGroup()
    common.registerSettingsPage()

    common.setOnInputChangedBoolean(function(path, action, value)
    if value then
        bindInteraction(path)
    else
        justRecorded = false
    end
    end)

    -- Current .RC does not process actions during the main menu.
    -- So until a game has been loaded we have to manually deal with menu bindings.
    common.setManualTriggerCallback('PointerActivate', function()
        if not justRecorded then
            vr._pointerActivate(true)
        end
    end)

    common.setManualTriggerCallback('MenuBack', async:callback(function()
        -- There isn't a catch-all solution for closing the current menu item from lua.
        -- I.UI.removeMode can only remove modes, not dialogue boxes, the console, or the postprocessing hud.
        -- From VR I need a one click solution, so i am using this placeholder internal function.
        ui._menuBack()
    end))

    common.setManualTriggerCallback('Recenter', async:callback(function()
        print('Recenter')
        I.vrspaces.recenterXY()
    end))
end

local gameLoaded = false

local function onFrame(dt)
    if not initialized then
        init()
        initialized = true
    end
    for _, controller in pairs (common.controllers) do
        local profile = I.vrinputs.getInteractionProfileOfController(controller)
        if profile ~= activeProfiles[controller] then
            I.Settings.updateRender(common.settingsPageKey)
        end
        activeProfiles[controller] = profile
    end

    -- Only want to process inputs/actions in this script during pre-game.
    if menu.getState() == menu.STATE.NoGame then
        common.onFrame(dt)
    end

    if not gameLoaded and menu.getState() == menu.STATE.Running then
        -- Now managed by the Player script, we can disable this callback.
        --common.setManualTriggerCallback('PointerLeft', nil)
        --common.setManualTriggerCallback('PointerRight', nil)
        common.setManualTriggerCallback('PointerActivate', nil)
        common.setManualTriggerCallback('MenuBack', nil)
        common.setManualTriggerCallback('Recenter', nil)
    end
end
return {
    engineHandlers = {
        onFrame = onFrame,
    },
    
    eventHandlers = {
        RecordVRBinding = function(data) bindInteraction(data.path) end,
    },

    interfaceName = 'vrinputs',
    ---
    -- vr inputs
    -- @module vrinputs

    interface = {
        --- Interface version
        -- @field [parent=#vrinputs] #number version
        version = 0,
        getInteractionProfileOfController = common.getInteractionProfileOfController,
        isKBMouseMode = common.isKBMouseMode,
        controllers = common.controllers,
    }
}
