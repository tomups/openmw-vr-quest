local vr = require('openmw.vr')
if not vr.isVr() then
    return {}
end
local menu = require('openmw.menu')
local util = require('openmw.util')
local I = require('openmw.interfaces')
local storage = require('openmw.storage')

local async = require('openmw.async')
local core = require('openmw.core')
local ambient = require('openmw.ambient')
local ui = require('openmw.ui')
local auxUi = require('openmw_aux.ui')
local auxUtil = require('openmw_aux.util')
local utils = require('scripts.omw.mwui.util.utils')
local constants = require('scripts.omw.mwui.constants')

local normalColor = util.color.commaString(core.getGMST("FontColor_color_normal"))
local normalColorOver = util.color.commaString(core.getGMST('fontcolor_color_normal_over'))
local normalColorPressed = util.color.commaString(core.getGMST('fontcolor_color_normal_pressed'))


local paddingTemplate = {
        type = ui.TYPE.Container,
        content = ui.content {
            {
                props = {
                    size = util.vector2(12, 1),
                },
            },
            {
                external = { slot = true },
                props = {
                    position = util.vector2(12, 1),
                    relativeSize = util.vector2(1, 1),
                },
            },
            {
                props = {
                    position = util.vector2(12, 1),
                    relativePosition = util.vector2(1, 1),
                    size = util.vector2(12, 1),
                },
            },
        }
    }

local function buttonPress(layout, press)
    layout.userData.pressed = press
    layout.userData:update()
end

local function buttonFocus(layout, focus)
    layout.userData.focused = focus
    layout.userData:update()
end

local buttonEvents = {
    mousePress = async:callback(function(_, layout)
        buttonPress(layout, true)
        ambient.playSound('menu click')
    end),
    mouseRelease = async:callback(function(_, layout)
        buttonPress(layout, false)
        layout.userData.callback()
    end),
    focusGain = async:callback(function(_, layout)
        buttonFocus(layout, true)
        return true
    end),
    focusLoss = async:callback(function(_, layout)
        buttonFocus(layout, false)
        return true
    end),
}

local props = {
    'borderless',
    'textColor',
    'textColorOver',
    'textColorPressed',
    'text',
    'textSize',
    'textShadow',
    'textShadowColor',
}
local function initButton(text, cb)
    local widget = {
        text = text,
        callback = cb,
    }

    function widget:update()
        local props = widget.props or {}
        local textProps = widget.textContent[1].props
        textProps.text = self.text
        textProps.textColor = widget:color()
        if props.borderless then
            widget.buttonContent[1].content = widget.textContent
        else
            widget.buttonContent[1].content = ui.content{{
                template = widget.template or I.MWUI.templates.button,
                content = ui.content{{
                    template = paddingTemplate,
                    content = widget.textContent
                }},
            }}
        end
    end

    function widget:color()
        if self.pressed then
            return normalColorPressed
        elseif self.focused then
            return normalColorOver
        end
        return normalColor
    end

    function widget:clicked()
        if self.events and self.events.buttonCallback then
            self.events.buttonCallback(nil, self.element.layout)
        end
    end

    widget.textContent = ui.content{{
        name = 'text',
        type = ui.TYPE.Text,
        template = I.MWUI.templates.textNormal,
        props = {}
    }}

    -- Add a redundant container layout so we can manipulate userData and events
    -- without colliding with input userData/events. Important for tooltips.
    widget.buttonContent = ui.content{{
        name = 'button',
        type = ui.TYPE.Container,
        content = widget.textContent,
        userData = widget,
        events = buttonEvents
    }}
    widget.element = ui.create{
        type = ui.TYPE.Container,
        content = widget.buttonContent
    }
    return widget.element
end
local function destroyButton(widget)
    -- Break circular reference to let GC do its work
    widget.element.layout.content[1].userData = nil
    auxUi.destroyRecursively(widget.element)
end

local recording = nil

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
    local setting = {
        space = space,
        offset = value or util.vector3(0,0,0),
    }
    local recorder = initButton(string.format('%.2f, %.2f, %.2f', setting.offset.x, setting.offset.y, setting.offset.z), function()
        recording = {
            setting = setting,
            refresh = function()
                set(setting)
            end
        }
    end)
end)

local function onVRFrame()
    if not recording then
        return
    end
    recording.position = I.vrspaces.locateSpace(recording.space, I.vrspaces.referenceSpaces.Local)
    if not recording.origin then
        recording.origin = recording.position
        recording.referencePosition = I.vrspaces.locateSpace(I.vrspaces.customSpaces[recording.space].reference, I.vrspaces.referenceSpaces.Local)
    end
end

return {
    engineHandlers = {
        onVRFrame = onVRFrame,
    },
}