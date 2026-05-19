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

local layer = 'vruiRecorderLayer'
-- Configure the default recorder to behave similar to HUD/Tooltip
local layerConfig = {
    backgroundOpacity = 0.7,
    center = util.vector2(0,0),
    extent = util.vector2(0.033,0.033),
    pixelsPerMeter = 1024,
    pixelResolution = util.vector2(1024, 1024),
    autosize = true,
}

-- Create a default widget ~ the size of a big tooltip
local layout = {
    template = I.MWUI.templates.boxSolidThick,
    props = {
        visible = false,
    },
    layer = layer,
    content = ui.content{{
        type = ui.TYPE.Widget,
        props = {
            size = util.vector2(300, 160)
        }
    }}
}

-- Make our own layer
local element = nil
local function init()
    ui.layers.insertAfter('MainMenu', layer, {interactive = true})
    I.vrui.overrideLayerConfig(layer, true)
    I.vrui.setLayerConfig(layer, layerConfig)
    element = ui.create(layout)
end

local function show(space)
    layout.props.visible = true
    element:update()
    layerConfig.space = space
    I.vrui.setLayerConfig(layer, layerConfig)
end

local function hide()
    layout.props.visible = false
    element:update()

    -- Layer won't be visible anymore so this doesn't really matter
    layerConfig.space = nil
    I.vrui.setLayerConfig(layer, layerConfig)
end

return function(
    space,
    isStart) -- true if this is the start of recording (show), false if it is the end (hide)
    if not element then init() end
    if isStart then
        show(space)
    else
        hide()
    end
end
