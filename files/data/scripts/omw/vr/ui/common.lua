---
-- vr ui
-- @module vrui
--

--- Configuration of a VR 3D GUI quad. Informs the engine where to place a layer of the GUI within the game world, and at what size.
-- @type GuiConfig
-- @field #number backgroundOpacity Default: 0
-- @field openmw.util#Vector2 center Defines where within a layer is considered the center. x and y values range from -0.5 to 0.5. The geometry will be positioned with its center at the assigned coordinate. This most importantly affects the direction auto-sized layers will grow. For example, the HUD has a default center of util.vector2(0, 0.5),  causing it to grow left as new magic effects are added, keeping all elements in place.
-- @field openmw.util#Vector2 extent Defines the spatial size of a layer in meters. Is ignored for auto-sized layers.
-- @field #number pixelsPerMeter Defines the spatial size of auto-sized layers. Is ignored for non-auto-sized layers.
-- @field #boolean autosize Defines whether this layer should be auto-sized or not
-- @field #string space (Optional) name of a space to actively track. If set, the UI element will actively track this space, updating its pose every frame.
--

--- A spatial pose, consisting of a position and an orientation
-- @type Pose
-- @field openmw.util#Vector3 position @{openmw.util#Vector3}
-- @field openmw.util#Transform orientation @{openmw.util#Transform}
--

-- For some reason, i cannot inline documentation statements with the corresponding function. The documentation build parents everything to table if i do...
-- This doesn't happen for any of the other interfaces...

--- Interface version
-- @field [parent=#vrui] #number version

--- Set Config for the given layer
-- @function [parent=#vrui] setLayerConfig
-- @param #GuiConfig config

--- Set pose for the given layer
-- @function [parent=#vrui] setLayerPose
-- @param #Pose pose

--- Override configs for a given layer. Stops this script from processing that layer, allowing you to change it however you wish without this script interfering.
-- @function [parent=#vrui] overrideLayerConfig
-- @param #string layer
-- @param #boolean override Set to true to override config. Set to false to stop overriding.

local async = require('openmw.async')
local core = require('openmw.core')
local I = require('openmw.interfaces')
local storage = require('openmw.storage')
local ui = require('openmw.ui')
local util = require('openmw.util')
local vr = require('openmw.vr')

local windowToLayer = {
    Alchemy = 'Windows',
    Book = 'JournalBooks',
    Companion = 'InventoryCompanionWindow',
    Container = 'InventoryCompanionWindow',
    Dialogue = 'DialogueWindow',
    EnchantingDialog = 'Windows',
    Inventory = 'InventoryWindow',
    JailScreen = 'Windows',
    Journal = 'JournalBooks',
    LevelUpDialog = 'Windows',
    Magic = 'SpellWindow',
    Map = 'MapWindow',
    MerchantRepair = 'Windows',
    QuickKeys = 'Windows',
    Recharge = 'Windows',
    Repair = 'Windows',
    Scroll = 'JournalBooks',
    SpellBuying = 'Windows',
    SpellCreationDialog = 'Windows',
    Stats = 'StatsWindow',
    Trade = 'InventoryCompanionWindow',
    Training = 'Windows',
    Travel = 'Windows',
    WaitDialog = 'Windows',
}

--- List of layer that need to be arranged when multiple windows are opened
--- In order of priority (left-to-right)
local layersForArrangement = {
    'DialogueWindow', 'MapWindow', 'SpellWindow', 'InventoryWindow', 'StatsWindow', 'InventoryCompanionWindow', 'Windows'
}

local function getWindowLayer(window)
    return windowToLayer[window] or 'Windows'
end

local function getAllLayers()
    local layers = {}
    for k, v in pairs(ui.layers) do
        layers[#layers + 1] = v.name
    end

    return layers
end

local function getLayerSize(layer)
    return ui.layers[ui.layers.indexOf(layer)].size
end

local function createDerivedSpaces()
    I.vrspaces.createDerivedSpace(
        'DefaultWindow',
        I.vrspaces.referenceSpaces.View,
        {
            position = util.vector3(0, 1, -0.25) * I.vrspaces.unitsPerMeter,
            orientation = util.transform.identity,
        }
    )
    I.vrspaces.createDerivedSpace(
        'DialogueWindow',
        I.vrspaces.referenceSpaces.View,
        {
            position = util.vector3(0, 1, -0.45) * I.vrspaces.unitsPerMeter,
            orientation = util.transform.identity,
        }
    )

    I.vrspaces.createDerivedSpace(
        'DefaultNotification',
        I.vrspaces.referenceSpaces.View,
        {
            position = util.vector3(0, 0.8, -0.25) * I.vrspaces.unitsPerMeter,
            orientation = util.transform.identity,
        }
    )

    I.vrspaces.createDerivedSpace(
        'DefaultVirtualKeyboard',
        I.vrspaces.referenceSpaces.View,
        {
            position = util.vector3(0, 35, -40),
            orientation = util.transform.identity,
        }
    )

    I.vrspaces.createDerivedSpace(
        'LeftWristInner',
        I.vrspaces.actionSpaces.LeftHandAim,
        {
            position = util.vector3(0.09, -0.200, -0.033) * I.vrspaces.unitsPerMeter,
            orientation = util.transform.rotate(math.pi/2, util.vector3(0, 0, 1))
        }
    )

    I.vrspaces.createDerivedSpace(
        'LeftWristTop',
        I.vrspaces.actionSpaces.LeftHandAim,
        {
            position = util.vector3(0.0, -0.200, 0.066) * I.vrspaces.unitsPerMeter
        }
    )

    I.vrspaces.createDerivedSpace(
        'RightWristInner',
        I.vrspaces.actionSpaces.RightHandAim,
        {
            position = util.vector3(-0.09, -0.200, -0.033) * I.vrspaces.unitsPerMeter,
            orientation = util.transform.rotate(-math.pi/2, util.vector3(0, 0, 1))
        }
    )

    I.vrspaces.createDerivedSpace(
        'RightWristTop',
        I.vrspaces.actionSpaces.RightHandAim,
        {
            position = util.vector3(0.0, -0.200, 0.066) * I.vrspaces.unitsPerMeter
        }
    )

    I.vrspaces.createDerivedSpace(
        'HUDTopLeft',
        I.vrspaces.referenceSpaces.View,
        {
            position = util.vector3(-12, 30, 6)
        }
    )

    I.vrspaces.createDerivedSpace(
        'HUDTopRight',
        I.vrspaces.referenceSpaces.View,
        {
            position = util.vector3(12, 30, 6)
        }
    )

    I.vrspaces.createDerivedSpace(
        'HUDBottomLeft',
        I.vrspaces.referenceSpaces.View,
        {
            position = util.vector3(-12, 30, -18)
        }
    )

    I.vrspaces.createDerivedSpace(
        'HUDBottomRight',
        I.vrspaces.referenceSpaces.View,
        {
            position = util.vector3(12, 30, -18)
        }
    )

    I.vrspaces.createDerivedSpace(
        'HUDMessage',
        I.vrspaces.referenceSpaces.View,
        {
            position = util.vector3(0, 30, -3)
        }
    )
end

local HUDpaces = {
    'LeftWristInner',
    'LeftWristTop',
    'RightWristInner',
    'RightWristTop',
    'HUDTopLeft',
    'HUDTopRight',
    'HUDBottomLeft',
    'HUDBottomRight',
}

-- Wrist spaces do not work in KBM mouse
local WristSpaceAliases = {
    LeftWristInner = 'HUDTopLeft',
    LeftWristTop = 'HUDTopLeft',
    RightWristInner = 'HUDTopRight',
    RightWristTop = 'HUDTopRight',
    HUDTopLeft = 'HUDTopLeft',
    HUDTopRight = 'HUDTopRight',
    HUDBottomLeft = 'HUDBottomLeft',
    HUDBottomRight = 'HUDBottomRight',
}

local function createDefaultConfig(backgroundOpacity, autosize)
    return {
        backgroundOpacity = backgroundOpacity,
        center = util.vector2(0,0),
        extent = util.vector2(1,1),
        pixelsPerMeter = 1024,
        pixelResolution = util.vector2(1024, 1024),
        autosize = autosize,
    }
end

local layerConfigOverridden = {}

local spaceForMode = {
    Default = 'DefaultWindow',
    Dialogue = 'DialogueWindow',
}

local function getWindowSpaceForCurrentMode()
    if not I.UI then
        return 'DefaultWindow'
    end
    local mode = I.UI.getMode()
    return spaceForMode[mode] or 'DefaultWindow'
end

local layerConfig = {}

local settingsPageKey = 'OMWVRUI'
local l10nKey = settingsPageKey
local uiGroupKey = 'UiGroup'
local uiSection = storage.playerSection(uiGroupKey)
local spacesGroupKey = 'SpacesGroup'
local spacesSection = storage.playerSection(spacesGroupKey)

local function registerSettingsPage()
    I.Settings.registerPage({
        key = settingsPageKey,
        l10n = l10nKey,
        name = 'UiPage',
        description = 'UiPageDescription',
    })
end

local function boolSetting(key, default, argument)
    return {
        key = key,
        renderer = 'checkbox',
        name = key,
        description = key .. 'Description',
        default = default,
        argument = argument,
    }
end

local function floatSetting(key, default, argument)
    return {
        key = key,
        renderer = 'number',
        name = key,
        description = key .. 'Description',
        default = default,
        argument = argument,
    }
end

local function selectSetting(key, items, default)
    if not default then default = items[1] end
    return {
        key = key,
        renderer = 'select',
        name = key,
        description = key .. 'Description',
        default = default,
        argument = {
            l10n = l10nKey,
            items = items
        }
    }
end

local function registerSettingsGroup()
    I.Settings.registerGroup({
        key = spacesGroupKey,
        page = settingsPageKey,
        l10n = l10nKey,
        name = spacesGroupKey,
        permanentStorage = true,
        settings = {
            selectSetting('HUDSpace', HUDpaces),
            selectSetting('TooltipSpace', HUDpaces),
            boolSetting('DialogueSpace', true),
        },
    })
    I.Settings.registerGroup({
        key = uiGroupKey,
        page = settingsPageKey,
        l10n = l10nKey,
        name = uiGroupKey,
        permanentStorage = true,
        settings = {
            boolSetting('ShowTutorials', true),
        },
    })
end

local function setLayerConfigIfNotOverridden(layer, config)
    if not layerConfigOverridden[layer] then
        vr._setLayerConfig(layer, config)
    end
end

local function setLayerPoseIfNotOverridden(layer, pose)
    if not layerConfigOverridden[layer] then
        vr._setLayerPose(layer, pose)
    end
end

local configHUD3D = createDefaultConfig(0, true)
configHUD3D.extent = util.vector2(0.033, 0.033)
configHUD3D.center = util.vector2(0, 0.5)
configHUD3D.space = 'LeftWristTop'
layerConfig.HUD_3D = configHUD3D

local configTooltip = createDefaultConfig(0, true)
configTooltip.extent = util.vector2(0.033, 0.033)
configTooltip.space = 'RightWristTop'
layerConfig.Tooltip = configTooltip

local function updateSpacesSettings()
    configHUD3D.space = spacesSection:get('HUDSpace')
    configTooltip.space = spacesSection:get('TooltipSpace')
    local dialogueSpace = spacesSection:get('DialogueSpace')
    if dialogueSpace then
        spaceForMode.Dialogue = 'DialogueWindow'
    else
        spaceForMode.Dialogue = nil
    end
    
    -- Wrist spaces do not work in KBM mouse
    if I.vrinputs and I.vrinputs.isKBMouseMode() then
        configHUD3D.space = WristSpaceAliases[configHUD3D.space]
        configTooltip.space = WristSpaceAliases[configTooltip.space]
    end
    
    setLayerConfigIfNotOverridden('HUD_3D', configHUD3D)
    setLayerConfigIfNotOverridden('Tooltip', configTooltip)
end
spacesSection:subscribe(async:callback(updateSpacesSettings))


local function setupDefaults(modes)
    updateSpacesSettings()

    local allLayers = getAllLayers()
    for _, layer in pairs(allLayers) do
        if not layerConfig[layer] then
            layerConfig[layer] = createDefaultConfig(0.7, true)
        end
    end

    local bookLayer = layerConfig[getWindowLayer('Book')]
    if bookLayer then
        bookLayer.backgroundOpacity = 0
    end
    local journalLayer = layerConfig[getWindowLayer('Journal')]
    if journalLayer then
        journalLayer.backgroundOpacity = 0
    end
    local scrollLayer = layerConfig[getWindowLayer('Scroll')]
    if scrollLayer then
        scrollLayer.backgroundOpacity = 0
    end

    layerConfig.VirtualKeyboard.extent = util.vector2(0.25, 0.25)
    layerConfig.Notification = createDefaultConfig(0, false)
    layerConfig.Notification.space = 'DefaultNotification'
    layerConfig.HUD = createDefaultConfig(0, true)
    
    for layer, config in pairs(layerConfig) do
        setLayerConfigIfNotOverridden(layer, config)
    end
end

local function makeUpright(orientation)
    return util.transform.rotateZ(orientation:getYaw())
end

local referenceWorldPose = {
    position = util.vector3(0,0,0),
    orientation = util.transform.identity,
}

local windowArrangementRelativePose = {}
local function updateWindowRelativePoses()
    windowArrangementRelativePose = I.vrspaces.locateSpace(getWindowSpaceForCurrentMode(), I.vrspaces.referenceSpaces.View) or windowArrangementRelativePose
end

local virtualKeyboardRelativePose = {
    position = util.vector3(0, 35, -40),
    orientation = util.transform.rotateX(math.pi/8),
}

local visibleLayersForArrangement = {}

local function updateLayerArrangement()
    if #visibleLayersForArrangement == 0 then
        return
    end

    local layerPoses = {}
    local step = (-math.pi / 4)
    local span = step * (#visibleLayersForArrangement - 1)
    local angle = -span / 2 + referenceWorldPose.orientation:getYaw()
    for _, layer in ipairs(visibleLayersForArrangement) do
        local transform = util.transform.rotateZ(angle) * util.transform.rotateZ(windowArrangementRelativePose.orientation:getYaw())
        local rotatedPosition = transform:apply(windowArrangementRelativePose.position)

        setLayerPoseIfNotOverridden(layer, {
            position = referenceWorldPose.position + rotatedPosition,
            orientation = transform
        })

        angle = angle + step
    end
end

local function updateVisibleLayers()
    local old = visibleLayersForArrangement
    visibleLayersForArrangement = {}

    for _, layer in ipairs(layersForArrangement) do
        if vr._isLayerRendering(layer) then
            visibleLayersForArrangement[#visibleLayersForArrangement + 1] = layer
        end
    end

    if #old ~= #visibleLayersForArrangement then
        return true
    end
    for i, layer in ipairs(old) do
        if visibleLayersForArrangement[i] ~= layer then
            return true
        end
    end
    return false
end

local function computeLayerPose()
    local transform = util.transform.rotateZ(referenceWorldPose.orientation:getYaw() + windowArrangementRelativePose.orientation:getYaw())
    return {
        position = referenceWorldPose.position + transform:apply(windowArrangementRelativePose.position),
        orientation = transform
    }
end

local function computeVirtualKeyboardPose()
    local orientation = util.transform.rotateX(math.pi/8)
    local transform = util.transform.rotateZ(referenceWorldPose.orientation:getYaw())
    return {
        position = referenceWorldPose.position + transform:apply(virtualKeyboardRelativePose.position),
        orientation = transform * virtualKeyboardRelativePose.orientation
    }
end

local function updateLayers()
    -- Update all the modes that did not get updated by arrangement.
    local pose = computeLayerPose()
    for _, layer in pairs(getAllLayers()) do
        if not layerConfig[layer].space then
            setLayerPoseIfNotOverridden(layer, pose)
        end
    end
    setLayerPoseIfNotOverridden('VirtualKeyboard', computeVirtualKeyboardPose())
end

local function updatePoses()
    referenceWorldPose = I.vrspaces.locateSpaceInWorld(I.vrspaces.referenceSpaces.View) or referenceWorldPose
    updateWindowRelativePoses()
end

local function overrideLayerConfig(layer, v)
    layerConfigOverridden[layer] = v
end

local interface =
{
    version = 0,

    overrideLayerConfig = overrideLayerConfig,
    
    showMessageInTheVoid = function(message) vr._showMessageInTheVoid(message) end,
    
    setLayerConfig = function(layer, config) vr._setLayerConfig(layer, config) end,
    
    setLayerPose = function(layer, pose) vr._setLayerPose(layer, pose) end,
    
    getWindowLayer = getWindowLayer,
}

return {
    createDefaultConfig = createDefaultConfig,
    createDerivedSpaces = createDerivedSpaces,
    setupDefaults = setupDefaults,
    makeUpright = makeUpright,
    updatePoses = updatePoses,
    updateLayers = updateLayers,
    updateVisibleLayers = updateVisibleLayers,
    updateLayerArrangement = updateLayerArrangement,
    registerSettingsPage = registerSettingsPage,
    registerSettingsGroup = registerSettingsGroup,
    interface = interface,
    uiSection = uiSection,
    spacesSection = spacesSection,
}
