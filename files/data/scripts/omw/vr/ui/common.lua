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

local core = require('openmw.core')
local storage = require('openmw.storage')

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

local function getWindowLayer(window)
    return windowToLayer[window] or 'Windows'
end

local settingsPageKey = 'OMWVRUI'
local uiGroupKey = 'UiGroup'
local uiSection = storage.playerSection(uiGroupKey)
local spacesGroupKey = 'SpacesGroup'
local spacesSection = storage.playerSection(spacesGroupKey)
local spaceOffsetGroupKey = 'SpaceOffsetGroup'

return {
    getWindowLayer = getWindowLayer,
    settingsPageKey = settingsPageKey,
    l10nKey = settingsPageKey,
    l10n = core.l10n(settingsPageKey),
    uiGroupKey = uiGroupKey,
    uiSection = uiSection,
    spacesGroupKey = spacesGroupKey,
    spacesSection = spacesSection,
    spaceOffsetGroupKey = spaceOffsetGroupKey,
}
