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

local layer = 'Tooltip'

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
            size = util.vector2(180, 110)
        }
    }}
}

-- Make our own layer
local element = nil
local function init()
    ui.layers.insertAfter('MainMenu', layer, {interactive = true})
    element = ui.create(layout)
end

local function show()
    layout.props.visible = true
    element:update()
end

local function hide()
    layout.props.visible = false
    element:update()
end

return function(isStart)
    if not element then init() end
    if isStart then
        show()
    else
        hide()
    end
end
