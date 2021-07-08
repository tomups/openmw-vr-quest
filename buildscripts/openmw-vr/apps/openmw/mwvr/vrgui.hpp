#ifndef OPENXR_MENU_HPP
#define OPENXR_MENU_HPP

#include <map>
#include <set>
#include <regex>
#include <MyGUI_Widget.h>

#include <osg/Geometry>
#include <osg/TexMat>
#include <osg/Texture2D>
#include <osg/Camera>
#include <osg/PositionAttitudeTransform>

#include "openxrmanager.hpp"
#include "vrtracking.hpp"

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
    class GUICamera;
    class VRGUIManager;

    // Some UI elements should occupy predefined geometries
    // Others should grow/shrink freely
    enum class SizingMode
    {
        Auto,
        Fixed
    };

    /// Configuration of a VRGUILayer
    struct LayerConfig
    {
        int priority; //!< Higher priority shows over lower priority windows by moving higher priority layers slightly closer to the player.
        bool sideBySide; //!< All layers with this config will show up side by side in a partial circle around the player, and will all be resized to a predefined size.
        osg::Vec4 backgroundColor; //!< Background color of layer
        osg::Vec3 offset; //!< Offset from tracked node in meters
        osg::Vec2 center; //!< Model space centerpoint of menu geometry. All menu geometries have model space lengths of 1 in each dimension. Use this to affect how geometries grow with changing size.
        osg::Vec2 extent; //!< Spatial extent of the layer in meters when using Fixed sizing mode
        int spatialResolution; //!< Pixels when using the Auto sizing mode. \note Pixels per meter of the GUI viewport, not the RTT texture.
        osg::Vec2i pixelResolution; //!< Pixel resolution of the RTT texture
        osg::Vec2 myGUIViewSize; //!< Resizable elements are resized to this (fraction of full view)
        SizingMode sizingMode; //!< How to size the layer
        std::string trackingPath; //!< The path that will be used to read tracking data
        std::string extraLayers; //!< Additional layers to draw (list separated by any non-alphabetic)

        bool operator<(const LayerConfig& rhs) const { return priority < rhs.priority; }
    };

    //! Extends the tracking source with /ui/input/stationary/pose
    //! \note reference space will be ignored when reading /ui/input/stationary/pose
    class VRGUITracking : public VRTrackingSource
    {
    public:
        VRGUITracking(const std::string& source);

        virtual std::vector<VRPath> listSupportedTrackingPosePaths() const override;
        virtual void updateTracking(DisplayTime predictedDisplayTime) override;
        void resetStationaryPose();

    protected:
        virtual VRTrackingPose getTrackingPoseImpl(DisplayTime predictedDisplayTime, VRPath path, VRPath reference = 0) override;

    private:
        VRPath mStationaryPath = 0;
        VRPath mHeadPath = 0;
        VRTrackingPose mStationaryPose = VRTrackingPose();
        VRTrackingSource* mSource = nullptr;

        bool mShouldUpdateStationaryPose = true;
    };

    /// \brief A single VR GUI Quad.
    ///
    /// In VR menus are shown as quads within the game world.
    /// The behaviour of that quad is defined by the MWVR::LayerConfig struct
    /// Each instance of VRGUILayer is used to show one MYGUI layer.
    class VRGUILayer : public VRTrackingListener
    {
    public:
        VRGUILayer(
            osg::ref_ptr<osg::Group> geometryRoot,
            osg::ref_ptr<osg::Group> cameraRoot,
            std::string layerName,
            LayerConfig config,
            VRGUIManager* parent);
        ~VRGUILayer();

        void update();

    protected:
        friend class VRGUIManager;
        osg::Camera* camera();
        osg::ref_ptr<osg::Texture2D> menuTexture();
        void setAngle(float angle);
        void updatePose();
        void updateRect();
        void insertWidget(MWGui::Layout* widget);
        void removeWidget(MWGui::Layout* widget);
        int  widgetCount() { return mWidgets.size(); }
        bool operator<(const VRGUILayer& rhs) const { return mConfig.priority < rhs.mConfig.priority; }

        /// Update layer quads based on current tracking information
        void onTrackingUpdated(VRTrackingSource& source, DisplayTime predictedDisplayTime) override;

    public:
        VRPath      mTrackingPath = 0;
        Pose        mTrackingPose = Pose();

        Pose mTrackedPose{};
        LayerConfig mConfig;
        std::string mLayerName;
        std::vector<MWGui::Layout*> mWidgets;
        osg::ref_ptr<osg::Group> mGeometryRoot;
        osg::ref_ptr<osg::Geometry> mGeometry{ new osg::Geometry };
        osg::ref_ptr<osg::PositionAttitudeTransform> mTransform{ new osg::PositionAttitudeTransform };
        osg::ref_ptr<osg::Group> mCameraRoot;
        osg::ref_ptr<GUICamera> mGUICamera;
        osg::ref_ptr<osg::Camera> mMyGUICamera{ nullptr };
        MyGUI::FloatRect mRealRect{};
        osg::Quat mRotation{ 0,0,0,1 };
    };

    /// \brief osg user data used to refer back to VRGUILayer objects when intersecting with the scene graph.
    class VRGUILayerUserData : public osg::Referenced
    {
    public:
        VRGUILayerUserData(std::shared_ptr<VRGUILayer> layer) : mLayer(layer) {};

        std::weak_ptr<VRGUILayer> mLayer;
    };

    /// \brief Manager of VRGUILayer objects.
    ///
    /// Constructs and destructs VRGUILayer objects in response to MWGui::Layout::setVisible calls.
    /// Layers can also be made visible directly by calling insertLayer() directly, e.g. to show
    /// the video player.
    class VRGUIManager : public VRTrackingListener
    {
    public:
        VRGUIManager(
            osg::ref_ptr<osgViewer::Viewer> viewer,
            Resource::ResourceSystem* resourceSystem,
            osg::Group* rootNode);

        ~VRGUIManager(void);

        /// Set visibility of the layer this layout is on
        void setVisible(MWGui::Layout*, bool visible);
        
        /// Insert the given layer quad if it isn't already
        void insertLayer(const std::string& name);

        /// Remove the given layer quad
        void removeLayer(const std::string& name);

        /// Check current pointer target and update focus layer
        bool updateFocus();

        /// Update traversal
        void update();

        /// Update traversal
        void updateTracking();

        /// Gui cursor coordinates to use to simulate a mouse press/move if the player is currently pointing at a vr gui layer
        osg::Vec2i guiCursor() { return mGuiCursor; };

        /// Inject mouse click if applicable
        bool injectMouseClick(bool onPress);

        /// Notify that widget is about to be destroyed.
        void notifyWidgetUnlinked(MyGUI::Widget* widget);

        /// Update settings where applicable
        void processChangedSettings(const std::set< std::pair<std::string, std::string> >& changed);

        static void registerMyGUIFactories();

        static void setPick(MWGui::Layout* widget, bool pick);

        void setGeometryRoot(osg::Group* root);
        void setCameraRoot(osg::Group* root);

    private:
        void computeGuiCursor(osg::Vec3 hitPoint);
        void updateSideBySideLayers();
        void insertWidget(MWGui::Layout* widget);
        void removeWidget(MWGui::Layout* widget);
        void setFocusLayer(VRGUILayer* layer);
        void setFocusWidget(MyGUI::Widget* widget);
        void configUpdated(const std::string& layer);

        osg::ref_ptr<osgViewer::Viewer> mOsgViewer{ nullptr };
        Resource::ResourceSystem* mResourceSystem;

        osg::ref_ptr<osg::Group> mGeometriesRootNode{ nullptr };
        osg::ref_ptr<osg::Group> mGeometries{ new osg::Group };
        osg::ref_ptr<osg::Group> mGUICamerasRootNode{ nullptr };
        osg::ref_ptr<osg::Group> mGUICameras{ new osg::Group };

        std::unique_ptr<VRGUITracking> mUiTracking = nullptr;

        std::map<std::string, std::shared_ptr<VRGUILayer>> mLayers;
        std::vector<std::shared_ptr<VRGUILayer> > mSideBySideLayers;

        osg::Vec2i  mGuiCursor{};
        VRGUILayer* mFocusLayer{ nullptr };
        MyGUI::Widget* mFocusWidget{ nullptr };
        std::map<std::string, LayerConfig> mLayerConfigs{};
    };
}

#endif
