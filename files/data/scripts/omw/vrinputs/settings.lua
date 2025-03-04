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
local common = require('scripts.omw.vrinputs.common')

local inputTypes = {
    action = input.actions,
    trigger = input.triggers,
}
local interfaceL10n = core.l10n('interface')

local function bindingLabel(isRecording, binding)
    if isRecording then
        return interfaceL10n('N/A')
    elseif binding and  binding.path then
        return common.getInteractionName(binding.path)
    else
        return interfaceL10n('None')
    end
end

local bindingsVersion = 1
local bindingsVersionMinSupported = 1
local bindingSection = storage.playerSection('OMWInputBindingsVR')
local version = bindingSection:get('version') or 0
if version < bindingsVersion then
    if version < bindingsVersionMinSupported then
        bindingSection:reset()
        ui.showMessage('Motion Controller bindings have been reset due to: version changed')
    else
    -- Do any conversion here
    end
    bindingSection:set('version', bindingsVersion)
end

-- Temp: Reset every time cause i keep breaking stuff
bindingSection:reset()

local recording = nil

I.Settings.registerRenderer('inputBindingVR', function(value, set, arg)
    if type(value) ~= 'table' then error('inputBindingVR: must have a table default value, got '..tostring(type(value))..' instead') end
    if type(value.id) ~= 'string' then error('inputBindingVR: default value table must contain a string id value') end
    if not value.type then error('inputBindingVR: default value table must contain a string type value') end
    if not inputTypes[value.type] then error('inputBindingVR: type must be "action" or "trigger"') end
    if not value.key then error('inputBindingVR: default value table must contain a string type key') end
    local info = inputTypes[value.type][value.key]
    if not info then print(string.format('inputBindingVR: %s %s not found', value.type, value.key)) return end
   
    local l10n = core.l10n(info.l10n)

    local name = {
        template = I.MWUI.templates.textNormal,
        props = {
            text = l10n(info.name),
        },
    }

    local description = {
        template = I.MWUI.templates.textNormal,
        props = {
            text = l10n(info.description),
        },
    }

    local binding = bindingSection:get(value.id)
    if not binding and value.path then
        -- 
        binding = {path = value.path, key = value.key, type = value.type}
        bindingSection:set(value.id, binding)
        print(tostring(value.id)..': defaulted to '..tostring(binding.path))
    end
    local label = bindingLabel(recording and recording.id == value.id, binding)

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
                if binding ~= nil then 
                    -- binding is userdata, so binding.path = nil won't work
                    -- have to remake the table instead
                    binding = {key = value.key, type = value.type}
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
            name,
            { props = { size = util.vector2(10, 0) } },
            recorder,
        },
    }
    local column = {
        type = ui.TYPE.Flex,
        content = ui.content {
            row,
            description,
        },
    }

    return column
end)

local function bindInteraction(path)
    if recording == nil then return end
    
    if path:sub(-5) == 'touch' then
        -- Before binding touch, we need a system to disambiguate touch from click otherwise
        -- it's impossible to bind click without binding touch.
        return
    end
    
    local binding = {
        path = path,
        type = recording.value.type,
        key = recording.value.key,
    }
    bindingSection:set(recording.value.id, binding)
    local refresh = recording.refresh
    recording = nil
    refresh()
end

common.registerSettingsGroup()
common.registerSettingsPage()

common.setOnInputChangedBoolean(function(path, action) 
    bindInteraction(path) 
end)

-- Current .RC does not process actions during the main menu.
-- So until a game has been loaded we have to manually deal with menu bindings.
--input.registerActionHandler('PointerLeft', function(v)
common.setManualActionCallback('PointerLeft', function(v)
    -- Should be fixed to true engine-side during main menu
    --vr._setPointerLeft(v)
    print('PointerLeft: '..tostring(v))
end)
--input.registerActionHandler('PointerRight', function(v)
common.setManualActionCallback('PointerRight', function(v)
    -- Should be fixed to true engine-side during main menu
    --vr._setPointerRight(v)
    print('PointerRight: '..tostring(v))
end)
--input.registerActionHandler('PointerActivate', function(v)
common.setManualActionCallback('PointerActivate', function(v)
    print('PointerActivate: '..tostring(v))
    vr._setPointerActivation(v)
end)

common.setManualActionCallback('MenuBack', async:callback(function(v)
    -- There isn't a catch-all solution for closing the current menu item from lua.
    -- I.UI.removeMode can only remove modes, not dialogue boxes, the console, or the postprocessing hud.
    -- From VR I need a one click solution, so i am using this placeholder internal function.
    ui._menuBack()
end))

local gameLoaded = false

local function onFrame(dt)
    -- Only want to process inputs/actions in this script during pre-game.
    if menu.getState() == menu.STATE.NoGame then
        common.onFrame(dt)
    end
    
    if not gameLoaded and menu.getState() == menu.STATE.Running then
        -- Now managed by the Player script, we can disable this callback.
        common.setManualActionCallback('PointerLeft', nil)
        common.setManualActionCallback('PointerRight', nil)
        common.setManualActionCallback('PointerActivate', nil)
        common.setManualActionCallback('MenuBack', nil)
    end
end

return {
    engineHandlers = {
        onFrame = onFrame,
    },
    
    eventHandlers = {
        RecordVRBinding = function(data) bindInteraction(data.path) end,
    },
}
