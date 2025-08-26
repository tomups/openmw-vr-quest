#ifndef OPENXR_MENU_HPP
#define OPENXR_MENU_HPP

#include <MyGUI_Widget.h>
#include <array>
#include <map>
#include <set>
#include <mutex>

#include <osg/Camera>
#include <osg/Geometry>
#include <osg/PositionAttitudeTransform>
#include <osg/TexMat>
#include <osg/Texture2D>

#include <components/vr/session.hpp>
#include <components/vr/space.hpp>
#include <components/vr/layer.hpp>
#include <components/vr/vr.hpp>

#include <components/lua_ui/element.hpp>

namespace MyGUI
{
    class Widget;
    class Window;
}

namespace MWGui
{
    class Layout;
    class WindowBase;
}

namespace Resource
{
    class ResourceSystem;
}

struct XrCompositionLayerQuad;
namespace MWVR
{
    class VRGUIManager;

    /// Configuration of a VRGUILayer
    struct LayerConfig
    {
        float opacity; //!< Background color of layer
        osg::Vec2 center; //!< Model space centerpoint of menu geometry. All menu geometries have model space lengths of
                          //!< 1 in each dimension. Use this to affect how geometries grow with changing size.
        osg::Vec2 extent; //!< Spatial extent of the layer in meters when using Fixed sizing mode
        int spatialResolution; //!< Pixels when using the Auto sizing mode. \note Pixels per meter of the GUI viewport,
                               //!< not the RTT texture.
        osg::Vec2i pixelResolution; //!< Pixel resolution of the RTT texture
        osg::Vec2 myGUIViewSize; //!< Resizable elements are resized to this (fraction of full view)
        bool autoSize; //!< How to size the layer
        std::string space;
        //std::string extraLayers; //!< Additional layers to draw (list separated by any non-alphabetic)
        bool intersectable;
    };

    /// \brief A single VR GUI Quad.
    ///
    /// In VR menus are shown as quads within the game world.
    /// The behaviour of that quad is defined by the MWVR::LayerConfig struct
    /// Each instance of VRGUILayer is used to show one MYGUI layer.
    class VRGUILayer : public osg::Referenced
    {
    public:
        VRGUILayer(osg::ref_ptr<osg::Group> geometryRoot, osg::ref_ptr<osg::Group> cameraRoot, std::string layerName, VRGUIManager* parent);
        ~VRGUILayer();

        void update(osg::NodeVisitor* nv);

    public:
        osg::ref_ptr<osg::Geometry> createLayerGeometry(osg::ref_ptr<osg::StateSet> stateset);
        //void setAngle(float angle);
        void updatePose();
        void updateRect();
        void insertWidget(MWGui::Layout* widget);
        void removeWidget(MWGui::Layout* widget);
        int widgetCount() { return mWidgets.size() + mLuaElements.size(); }
        //bool operator<(const VRGUILayer& rhs) const { return mConfig->priority < rhs.mConfig->priority; }
        void cull(osgUtil::CullVisitor* cv);
        void setVisible(bool visible);
        void removeFromSceneGraph();
        void addToSceneGraph();

        /// Update layer quads based on current tracking information
        //void onTrackingUpdated(VR::TrackingManager& manager) override;

        osg::Texture2D* colorTexture() const;
        std::shared_ptr<VR::QuadLayer> vrLayer() { return mVrLayer; } 

        bool updateLayer();
        void blitLayer(osg::RenderInfo& info);

        bool visible() const { return mVisible; }

        void setConfig(const std::string& mode, const LayerConfig& config);
        void setMode(const std::string& mode);

        void clear();

        void addLuaElement(const LuaUi::Element* element);
        void removeLuaElement(const LuaUi::Element* element);

    public:
        //Stereo::Pose mTrackingPose = Stereo::Pose();
        //Stereo::Pose mTrackedPose{};
        //osg::Quat mRotation{ 0, 0, 0, 1 };

        const LayerConfig* activeConfig();

        std::map<std::string, LayerConfig> mPerModeConfigs;
        std::string mActiveMode;

        std::string mLayerName;
        std::vector<MWGui::Layout*> mWidgets;
        osg::ref_ptr<osg::Group> mGeometryRoot;
        std::array<osg::ref_ptr<osg::Geometry>, 2> mGeometries;
        osg::ref_ptr<osg::PositionAttitudeTransform> mTransform{ new osg::PositionAttitudeTransform };
        osg::ref_ptr<osg::StateSet> mStateset;
        MyGUI::IntRect mRect{};
        MyGUI::FloatRect mRealRect{};
        osg::ref_ptr<osg::Group> mCameraRoot;
        osg::ref_ptr<osg::Node> mGUIRTT;
        osg::ref_ptr<osg::Camera> mMyGUICamera{ nullptr };
        bool mVisible = false;
        bool mDirty = false;
        bool mSpaceIsLost = false;
        bool mVisibleChanged = false;
        std::shared_ptr<VR::QuadLayer> mVrLayer;
        std::shared_ptr<VR::Space> mSpace;
        Stereo::Pose mPose;
        std::vector<const LuaUi::Element*> mLuaElements;
    };

    /// \brief Manager of VRGUILayer objects.
    ///
    /// Constructs and destructs VRGUILayer objects in response to MWGui::Layout::setVisible calls.
    /// Layers can also be made visible directly by calling insertLayer() directly, e.g. to show
    /// the video player.
    class VRGUIManager : VR::Session::Listener
    {
    public:
        static constexpr const char* DefaultUiSpace = "/default/ui/menu";

        static VRGUIManager& instance();

        VRGUIManager(osg::Group* rootNode);

        ~VRGUIManager(void);

        void initScene();

        void readConfig();

        void clear();

        /// Set visibility of the layer this layout is on
        void setVisible(MWGui::Layout*, bool visible);

        /// Show the given layer
        void showLayer(const std::string& name);

        /// Hide the given layer
        void hideLayer(const std::string& name);

        /// Check current pointer target and update focus layer
        void updateFocus(osg::Node* focusNode, osg::Vec3f hitPoint);

        /// True if user is currently pointing at something
        bool hasFocus() const;

        /// Update traversal
        void update(osg::NodeVisitor* nv);

        /// Update traversal
        void onFrameUpdate(VR::Frame& frame) override;
        void onSpaceUpdate() override;
        void onFrameEnd(osg::RenderInfo& info, VR::Frame& frame) override;

        /// Gui cursor coordinates to use to simulate a mouse press/move if the player is currently pointing at a vr gui
        /// layer
        osg::Vec2i guiCursor() { return mGuiCursor; }

        /// Inject mouse click if applicable
        bool injectMouseClick();

        /// Update settings where applicable
        void processChangedSettings(const std::set<std::pair<std::string, std::string>>& changed);

        static void registerMyGUIFactories();

        static void setPick(MWGui::Layout* widget, bool pick);

        void setLayerConfig(const std::string& layer, const LayerConfig& config);
        void setLayerPose(const std::string& layer, const Stereo::Pose& pose);
        void setModeConfig(const std::string& mode, const LayerConfig& config);
        void setModePose(const std::string& mode, const Stereo::Pose& pose, const std::string window = "");

        void registerLuaElement(const LuaUi::Element* element);
        void deregisterLuaElement(const LuaUi::Element* element);

    private:
        // Not used: void removeLayer(const std::string& name);
        void computeGuiCursor(osg::Vec3 hitPoint);
        //void updateSideBySideLayers();
        void insertWidget(MWGui::Layout* widget);
        void removeWidget(MWGui::Layout* widget);
        void setFocusLayer(osg::ref_ptr<VRGUILayer> layer);
        void setFocusWidget(MyGUI::Widget* widget);
        VRGUILayer& getLayer(const std::string& layer);

        osg::ref_ptr<osgViewer::Viewer> mOsgViewer;

        osg::ref_ptr<osg::Group> mRootNode = nullptr;
        osg::ref_ptr<osg::Group> mGeometries = new osg::Group;
        osg::ref_ptr<osg::Group> mGUICameras = new osg::Group;

        std::map<std::string, LayerConfig> mModeConfigs;
        std::map<std::string, osg::ref_ptr<VRGUILayer>> mLayers;
        std::vector<osg::ref_ptr<VRGUILayer>> mSideBySideLayers;

        osg::Vec2i mGuiCursor;
        osg::ref_ptr<VRGUILayer> mFocusLayer = nullptr;
        MyGUI::Widget* mFocusWidget = nullptr;
        std::map<std::string, LayerConfig> mDefaultLayerConfigs;

        std::shared_ptr<VR::Session::Listener> mSessionListener;
        std::mutex mMutex;
    };
}

#endif
