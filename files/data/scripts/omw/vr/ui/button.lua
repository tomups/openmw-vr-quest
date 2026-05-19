local util = require('openmw.util')
local I = require('openmw.interfaces')
local async = require('openmw.async')
local core = require('openmw.core')
local ambient = require('openmw.ambient')
local ui = require('openmw.ui')
local auxUi = require('openmw_aux.ui')

local normalColor = util.color.commaString(core.getGMST("FontColor_color_normal"))
local normalColorOver = util.color.commaString(core.getGMST('fontcolor_color_normal_over'))
local normalColorPressed = util.color.commaString(core.getGMST('fontcolor_color_normal_pressed'))
local function paddedBox(content)
    return {
        template = I.MWUI.templates.box,
        content = ui.content {
            {
                template = I.MWUI.templates.padding,
                content = content,
            },
        }
    }
end

local function buttonPress(layout, press)
    layout.userData.pressed = press
    layout.userData:update()
end

local function buttonFocus(layout, focus)
    if layout.userData then
        layout.userData.focused = focus
        layout.userData:update()
    end
end

local buttonEvents = {
    mousePress = async:callback(function(_, layout)
        if layout.userData then
            buttonPress(layout, true)
            ambient.playSound('menu click')
        end
    end),
    mouseRelease = async:callback(function(_, layout)
        if layout.userData then
            buttonPress(layout, false)
            layout.userData.callback()
        end
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

local function newButton(text, cb)
    local widget = {
        text = text,
        callback = cb,
    }

    function widget:update()
        local textProps = widget.textContent[1].props
        textProps.text = self.text
        textProps.textColor = widget:color()
        self.element:update()
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

    function widget:cleanup()
        self.buttonContent[1].userData = nil
    end

    function widget:destroy()
        self:cleanup()
        self.element:destroy()
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
        content = ui.content{paddedBox(widget.textContent)},
        userData = widget,
        events = buttonEvents
    }}
    widget.element = ui.create{
        type = ui.TYPE.Container,
        props = {},
        content = widget.buttonContent,
    }
    widget:update()
    return widget
end
return newButton
