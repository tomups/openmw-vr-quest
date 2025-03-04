local vr = require('openmw.vr')
local ui = require('openmw.ui')
local util = require('openmw.util')
local I = require('openmw.interfaces')
local storage = require('openmw.storage')
local input = require('openmw.input')
local core = require('openmw.core')
local async = require('openmw.async')


local controllers = {
    LEFT_HAND = '/user/hand/left',
    RIGHT_HAND = '/user/hand/right',
}

local interactionNames = {
    ['/user/hand/left/input/a/click'] = 'Left A Click',
    ['/user/hand/left/input/a/touch'] = 'Left A Touch',
    ['/user/hand/left/input/b/click'] = 'Left B Click',
    ['/user/hand/left/input/b/touch'] = 'Left B Touch',
    ['/user/hand/left/input/back/click'] = 'Left Back Click',
    ['/user/hand/left/input/menu/click'] = 'Left Menu Click',
    ['/user/hand/left/input/select/click'] = 'Left Select Click',
    ['/user/hand/left/input/squeeze/click'] = 'Left Squeeze Click',
    ['/user/hand/left/input/squeeze/force'] = 'Left Squeeze Force',
    ['/user/hand/left/input/squeeze/value'] = 'Left Squeeze Value',
    ['/user/hand/left/input/thumbrest/touch'] = 'Left Thumbrest Touch',
    ['/user/hand/left/input/thumbstick/click'] = 'Left Stick Click',
    ['/user/hand/left/input/thumbstick/touch'] = 'Left Stick Touch',
    ['/user/hand/left/input/thumbstick/x'] = 'Left Stick X',
    ['/user/hand/left/input/thumbstick/x/left'] = 'Left Stick Left',
    ['/user/hand/left/input/thumbstick/x/right'] = 'Left Stick Right',
    ['/user/hand/left/input/thumbstick/y'] = 'Left Stick Y',
    ['/user/hand/left/input/thumbstick/y/down'] = 'Left Stick Down',
    ['/user/hand/left/input/thumbstick/y/up'] = 'Left Stick Up',
    ['/user/hand/left/input/trackpad/click'] = 'Left Trackpad Click',
    ['/user/hand/left/input/trackpad/force'] = 'Left Trackpad Force',
    ['/user/hand/left/input/trackpad/touch'] = 'Left Trackpad Touch',
    ['/user/hand/left/input/trackpad/x'] = 'Left Trackpad X',
    ['/user/hand/left/input/trackpad/x/left'] = 'Left Trackpad X Left',
    ['/user/hand/left/input/trackpad/x/right'] = 'Left Trackpad X Right',
    ['/user/hand/left/input/trackpad/y'] = 'Left Trackpad Y',
    ['/user/hand/left/input/trackpad/y/down'] = 'Left Trackpad Y Down',
    ['/user/hand/left/input/trackpad/y/up'] = 'Left Trackpad Y Up',
    ['/user/hand/left/input/trigger/click'] = 'Left Trigger Touch',
    ['/user/hand/left/input/trigger/touch'] = 'Left Trigger Touch',
    ['/user/hand/left/input/trigger/value'] = 'Left Trigger Value',
    ['/user/hand/left/input/x/click'] = 'Left X Click',
    ['/user/hand/left/input/x/touch'] = 'Left X Touch',
    ['/user/hand/left/input/y/click'] = 'Left Y Click',
    ['/user/hand/left/input/y/touch'] = 'Left Y Touch',
    ['/user/hand/right/input/a/click'] = 'Right A Click',
    ['/user/hand/right/input/a/touch'] = 'Right A Touch',
    ['/user/hand/right/input/b/click'] = 'Right B Click',
    ['/user/hand/right/input/b/touch'] = 'Right B Touch',
    ['/user/hand/right/input/back/click'] = 'Right Back Click',
    ['/user/hand/right/input/menu/click'] = 'Right Menu Click',
    ['/user/hand/right/input/select/click'] = 'Right Select Click',
    ['/user/hand/right/input/squeeze/click'] = 'Right Squeeze Click',
    ['/user/hand/right/input/squeeze/force'] = 'Right Squeeze Force',
    ['/user/hand/right/input/squeeze/value'] = 'Right Squeeze Value',
    ['/user/hand/right/input/thumbrest/touch'] = 'Right Thumbrest Touch',
    ['/user/hand/right/input/thumbstick/click'] = 'Right Thumbstick Click',
    ['/user/hand/right/input/thumbstick/touch'] = 'Right Thumbstick Touch',
    ['/user/hand/right/input/thumbstick/x'] = 'Right Thumbstick X',
    ['/user/hand/right/input/thumbstick/x/left'] = 'Right Thumbstick X Left',
    ['/user/hand/right/input/thumbstick/x/right'] = 'Right Thumbstick X Right',
    ['/user/hand/right/input/thumbstick/y'] = 'Right Thumbstick Y',
    ['/user/hand/right/input/thumbstick/y/down'] = 'Right Thumbstick Y Down',
    ['/user/hand/right/input/thumbstick/y/up'] = 'Right Thumbstick Y Up',
    ['/user/hand/right/input/trackpad/click'] = 'Right Trackpad Click',
    ['/user/hand/right/input/trackpad/force'] = 'Right Trackpad Force',
    ['/user/hand/right/input/trackpad/touch'] = 'Right Trackpad Touch',
    ['/user/hand/right/input/trackpad/x'] = 'Right Trackpad X',
    ['/user/hand/right/input/trackpad/x/left'] = 'Right Trackpad X Left',
    ['/user/hand/right/input/trackpad/x/right'] = 'Right Trackpad X Right',
    ['/user/hand/right/input/trackpad/y'] = 'Right Trackpad Y',
    ['/user/hand/right/input/trackpad/y/down'] = 'Right Trackpad Y Down',
    ['/user/hand/right/input/trackpad/y/up'] = 'Right Trackpad Y Up',
    ['/user/hand/right/input/trigger/click'] = 'Right Trigger Click',
    ['/user/hand/right/input/trigger/touch'] = 'Right Trigger Touch',
    ['/user/hand/right/input/trigger/value'] = 'Right Trigger Value',
}

local supportedInteractionPaths = {
    input = {},
    output = {},
}

for profile, controllers in pairs(vr.availableInteractions) do
    for controller, interactions in pairs(controllers) do
        for interaction, interactionType in pairs(interactions) do
            local path = controller..interaction
            if string.find(interaction, 'input') then
                supportedInteractionPaths.input[path] = interactionType
            else
                supportedInteractionPaths.output[path] = interactionType
            end
        end
    end
end

local vrActions = {
    use = vr.INTERACTION_VALUE_TYPES.Boolean,
    menu_back = vr.INTERACTION_VALUE_TYPES.Boolean,
    -- todo

}


local inputActions = {
}

local function registerAction(path, type, func)
    -- I wanted to use openmw.input's action system for this
    -- but it doesn't work during the main menu.
    
    --input.registerAction {
    --    key = path,
    --    type = type,
    --    l10n = 'VRInteractionPathLocalization',
    --    name = path,
    --    description = 'A VR interaction path',
    --    defaultValue = default,
    --}
    --input.bindAction(path, async:callback(func), {})
    
    -- Instead I'm rolling my own
    inputActions[path] = {
        type = type,
        func = func,
        valueBoolean = false,
        valueNumber = 0,
    }
end

-- Register an action for every input
for path, type in pairs(supportedInteractionPaths.input) do
    if type == vr.INTERACTION_VALUE_TYPES.Boolean then
        registerAction(
            path, 
            input.ACTION_TYPE.Boolean,
            function() 
                return vr.getActionValue(path) == true 
            end
        )
    elseif type == vr.INTERACTION_VALUE_TYPES.Float then
        registerAction(
            path, 
            input.ACTION_TYPE.Number,
            function() 
                local v = vr.getActionValue(path) 
                if not v then return 0 end
                return v
            end
        )
    elseif type == vr.INTERACTION_VALUE_TYPES.Axis then
        registerAction(
            path, 
            input.ACTION_TYPE.Number,
            function() 
                local v = vr.getActionValue(path) 
                if not v then return 0 end
                return v
            end
        )
        -- For Axis actions we want the option to assigning left/right/up/down as separate Number actions
        -- allowing thumbsticks or forcepads to be used as additional assignables
        local neg = '/left'
        local pos = '/right'
        if path:sub(-2) == '/y' then
            neg = '/down'
            pos = '/up'
        end
        registerAction(
            path..neg,
            input.ACTION_TYPE.Number,
            function() 
                local v = vr.getActionValue(path) 
                if not v or v > 0 then return 0 end
                return -v
            end
        )
        registerAction(
            path..pos,
            input.ACTION_TYPE.Number,
            function() 
                local v = vr.getActionValue(path) 
                if not v or v < 0 then return 0 end
                return v
            end
        )
    end
end


local function invalidBinding(binding)
    if not binding.key then
        return 'has no key'
    elseif binding.type ~= 'action' and binding.type ~= 'trigger' then
        return string.format('has invalid type', binding.type)
    elseif binding.type == 'action' and not input.actions[binding.key] then
        return string.format("action %s doesn't exist", binding.key)
    elseif binding.type == 'trigger' and not input.triggers[binding.key] then
        return string.format("trigger %s doesn't exist", binding.key)
    end
end

local bindingSection = storage.playerSection('OMWInputBindingsVR')
local manualActionCallbacks = {}
local actionBindings = {}
local boundActions = {}
local triggerBindings = {}

local function setManualActionCallback(key, callback)
    manualActionCallbacks[key] = callback
end

-- Until actions work during the main menu, we have to manually fire some actions
-- This is TEMPORARY
local function tryManualActions(path, action)
    for key, cb in pairs(manualActionCallbacks) do
        local once = false
        for id, binding in pairs(actionBindings[key] or {}) do
            if binding.path == path and not once then
                cb(action.valueBoolean)
                once = true
            end
        end
    end
end

local function setupValueBinding(action)
    if not boundActions[action] then
        boundActions[action] = true
        if input.actions[action].type == input.ACTION_TYPE.Boolean then
            input.bindAction(action, async:callback(function(dt)
                local v = false
                for _, binding in pairs(actionBindings[action] or {}) do
                    v = inputActions[binding.path].valueBoolean or v
                end
                return v
            end), {})
        else
            input.bindAction(action, async:callback(function(dt)
                local v = 0
                for _, binding in pairs(actionBindings[action] or {}) do
                    local v2 = inputActions[binding.path].valueNumber
                    v = math.max(math.abs(v or 0), math.abs(v2 or 0))
                end
                return v
            end), {})
        end
    end
end

local function bindAction(binding, id)
    local action = binding.key
    actionBindings[action] = actionBindings[action] or {}
    actionBindings[action][id] = binding
    setupValueBinding(action)
end

local function bindTrigger(binding, id)
    triggerBindings[binding.path] = triggerBindings[binding.path] or {}
    triggerBindings[binding.path][id] = binding
end

local function clearBinding(id)
    actionBindings[id] = nil
    triggerBindings[id] = nil
end

local function registerBinding(binding, id)
    local invalid = invalidBinding(binding)
    if invalid then
        print(string.format('Skipping invalid binding %s: %s', id, invalid))
    elseif binding.type == 'action' then
        bindAction(binding, id)
    elseif binding.type == 'trigger' then
        bindTrigger(binding, id)
    end
end

bindingSection:subscribe(async:callback(function(_, id)
    if not id then return end
    if id == 'version' then return end
    local binding = bindingSection:get(id)
    clearBinding(id)
    if binding ~= nil and binding.path then
        registerBinding(binding, id)
    end
    return id
end))

local function registerTriggers()
end

local function bindSetting(key, type, default, long)
    return {
        key = key..'Key',
        renderer = 'inputBindingVR',
        name = key..'Key',
        description = key .. 'KeyDescription',
        default = {
            id = key..'_VR',
            key = key,
            path = default,
            type = type,
            long = long,
        },
    }
end

local settings = {
    }

local function registerActions()
    local reg = function(key, default, long, type)
        input.registerAction {
            key = key,
            type = type,
            l10n = 'OMWControlsVR',
            name = key..'_Name',
            description = key..'_Description',
            defaultValue = false,
        }
        settings[#settings + 1] = bindSetting(key, 'action', default)
        -- Make sure our bindings are always fired first by setting up the binding right away
        setupValueBinding(key)
    end
    local regBool = function(key, default, long)
        reg(key, default, long, input.ACTION_TYPE.Boolean)
    end
    local regFloat = function(key, default, long)
        reg(key, default, long, input.ACTION_TYPE.Number)
    end
    regBool('PointerLeft', '/user/hand/left/input/squeeze/value')
    regBool('PointerRight', '/user/hand/right/input/squeeze/value')
    regBool('PointerActivate', '/user/hand/right/input/a/click')
    regBool('Recenter', '/user/hand/left/input/menu/click', true)
    regBool('MetaMenu', '/user/hand/left/input/menu/click')
    --regBool('MenuSelect', '/user/hand/right/input/b/click')
    regBool('MenuBack', '/user/hand/left/input/menu/click')
end

-- The current .49 RC has a bug where actions defined during main menu won't transfer to in-game
-- So we re-register actions when loading the player script but only if they don't already exist.
if not input.actions['PointerActivate'] then
    registerActions()
    registerTriggers()
end
    
local settingsGroup = 'SettingsOMWControlsVR'

local function registerSettingsPage()
    I.Settings.registerPage({
        key = 'OMWControlsVR',
        l10n = 'OMWControlsVR',
        name = 'ControlsPage',
        description = 'ControlsPageDescription',
    })
end

local function registerSettingsGroup()
    I.Settings.registerGroup({
        key = settingsGroup,
        page = 'OMWControlsVR',
        l10n = 'OMWControlsVR',
        name = 'MovementSettingsVR',
        permanentStorage = true,
        settings = settings,
    })
end

local initialized = false

local function init()
    -- Set up stored bindings from the bindingSection.
    for id, binding in pairs(bindingSection:asTable()) do
        if id ~= 'version' then
            clearBinding(id)
            if binding.path then
                registerBinding(binding, id)
            end
        end
    end
end

local onInputChangedBoolean = nil

local function inputChangedBoolean(path, action)
    if action.valueBoolean then
        for _, binding in pairs (triggerBindings[path] or {}) do
            input.activateTrigger(binding.key)
        end
    end
    
    if onInputChangedBoolean then
        onInputChangedBoolean(path, action)
    end
    tryManualActions(path, action)
end

local function updateInputs()
    for path, action in pairs(inputActions) do
        local prevBoolean = action.valueBoolean
        value = action.func()
        if type(value) == 'number' then
            action.valueNumber = value
            -- When converting a float input to bool, we want to transition from false to true and back at different values,
            -- to avoid rapid change-spam when input near 0.5
            action.valueBoolean = prevBoolean 
                and value > 0.4
                or value > 0.6
        else
            action.valueNumber = value and 1 or 0
            action.valueBoolean = value
        end
        if action.valueBoolean ~= prevBoolean then
            inputChangedBoolean(path, action)
        end
    end
end

local function getInteractionName(interactionPath)
    if interactionNames[interactionPath] then
        return interactionNames[interactionPath]
    end
    return interactionPath
end

local function onFrame(dt)
    if not initialized then 
        init() 
        initialized = true
    end
    updateInputs()
end

return {
    updateInputs = updateInputs,
    onFrame = onFrame,
    settings = settings,
    registerSettingsGroup = registerSettingsGroup,
    registerSettingsPage = registerSettingsPage,
    setOnInputChangedBoolean = function(func) onInputChangedBoolean = func end,
    interactionNames = interactionNames,
    getInteractionName = getInteractionName,
    setManualActionCallback = setManualActionCallback,
    controllers = controllers,
}
