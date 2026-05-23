local vr = require('openmw.vr')
if not vr.isVr() then
    return {}
end
local async = require('openmw.async')
local util = require('openmw.util')
local core = require('openmw.core')
local ui = require('openmw.ui')
local I = require('openmw.interfaces')
local common = require('scripts.omw.vr.ui.common')








--- List of layer that need to be arranged when multiple windows are opened
--- In order of priority (left-to-right)
local layersForArrangement = {
    'DialogueWindow', 'MapWindow', 'SpellWindow', 'InventoryWindow', 'StatsWindow', 'InventoryCompanionWindow', 'Windows'
}

local function getAllLayers()
    local layers = {}
    for _, v in pairs(ui.layers) do
        layers[#layers + 1] = v.name
    end

    return layers
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

local HUDSpaces = {
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


local configHUD3D = createDefaultConfig(0, true)
configHUD3D.extent = util.vector2(0.033, 0.033)
configHUD3D.center = util.vector2(0, 0.5)
configHUD3D.space = 'LeftWristTop'
layerConfig.HUD_3D = configHUD3D

local configTooltip = createDefaultConfig(0, true)
configTooltip.extent = util.vector2(0.033, 0.033)
configTooltip.space = 'RightWristTop'
layerConfig.Tooltip = configTooltip

local function registerSettingsPage()
    I.Settings.registerPage({
        key = common.settingsPageKey,
        l10n = common.l10nKey,
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

local function selectSetting(key, items, default)
    if not default then default = items[1] end
    return {
        key = key,
        renderer = 'select',
        name = key,
        description = key .. 'Description',
        default = default,
        argument = {
            l10n = common.l10nKey,
            items = items
        }
    }
end

local function spaceOffsetSettingKey(space)
    return space..'Offset'
end

local function spaceOffsetSetting(space, disabled)
    local key = spaceOffsetSettingKey(space)
    return {
        key = key,
        renderer = 'spaceOffset',
        name = key,
        description = key .. 'Description',
        argument = {
            space = space,
            disabled = disabled,
        }
    }
end

local function registerSettingsGroup()
    I.Settings.registerGroup({
        key = common.spacesGroupKey,
        page = common.settingsPageKey,
        l10n = common.l10nKey,
        name = common.spacesGroupKey,
        permanentStorage = true,
        settings = {
            selectSetting('HUDSpace', HUDSpaces),
            selectSetting('TooltipSpace', HUDSpaces),
            boolSetting('DialogueSpace', true),
        },
    })
    I.Settings.registerGroup({
        key = common.uiGroupKey,
        page = common.settingsPageKey,
        l10n = common.l10nKey,
        name = common.uiGroupKey,
        permanentStorage = true,
        settings = {
            boolSetting('ShowTutorials', true),
        },
    })
    local spaceOffsetSettings = {
        spaceOffsetSetting('DefaultWindow')
    }
    for _, v in ipairs(HUDSpaces) do
        spaceOffsetSettings[#spaceOffsetSettings+1] = spaceOffsetSetting(v, false)
    end
    I.Settings.registerGroup({
        key = common.spaceOffsetGroupKey,
        page = common.settingsPageKey,
        l10n = common.l10nKey,
        name = common.spaceOffsetGroupKey,
        permanentStorage = true,
        settings = spaceOffsetSettings,
    })

-- Tooltip won't be visible when we hit record, so create our own tooltip as a preview instead
local tooltipPreview = require('scripts.omw.vr.ui.tooltipRecorderHandler')
    I.SpaceOffsetSettingsRenderer.addRecordingHandler(function(space, isStart)
        if space == configHUD3D.space then
            -- The HUD is already our recording preview
            return false
        end
        if space == configTooltip.space then
            tooltipPreview(isStart)
            return false
        end
        if space == 'DefaultWindow' then
            -- The position of the settings menu is already based on this space
            return false
        end
    end)
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

local function updateSpacesSettings()
    configHUD3D.space = common.spacesSection:get('HUDSpace')
    configTooltip.space = common.spacesSection:get('TooltipSpace')
    local dialogueSpace = common.spacesSection:get('DialogueSpace')
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

    for _, space in ipairs(HUDSpaces) do
        local disabled = (space ~= configHUD3D.space) and (space ~= configTooltip.space)
        I.Settings.updateRendererArgument(common.
            spaceOffsetGroupKey,
            spaceOffsetSettingKey(space),
            { space = space, disabled = disabled }
        )
    end
end
common.spacesSection:subscribe(async:callback(updateSpacesSettings))

local function setupDefaults()
    updateSpacesSettings()

    local allLayers = getAllLayers()
    for _, layer in pairs(allLayers) do
        if not layerConfig[layer] then
            layerConfig[layer] = createDefaultConfig(0.7, true)
        end
    end

    local bookLayer = layerConfig[common.getWindowLayer('Book')]
    if bookLayer then
        bookLayer.backgroundOpacity = 0
    end
    local journalLayer = layerConfig[common.getWindowLayer('Journal')]
    if journalLayer then
        journalLayer.backgroundOpacity = 0
    end
    local scrollLayer = layerConfig[common.getWindowLayer('Scroll')]
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

local referenceWorldPose = {
    position = util.vector3(0,0,0),
    orientation = util.transform.identity,
}

local windowArrangementRelativePose = {}
local function updateWindowRelativePoses()
    windowArrangementRelativePose = I.vrspaces.locateSpace(getWindowSpaceForCurrentMode(), I.vrspaces.referenceSpaces.View) or windowArrangementRelativePose
    if windowArrangementRelativePose.position then
        -- Rewrite the orientation to be facing the player directly
        local position = windowArrangementRelativePose.position
        local a2 = math.atan2(position.x, position.y)
        windowArrangementRelativePose.orientation = util.transform.rotateZ(a2)
    end
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

    local step = (-math.pi / 4)
    local span = step * (#visibleLayersForArrangement - 1)
    local angle = -span / 2 + referenceWorldPose.orientation:getYaw()
    for _, layer in ipairs(visibleLayersForArrangement) do
        local transform = util.transform.rotateZ(angle) --* util.transform.rotateZ(windowArrangementRelativePose.orientation:getYaw())
        local rotatedPosition = transform:apply(windowArrangementRelativePose.position)

        setLayerPoseIfNotOverridden(layer, {
            position = referenceWorldPose.position + rotatedPosition,
            orientation = util.transform.rotateZ(angle) * util.transform.rotateZ(windowArrangementRelativePose.orientation:getYaw())
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
    local transform = util.transform.rotateZ(referenceWorldPose.orientation:getYaw())
    return {
        position = referenceWorldPose.position + transform:apply(windowArrangementRelativePose.position),
        orientation = transform * windowArrangementRelativePose.orientation
}
end

local function computeVirtualKeyboardPose()
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

createDerivedSpaces()
registerSettingsPage()
registerSettingsGroup()

local initialized = false
local function init()
    setupDefaults()
    updatePoses()
    updateLayers()
    initialized = true
end

local function update()
    setupDefaults()
    updatePoses()
    updateLayers()
    updateLayerArrangement()
end

local wasKBMouseMode = false
local wasPaused = false
local updateOnce = true
local function onVRFrame()
    if not initialized then
        init()
    end

    local KBMouseMode = I.vrinputs.isKBMouseMode()
    if KBMouseMode ~= wasKBMouseMode then
        updateOnce = true
        wasKBMouseMode = KBMouseMode
    end

    if updateOnce then
        update()
        updateOnce = false
    end

    -- We only want to update the reference poses when the user enters GUI mode. Otherwise, the windows will be actively tracking
    -- them which is weird, uncomfortable, and impractical.
    local isPaused = core.isWorldPaused()
    if (isPaused and not wasPaused) or updateVisibleLayers() then
        update()
    end

    wasPaused = isPaused
end

return {
    engineHandlers = {
        onVRFrame = onVRFrame,
        onVRRecenter = update,
    },
    interfaceName = 'vrui',
    interface = {
        version = 0,
        overrideLayerConfig = overrideLayerConfig,
        showMessageInTheVoid = function(message) vr._showMessageInTheVoid(message) end,
        setLayerConfig = function(layer, config) vr._setLayerConfig(layer, config) end,
        setLayerPose = function(layer, pose) vr._setLayerPose(layer, pose) end,
        getWindowLayer = common.getWindowLayer,
        setLayerPickable = vr._setLayerPickable
    },
    eventHandlers = {
        InterfaceCall = function(data)
            I[data.interface][data.func](unpack(data.arg or {}))
        end,
        vruiUpdateOnce = function(_)
            updateOnce = true
        end,
    }
}
