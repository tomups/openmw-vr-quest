local vr = require('openmw.vr')
local ui = require('openmw.ui')
local util = require('openmw.util')
local I = require('openmw.interfaces')
local storage = require('openmw.storage')
local input = require('openmw.input')
local core = require('openmw.core')
local async = require('openmw.async')

local alias = {}

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
    ['/user/hand/left/input/thumbstick/left'] = 'Left Stick Left',
    ['/user/hand/left/input/thumbstick/right'] = 'Left Stick Right',
    ['/user/hand/left/input/thumbstick/down'] = 'Left Stick Down',
    ['/user/hand/left/input/thumbstick/up'] = 'Left Stick Up',
    ['/user/hand/left/input/trackpad/click'] = 'Left Trackpad Click',
    ['/user/hand/left/input/trackpad/force'] = 'Left Trackpad Force',
    ['/user/hand/left/input/trackpad/touch'] = 'Left Trackpad Touch',
    ['/user/hand/left/input/trackpad/left'] = 'Left Trackpad X Left',
    ['/user/hand/left/input/trackpad/right'] = 'Left Trackpad X Right',
    ['/user/hand/left/input/trackpad/down'] = 'Left Trackpad Y Down',
    ['/user/hand/left/input/trackpad/up'] = 'Left Trackpad Y Up',
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
    ['/user/hand/right/input/thumbstick/left'] = 'Right Thumbstick X Left',
    ['/user/hand/right/input/thumbstick/right'] = 'Right Thumbstick X Right',
    ['/user/hand/right/input/thumbstick/down'] = 'Right Thumbstick Y Down',
    ['/user/hand/right/input/thumbstick/up'] = 'Right Thumbstick Y Up',
    ['/user/hand/right/input/trackpad/click'] = 'Right Trackpad Click',
    ['/user/hand/right/input/trackpad/force'] = 'Right Trackpad Force',
    ['/user/hand/right/input/trackpad/touch'] = 'Right Trackpad Touch',
    ['/user/hand/right/input/trackpad/left'] = 'Right Trackpad X Left',
    ['/user/hand/right/input/trackpad/right'] = 'Right Trackpad X Right',
    ['/user/hand/right/input/trackpad/down'] = 'Right Trackpad Y Down',
    ['/user/hand/right/input/trackpad/up'] = 'Right Trackpad Y Up',
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

local function deadzone(v)
    -- TODO: Configurable deadzone
    -- for now use a deadzone of 20% off both ends
    if (not v) or (math.abs(v) < 0.2) then
        return 0
    end
    local absv = math.abs(v)
    local sign = (v > 0) and 1 or -1
    return sign * math.min(1, (absv - 0.2) / 0.6)
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
                return deadzone(vr.getActionValue(path))
            end
        )
    elseif type == vr.INTERACTION_VALUE_TYPES.Axis then
        -- It's easier to make a binding system for each individual direction than for whole axes.
        -- So we split them apart into /x=/left,/right, and /y=/down,/up
        --registerAction(
        --    path, 
        --    input.ACTION_TYPE.Number,
        --    function() 
        --        return deadzone(vr.getActionValue(path))
        --    end
        --)
        -- For Axis actions we want the option to assigning left/right/up/down as separate Number actions
        -- allowing thumbsticks or forcepads to be used as additional assignables
        local parent = path:sub(1, -3)
        local neg = parent..'/left'
        local pos = parent..'/right'
        if path:sub(-2) == '/y' then
            neg = parent..'/down'
            pos = parent..'/up'
        end
        registerAction(
            neg,
            input.ACTION_TYPE.Number,
            function() 
                local v = deadzone(vr.getActionValue(path))
                return (v < 0) and -v or 0
            end
        )
        registerAction(
            pos,
            input.ACTION_TYPE.Number,
            function() 
                local v = deadzone(vr.getActionValue(path))
                return (v > 0) and v or 0
            end
        )
    end
end

-- Because controllers can look wildly different in terms of available interactions,
-- we need per-profile default bindings.
local defaultBindings = require('scripts.omw.vr.inputs.defaultBindings').defaultBindings

local function getDefaultBindings(id)
    local defaults = {}
    id = alias[id] or id
    -- This would be much easier/faster if the default bindings table was built in the inverse order (as in, bottom up)
    -- but that would be incredibly obnoxious to write/read/maintain so i do it this way instead.
    -- If this is an actual performance issue i'll write a function that inverts the table later...
    for profile, controllers in pairs(defaultBindings) do
        for controller, bindings in pairs(controllers) do
            if bindings[id] then
                defaults[profile] = defaults[profile] or {}
                defaults[profile][controller] = bindings[id]
            end
        end
    end
    return defaults
end

local function getActiveProfileAndControllerFromPath(path)
    local controller = controllers.RIGHT_HAND
    if string.find(path, '/left') then
        controller = controllers.LEFT_HAND
    end
    return vr.getInteractionProfileOfController(controller), controller
end

local bindingSection = storage.playerSection('OMWInputBindingsVR')

local manualTriggerCallbacks = {}
local boundActions = {}
local actionBindings = {}
local triggerBindings = {}
local activeActionBindings = {}
local activeTriggerBindings = {}

local function getOrInitBindingsForProfile(tab, profile)
    local bindings = tab[profile]
    if not bindings then
        bindings = {}
        for name, controller in pairs(controllers) do
            bindings[controller] = {}
        end
        tab[profile] = bindings
    end
    return bindings
end

local function getBindingsForController(tab, controller)
    local profile = vr.getInteractionProfileOfController(controller)
    if not profile then 
        return {} 
    end
    return getOrInitBindingsForProfile(tab, profile)[controller]
end

local function getActionBindingsForController(controller)
    return getBindingsForController(actionBindings, controller)
end

local function getTriggerBindingsForController(controller)
    return getBindingsForController(triggerBindings, controller)
end

local function setManualTriggerCallback(key, callback)
    manualTriggerCallbacks[key] = callback
end

local activeProfiles = {
}

local function updateActiveProfiles()

    -- TODO: Make an engine handler for profile changes so i don't have to do this every frame
    local changed = false
    for name, controller in pairs(controllers) do
        if activeProfiles[controller] ~= vr.getInteractionProfileOfController then
            changed = true
        end
    end
    
    if not changed then return end

    activeActionBindings = {}
    activeTriggerBindings = {}
    for name, controller in pairs(controllers) do
        local profile = vr.getInteractionProfileOfController(controller) 
        if profile then
            activeActionBindings[controller] = getActionBindingsForController(controller)
            for key, bindings in pairs(getTriggerBindingsForController(controller)) do
                for id, path in pairs(bindings) do
                    activeTriggerBindings[path] = activeTriggerBindings[path] or {}
                    activeTriggerBindings[path][id] = key
                end
            end
        end
    end
end

local function getActionBindingValueBoolean(action, v)
    for controller, bindings in pairs(activeActionBindings) do
        for id, path in pairs(bindings[action] or {}) do
            v = inputActions[path].valueBoolean or v
        end
    end
    return v
end

local function getActionBindingValueNumber(action, v)
    for _, bindings in pairs(activeActionBindings) do
        for _, path in pairs(bindings[action] or {}) do
            local v2 = inputActions[path].valueNumber or 0
            if math.abs(v2) > math.abs(v) then
                v = v2
            end
        end
    end
    return v
end

-- Until actions work during the main menu, we have to manually fire some actions
local function tryManualTriggers(path, value)
    local once = {}
    
    for id, key in pairs(activeTriggerBindings[path] or {}) do            
        if manualTriggerCallbacks[key] and not once[key] then
            manualTriggerCallbacks[key](value)
            once[key] = true
        end
    end
end

local function setupValueBinding(action, required, alias)
    if not boundActions[action] then
        boundActions[action] = true
        if not input.actions[action] then
            -- Some of upstream's actions/triggers aren't registered until a game is loaded
            -- so we have to delay setup until the game IS loaded
        else
            if input.actions[action].type == input.ACTION_TYPE.Boolean then
                input.bindAction(action, async:callback(function(dt, v)
                    -- If this is an aliased action, that implies the value v is from an existing built-in action so we want to keep
                    -- the existing value (v), otherwise (v) will be last frame's value and we should discard it.
                    return getActionBindingValueBoolean(alias or action, alias and v or false)
                end), {})
            else
                input.bindAction(action, async:callback(function(dt, v)
                    return getActionBindingValueNumber(alias or action, alias and v or 0)
                end), {})
            end
        end
    end
end


local function bindAction(binding, id)
    local action = binding.key
    for profile, controllers in pairs(binding.paths or {}) do
        for controller, path in pairs(controllers or {}) do
            local tab = getOrInitBindingsForProfile(actionBindings, profile)[controller]
            tab[action] = tab[action] or {}
            tab[action][id] = path
        end
    end
    setupValueBinding(action)
end

local function bindTrigger(binding, id)
    local trigger = binding.key
    for profile, controllers in pairs(binding.paths or {}) do
        for controller, path in pairs(controllers or {}) do
            local tab = getOrInitBindingsForProfile(triggerBindings, profile)[controller]
            tab[trigger] = tab[trigger] or {}
            tab[trigger][id] = path
        end
    end
end

local function clearBindingForTable(tab, id)
    for profile, controllers in pairs(tab) do
        for controller, actionBindings in pairs(controllers) do
            for _, bindings in pairs(actionBindings) do
                bindings[id] = nil
            end
        end
    end
end

local function clearBinding(id)
    clearBindingForTable(actionBindings, id)
    clearBindingForTable(triggerBindings, id)
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
    if binding ~= nil and binding.paths then
        print('registering binding for id = '..id)
        registerBinding(binding, id)
    end
    return id
end))

local settings = {
    }
    
local function bindSetting(key, type, default, required)
    if not default then print('error default nil') end
    settings[#settings+1] = {
        key = key..'Key',
        renderer = 'inputBindingVR',
        name = key..'Key',
        description = key .. 'KeyDescription',
        default = {
            id = key..'_VR',
            key = key,
            paths = default,
            type = type,
            required = required
        },
    }
end


local function registerTriggers()
    local reg = function(key, required)
        if not input.triggers[key] then
            input.registerTrigger {
                key = key,
                l10n = 'OMWControlsVR',
                name = key .. '_name',
                description = key .. '_description',
            }
            bindSetting(key, 'trigger', getDefaultBindings(key..'_VR'), required)
        end
    end

    reg('PointerActivate', true)
    reg('Recenter')
    reg('MetaMenu', true)
    --reg('MenuSelect')
    reg('MenuBack')
    
    local regExisting = function(key, required)
        bindSetting(key, 'trigger', getDefaultBindings(key..'_VR'), required)
    end
    regExisting('ToggleSneak')
    regExisting('ToggleWeapon')
    regExisting('ToggleSpell')
end
local function registerActions()
    local reg = function(key, type, value, required)
        if not input.actions[key] then
            input.registerAction {
                key = key,
                type = type,
                l10n = 'OMWControlsVR',
                name = key..'_Name',
                description = key..'_Description',
                defaultValue = value,
            }
        end
        bindSetting(key, 'action', getDefaultBindings(key..'_VR'), required)
        -- Make sure our bindings are always fired first by setting up the binding right away
        setupValueBinding(key, required)
    end
    local regBool = function(key, required)
        reg(key, input.ACTION_TYPE.Boolean, false, required)
    end
    local regRange = function(key, required)
        reg(key, input.ACTION_TYPE.Range, 0, required)
    end
    regBool('PointerLeft')
    regBool('PointerRight')
    regRange('LookLeft')
    regRange('LookRight')
    
    -- Existing actions.
    -- The intent is to only add bindings for these,
    -- but the upstream scripts don't define them until a game is loaded, which doesn't work for me.
    local regExisting = function(f, key, required)
        local key2 = key..'_Alias'
        alias[key2..'_VR'] = key..'_VR'
        f(key2, required)
        --bindSetting(key, 'action', getDefaultBindings(key..'_VR'), required)
        setupValueBinding(key, required, key2)
    end
    local regExistingBool = function(key, required)
        regExisting(regBool, key, required)
    end
    local regExistingRange = function(key, required)
        regExisting(regRange, key, required)
    end
    -- For now, registering existing doesn't work
    regExistingRange('MoveLeft')
    regExistingRange('MoveRight')
    regExistingRange('MoveForward')
    regExistingRange('MoveBackward')
    regExistingBool('Sneak')
end

-- The current .49 RC has a bug where actions defined during main menu won't transfer to in-game
-- So we re-register actions when loading the player script but only if they don't already exist.
--if not input.actions['PointerLeft'] then
    registerActions()
    registerTriggers()
--end
    
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
            if binding.paths then
                registerBinding(binding, id)
            end
        end
    end
end

local onInputChangedBoolean = nil

local function inputChangedBoolean(path, action)
    if action.valueBoolean then
        for id, key in pairs (activeTriggerBindings[path] or {}) do
            input.activateTrigger(key)
        end
        tryManualTriggers(path, action.valueBoolean)
    end
    
    if onInputChangedBoolean then
        onInputChangedBoolean(path, action)
    end
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
    updateActiveProfiles()
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
    setManualTriggerCallback = setManualTriggerCallback,
    controllers = controllers,
    getActiveProfileAndControllerFromPath = getActiveProfileAndControllerFromPath,
}
