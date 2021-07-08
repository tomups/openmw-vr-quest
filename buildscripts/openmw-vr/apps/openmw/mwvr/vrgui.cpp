#include "vrgui.hpp"

#include <cmath>

#include "vranimation.hpp"
#include "vrenvironment.hpp"
#include "vrpointer.hpp"
#include "vrsession.hpp"
#include "openxrinput.hpp"
#include "openxrmanagerimpl.hpp"

#include <osg/Texture2D>
#include <osg/ClipNode>
#include <osg/FrontFace>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/Fog>
#include <osg/LightModel>


#include <osgViewer/Renderer>

#include <components/sceneutil/visitor.hpp>
#include <components/sceneutil/shadow.hpp>
#include <components/myguiplatform/myguirendermanager.hpp>
#include <components/myguiplatform/additivelayer.hpp>
#include <components/myguiplatform/scalinglayer.hpp>
#include <components/misc/constants.hpp>
#include <components/misc/stringops.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/shader/shadermanager.hpp>

#include "../mwrender/util.hpp"
#include "../mwrender/renderbin.hpp"
#include "../mwrender/renderingmanager.hpp"
#include "../mwrender/camera.hpp"
#include "../mwrender/vismask.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwgui/windowbase.hpp"

#include "../mwbase/statemanager.hpp"

#include <MyGUI_Widget.h>
#include <MyGUI_ILayer.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_WidgetManager.h>
#include <MyGUI_Window.h>
#include <MyGUI_FactoryManager.h>
#include <MyGUI_OverlappedLayer.h>
#include <MyGUI_SharedLayer.h>

namespace osg
{
    // Convenience
    const double PI_8 = osg::PI_4 / 2.;
}

namespace MWVR
{

    // When making a circle of a given radius of equally wide planes separated by a given angle, what is the width
    static osg::Vec2 radiusAngleWidth(float radius, float angleRadian)
    {
        const float width = std::fabs(2.f * radius * tanf(angleRadian / 2.f));
        return osg::Vec2(width, width);
    }

    /// RTT camera used to draw the osg GUI to a texture
    class GUICamera : public osg::Camera
    {
    public:
        GUICamera(int width, int height, osg::Vec4 clearColor)
        {
            setRenderOrder(osg::Camera::PRE_RENDER);
            setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            setCullingActive(false);

            // Make the texture just a little transparent to feel more natural in the game world.
            setClearColor(clearColor);

            setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
            setReferenceFrame(osg::Camera::ABSOLUTE_RF);
            setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
            setName("GUICamera");

            setCullMask(MWRender::Mask_GUI);
            setCullMaskLeft(MWRender::Mask_GUI);
            setCullMaskRight(MWRender::Mask_GUI);
            setNodeMask(MWRender::Mask_RenderToTexture);

            setViewport(0, 0, width, height);

            // No need for Update traversal since the mSceneRoot is already updated as part of the main scene graph
            // A double update would mess with the light collection (in addition to being plain redundant)
            setUpdateCallback(new MWRender::NoTraverseCallback);

            // Create the texture
            mTexture = new osg::Texture2D;
            mTexture->setTextureSize(width, height);
            mTexture->setInternalFormat(GL_RGBA);
            mTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR);
            mTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
            mTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            mTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            attach(osg::Camera::COLOR_BUFFER, mTexture);
            // Need to regenerate mipmaps every frame
            setPostDrawCallback(new MWRender::MipmapCallback(mTexture));

            // Do not want to waste time on shadows when generating the GUI texture
            SceneUtil::ShadowManager::disableShadowsForStateSet(getOrCreateStateSet());

            // Put rendering as early as possible
            getOrCreateStateSet()->setRenderBinDetails(-1, "RenderBin");

        }

        void setScene(osg::Node* scene)
        {
            if (mScene)
                removeChild(mScene);
            mScene = scene;
            addChild(scene);
            Log(Debug::Verbose) << "Set new scene: " << mScene->getName();
        }

        osg::Texture2D* getTexture() const
        {
            return mTexture.get();
        }

    private:
        osg::ref_ptr<osg::Texture2D> mTexture;
        osg::ref_ptr<osg::Node> mScene;
    };


    //class LayerUpdateCallback : public osg::Callback
    //{
    //public:
    //    LayerUpdateCallback(VRGUILayer* layer)
    //        : mLayer(layer)
    //    {

    //    }

    //    bool run(osg::Object* object, osg::Object* data)
    //    {
    //        mLayer->update();
    //        return traverse(object, data);
    //    }

    //private:
    //    VRGUILayer* mLayer;
    //};

    VRGUILayer::VRGUILayer(
        osg::ref_ptr<osg::Group> geometryRoot,
        osg::ref_ptr<osg::Group> cameraRoot,
        std::string layerName,
        LayerConfig config,
        VRGUIManager* parent)
        : mConfig(config)
        , mLayerName(layerName)
        , mGeometryRoot(geometryRoot)
        , mCameraRoot(cameraRoot)
    {
        osg::ref_ptr<osg::Vec3Array> vertices{ new osg::Vec3Array(4) };
        osg::ref_ptr<osg::Vec2Array> texCoords{ new osg::Vec2Array(4) };
        osg::ref_ptr<osg::Vec3Array> normals{ new osg::Vec3Array(1) };

        auto extent_units = config.extent * Constants::UnitsPerMeter;

        float left = mConfig.center.x() - 0.5;
        float right = left + 1.f;
        float top = 0.5f + mConfig.center.y();
        float bottom = top - 1.f;

        // Define the menu quad
        osg::Vec3 top_left(left, 1, top);
        osg::Vec3 bottom_left(left, 1, bottom);
        osg::Vec3 bottom_right(right, 1, bottom);
        osg::Vec3 top_right(right, 1, top);
        (*vertices)[0] = bottom_left;
        (*vertices)[1] = top_left;
        (*vertices)[2] = bottom_right;
        (*vertices)[3] = top_right;
        mGeometry->setVertexArray(vertices);
        (*texCoords)[0].set(0.0f, 0.0f);
        (*texCoords)[1].set(0.0f, 1.0f);
        (*texCoords)[2].set(1.0f, 0.0f);
        (*texCoords)[3].set(1.0f, 1.0f);
        mGeometry->setTexCoordArray(0, texCoords);
        (*normals)[0].set(0.0f, -1.0f, 0.0f);
        mGeometry->setNormalArray(normals, osg::Array::BIND_OVERALL);
        // TODO: Just use GL_TRIANGLES
        mGeometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_STRIP, 0, 4));
        mGeometry->setDataVariance(osg::Object::STATIC);
        mGeometry->setSupportsDisplayList(false);
        mGeometry->setName("VRGUILayer");

        // Create the camera that will render the menu texture
        std::string filter = mLayerName;
        if (!mConfig.extraLayers.empty())
            filter = filter + ";" + mConfig.extraLayers;
        mGUICamera = new GUICamera(config.pixelResolution.x(), config.pixelResolution.y(), config.backgroundColor);
        osgMyGUI::RenderManager& renderManager = static_cast<osgMyGUI::RenderManager&>(MyGUI::RenderManager::getInstance());
        mMyGUICamera = renderManager.createGUICamera(osg::Camera::NESTED_RENDER, filter);
        mGUICamera->setScene(mMyGUICamera);

        // Define state set that allows rendering with transparency
        osg::StateSet* stateSet = mGeometry->getOrCreateStateSet();
        auto texture = menuTexture();
        texture->setName("diffuseMap");
        stateSet->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);

        osg::ref_ptr<osg::Material> mat = new osg::Material;
        mat->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
        stateSet->setAttribute(mat);

        // Position in the game world
        mTransform->setScale(osg::Vec3(extent_units.x(), 1.f, extent_units.y()));
        mTransform->addChild(mGeometry);

        // Add to scene graph
        mGeometryRoot->addChild(mTransform);
        mCameraRoot->addChild(mGUICamera);

        // Edit offset to account for priority
        if (!mConfig.sideBySide)
        {
            mConfig.offset.y() -= 0.001f * static_cast<float>(mConfig.priority);
        }

        //mTransform->addUpdateCallback(new LayerUpdateCallback(this));

        auto* tm = Environment::get().getTrackingManager();
        mTrackingPath = tm->stringToVRPath(mConfig.trackingPath);
        tm->bind(this, "uisource");
    }

    VRGUILayer::~VRGUILayer()
    {
        mGeometryRoot->removeChild(mTransform);
        mCameraRoot->removeChild(mGUICamera);
    }
    osg::Camera* VRGUILayer::camera()
    {
        return mGUICamera.get();
    }

    osg::ref_ptr<osg::Texture2D> VRGUILayer::menuTexture()
    {
        if (mGUICamera)
            return mGUICamera->getTexture();
        return nullptr;
    }

    void VRGUILayer::setAngle(float angle)
    {
        mRotation = osg::Quat{ angle, osg::Z_AXIS };
        updatePose();
    }

    void VRGUILayer::onTrackingUpdated(VRTrackingSource& source, DisplayTime predictedDisplayTime)
    {
        auto tp = source.getTrackingPose(predictedDisplayTime, mTrackingPath);
        if (!!tp.status)
        {
            mTrackedPose = tp.pose;
            updatePose();
        }

        update();
    }

    void VRGUILayer::updatePose()
    {

        auto orientation = mRotation * mTrackedPose.orientation;

        if(mLayerName == "StatusHUD" || mLayerName == "VirtualKeyboard")
        {
            orientation = osg::Quat(osg::PI_2, osg::Vec3(0, 0, 1)) * orientation;
        }

        // Orient the offset and move the layer
        auto position = mTrackedPose.position + orientation * mConfig.offset * Constants::UnitsPerMeter;

        mTransform->setAttitude(orientation);
        mTransform->setPosition(position);
    }

    void VRGUILayer::updateRect()
    {
        auto viewSize = MyGUI::RenderManager::getInstance().getViewSize();
        mRealRect.left = 1.f;
        mRealRect.top = 1.f;
        mRealRect.right = 0.f;
        mRealRect.bottom = 0.f;
        float realWidth = static_cast<float>(viewSize.width);
        float realHeight = static_cast<float>(viewSize.height);
        for (auto* widget : mWidgets)
        {
            auto rect = widget->mMainWidget->getAbsoluteRect();
            mRealRect.left = std::min(static_cast<float>(rect.left) / realWidth, mRealRect.left);
            mRealRect.top = std::min(static_cast<float>(rect.top) / realHeight, mRealRect.top);
            mRealRect.right = std::max(static_cast<float>(rect.right) / realWidth, mRealRect.right);
            mRealRect.bottom = std::max(static_cast<float>(rect.bottom) / realHeight, mRealRect.bottom);
        }

        // Some widgets don't capture the full visual
        if (mLayerName == "JournalBooks")
        {
            mRealRect.left = 0.f;
            mRealRect.top = 0.f;
            mRealRect.right = 1.f;
            mRealRect.bottom = 1.f;
        }

        if (mLayerName == "Notification")
        {
            // The latest widget for notification is always the top one
            // So i just stretch the rectangle to the bottom.
            mRealRect.bottom = 1.f;
        }
    }

    void VRGUILayer::update()
    {
        if (mConfig.sideBySide)
        {
            // The side-by-side windows are also the resizable windows.
            // Stretch according to config
            // This genre of layer should only ever have 1 widget as it will cover the full layer
            auto* widget = mWidgets.front();
            auto* myGUIWindow = dynamic_cast<MyGUI::Window*>(widget->mMainWidget);
            auto* windowBase = dynamic_cast<MWGui::WindowBase*>(widget);
            if (windowBase && myGUIWindow)
            {
                auto w = mConfig.myGUIViewSize.x();
                auto h = mConfig.myGUIViewSize.y();
                windowBase->setCoordf(0.f, 0.f, w, h);
                windowBase->onWindowResize(myGUIWindow);
            }
        }
        updateRect();

        float w = 0.f;
        float h = 0.f;
        for (auto* widget : mWidgets)
        {
            w = std::max(w, (float)widget->mMainWidget->getWidth());
            h = std::max(h, (float)widget->mMainWidget->getHeight());
        }

        // Pixels per unit
        float res = static_cast<float>(mConfig.spatialResolution) / Constants::UnitsPerMeter;

        if (mConfig.sizingMode == SizingMode::Auto)
        {
            mTransform->setScale(osg::Vec3(w / res, 1.f, h / res));
        }
        if (mLayerName == "Notification")
        {
            auto viewSize = MyGUI::RenderManager::getInstance().getViewSize();
            h = (1.f - mRealRect.top) * static_cast<float>(viewSize.height);
            mTransform->setScale(osg::Vec3(w / res, 1.f, h / res));
        }

        // Convert from [0,1] range to [-1,1]
        float menuLeft = mRealRect.left * 2. - 1.;
        float menuRight = mRealRect.right * 2. - 1.;
        // Opposite convention 
        float menuBottom = (1.f - mRealRect.bottom) * 2. - 1.;
        float menuTop = (1.f - mRealRect.top) * 2.f - 1.;

        if(mLayerName == "InputBlocker")
            mMyGUICamera->setProjectionMatrixAsOrtho2D(menuRight, menuLeft, menuTop, menuBottom);
        else
            mMyGUICamera->setProjectionMatrixAsOrtho2D(menuLeft, menuRight, menuBottom, menuTop);
    }

    void
        VRGUILayer::insertWidget(
            MWGui::Layout* widget)
    {
        for (auto* w : mWidgets)
            if (w == widget)
                return;
        mWidgets.push_back(widget);
    }

    void
        VRGUILayer::removeWidget(
            MWGui::Layout* widget)
    {
        for (auto it = mWidgets.begin(); it != mWidgets.end(); it++)
        {
            if (*it == widget)
            {
                mWidgets.erase(it);
                return;
            }
        }
    }

    class VRGUIManagerUpdateCallback : public osg::Callback
    {
    public:
        VRGUIManagerUpdateCallback(VRGUIManager* manager)
            : mManager(manager)
        {

        }

        bool run(osg::Object* object, osg::Object* data)
        {
            mManager->update();
            return traverse(object, data);
        }

    private:
        VRGUIManager* mManager;
    };

    static const LayerConfig createDefaultConfig(int priority, bool background = true, SizingMode sizingMode = SizingMode::Auto, std::string extraLayers = "Popup")
    {
        return LayerConfig{
            priority,
            false, // side-by-side
            background ? osg::Vec4{0.f,0.f,0.f,.75f} : osg::Vec4{}, // background
            osg::Vec3(0.f,0.66f,-.25f), // offset
            osg::Vec2(0.f,0.f), // center (model space)
            osg::Vec2(1.f, 1.f), // extent (meters)
            1024, // Spatial resolution (pixels per meter)
            osg::Vec2i(2048,2048), // Texture resolution
            osg::Vec2(1,1),
            sizingMode,
            "/ui/input/stationary/pose",
            extraLayers
        };
    }

    static const float sSideBySideRadius = 1.f;
    static const float sSideBySideAzimuthInterval = -osg::PI_4;

    static const LayerConfig createSideBySideConfig(int priority)
    {
        LayerConfig config = createDefaultConfig(priority, true, SizingMode::Fixed, "");
        config.sideBySide = true;
        config.offset = osg::Vec3(0.f, sSideBySideRadius, -.25f);
        config.extent = radiusAngleWidth(sSideBySideRadius, sSideBySideAzimuthInterval);
        config.myGUIViewSize = osg::Vec2(0.70f, 0.70f);
        return config;
    };

    static osg::Vec3 gLeftHudOffsetTop = osg::Vec3(-0.200f, -.05f, .066f);
    static osg::Vec3 gLeftHudOffsetWrist = osg::Vec3(-0.200f, -.090f, -.033f);

    void VRGUIManager::setGeometryRoot(osg::Group* root)
    {
        mGeometriesRootNode->removeChild(mGeometries);
        mGeometriesRootNode = root;
        mGeometriesRootNode->addChild(mGeometries);
    }

    void VRGUIManager::setCameraRoot(osg::Group* root)
    {
        mGUICamerasRootNode->removeChild(mGUICameras);
        mGUICamerasRootNode = root;
        mGUICamerasRootNode->addChild(mGUICameras);
    }

    VRGUIManager::VRGUIManager(
        osg::ref_ptr<osgViewer::Viewer> viewer,
        Resource::ResourceSystem* resourceSystem,
        osg::Group* rootNode)
        : mOsgViewer(viewer)
        , mResourceSystem(resourceSystem)
        , mGeometriesRootNode(rootNode)
        , mGUICamerasRootNode(rootNode)
        , mUiTracking(new VRGUITracking("pcworld"))
    {
        mGeometries->setName("VR GUI Geometry Root");
        mGeometries->setUpdateCallback(new VRGUIManagerUpdateCallback(this));
        mGeometries->setNodeMask(MWRender::VisMask::Mask_3DGUI);
        mGeometriesRootNode->addChild(mGeometries);

        auto stateSet = mGeometries->getOrCreateStateSet();
        stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
        stateSet->setAttributeAndModes(new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
        // assign large value to effectively turn off fog
        // shaders don't respect glDisable(GL_FOG)
        osg::ref_ptr<osg::Fog> fog(new osg::Fog);
        fog->setStart(10000000);
        fog->setEnd(10000000);
        stateSet->setAttributeAndModes(fog, osg::StateAttribute::OFF);

        osg::ref_ptr<osg::LightModel> lightmodel = new osg::LightModel;
        lightmodel->setAmbientIntensity(osg::Vec4(1.0, 1.0, 1.0, 1.0));
        stateSet->setAttributeAndModes(lightmodel, osg::StateAttribute::ON);

        SceneUtil::ShadowManager::disableShadowsForStateSet(stateSet);
        mGeometries->setStateSet(stateSet);

        mGUICameras->setName("VR GUI Cameras Root");
        mGUICameras->setNodeMask(MWRender::VisMask::Mask_3DGUI);
        mGUICamerasRootNode->addChild(mGUICameras);

        LayerConfig defaultConfig = createDefaultConfig(1);
        LayerConfig loadingScreenConfig = createDefaultConfig(1, true, SizingMode::Fixed, "Menu");
        LayerConfig mainMenuConfig = createDefaultConfig(1, true);
        LayerConfig journalBooksConfig = createDefaultConfig(2, false, SizingMode::Fixed);
        LayerConfig defaultWindowsConfig = createDefaultConfig(3, true);
        LayerConfig videoPlayerConfig = createDefaultConfig(4, true, SizingMode::Fixed);
        LayerConfig messageBoxConfig = createDefaultConfig(6, false, SizingMode::Auto);;
        LayerConfig notificationConfig = createDefaultConfig(7, false, SizingMode::Fixed);
        LayerConfig listBoxConfig = createDefaultConfig(10, true);

        LayerConfig statsWindowConfig = createSideBySideConfig(0);
        LayerConfig inventoryWindowConfig = createSideBySideConfig(1);
        LayerConfig spellWindowConfig = createSideBySideConfig(2);
        LayerConfig mapWindowConfig = createSideBySideConfig(3);
        LayerConfig inventoryCompanionWindowConfig = createSideBySideConfig(4);
        LayerConfig dialogueWindowConfig = createSideBySideConfig(5);

        osg::Vec3 leftHudOffset = gLeftHudOffsetWrist;

        std::string leftHudSetting = Settings::Manager::getString("left hand hud position", "VR");
        if (Misc::StringUtils::ciEqual(leftHudSetting, "top"))
            leftHudOffset = gLeftHudOffsetTop;

        osg::Vec3 vkeyboardOffset = leftHudOffset + osg::Vec3(0,0.0001,0);

        LayerConfig virtualKeyboardConfig = LayerConfig{
            10,
            false,
            osg::Vec4{0.f,0.f,0.f,.75f},
            vkeyboardOffset, // offset (meters)
            osg::Vec2(0.f,0.5f), // center (model space)
            osg::Vec2(.25f, .25f), // extent (meters)
            2048, // Spatial resolution (pixels per meter)
            osg::Vec2i(2048,2048), // Texture resolution
            osg::Vec2(1,1),
            SizingMode::Auto,
            "/user/hand/left/input/aim/pose",
            ""
        };
        LayerConfig statusHUDConfig = LayerConfig
        {
            0,
            false, // side-by-side
            osg::Vec4{}, // background
            leftHudOffset, // offset (meters)
            osg::Vec2(0.f,0.5f), // center (model space)
            osg::Vec2(.1f, .1f), // extent (meters)
            1024, // resolution (pixels per meter)
            osg::Vec2i(1024,1024),
            defaultConfig.myGUIViewSize,
            SizingMode::Auto,
            "/user/hand/left/input/aim/pose",
            ""
        };

        LayerConfig popupConfig = LayerConfig
        {
            0,
            false, // side-by-side
            osg::Vec4{0.f,0.f,0.f,0.f}, // background
            osg::Vec3(-0.025f,-.200f,.066f), // offset (meters)
            osg::Vec2(0.f,0.5f), // center (model space)
            osg::Vec2(.1f, .1f), // extent (meters)
            1024, // resolution (pixels per meter)
            osg::Vec2i(2048,2048),
            defaultConfig.myGUIViewSize,
            SizingMode::Auto,
            "/user/hand/right/input/aim/pose",
            ""
        };

        mLayerConfigs = std::map<std::string, LayerConfig>
        {
            {"DefaultConfig", defaultConfig},
            {"StatusHUD", statusHUDConfig},
            {"Tooltip", popupConfig},
            {"JournalBooks", journalBooksConfig},
            {"InventoryCompanionWindow", inventoryCompanionWindowConfig},
            {"InventoryWindow", inventoryWindowConfig},
            {"SpellWindow", spellWindowConfig},
            {"MapWindow", mapWindowConfig},
            {"StatsWindow", statsWindowConfig},
            {"DialogueWindow", dialogueWindowConfig},
            {"MessageBox", messageBoxConfig},
            {"Windows", defaultWindowsConfig},
            {"ListBox", listBoxConfig},
            {"MainMenu", mainMenuConfig},
            {"Notification", notificationConfig},
            {"InputBlocker", videoPlayerConfig},
            {"Menu", videoPlayerConfig},
            {"LoadingScreen", loadingScreenConfig},
            {"VirtualKeyboard", virtualKeyboardConfig},
        };
    }

    VRGUIManager::~VRGUIManager(void)
    {
    }

    static std::set<std::string> layerBlacklist =
    {
        "Overlay",
        "AdditiveOverlay",
    };

    void VRGUIManager::updateSideBySideLayers()
    {
        // Nothing to update
        if (mSideBySideLayers.size() == 0)
            return;

        std::sort(mSideBySideLayers.begin(), mSideBySideLayers.end(), [](const auto& lhs, const auto& rhs) { return *lhs < *rhs; });

        int n = mSideBySideLayers.size();

        float span = sSideBySideAzimuthInterval * static_cast<float>(n - 1); // zero index, places lone layers straight ahead
        float low = -span / 2;

        for (unsigned i = 0; i < mSideBySideLayers.size(); i++)
        {
            mSideBySideLayers[i]->setAngle(low + static_cast<float>(i) * sSideBySideAzimuthInterval);
        }
    }

    void VRGUIManager::insertLayer(const std::string& name)
    {
        LayerConfig config{};
        auto configIt = mLayerConfigs.find(name);
        if (configIt != mLayerConfigs.end())
        {
            config = configIt->second;
        }
        else
        {
            Log(Debug::Warning) << "Layer " << name << " has no configuration, using default";
            config = mLayerConfigs["DefaultConfig"];
        }

        auto layer = std::shared_ptr<VRGUILayer>(new VRGUILayer(
            mGeometries,
            mGUICameras,
            name,
            config,
            this
        ));
        mLayers[name] = layer;

        layer->mGeometry->setUserData(new VRGUILayerUserData(mLayers[name]));

        if (config.sideBySide)
        {
            mSideBySideLayers.push_back(layer);
            updateSideBySideLayers();
        }

        Resource::SceneManager* sceneManager = mResourceSystem->getSceneManager();
        sceneManager->recreateShaders(layer->mGeometry);
    }

    void VRGUIManager::insertWidget(MWGui::Layout* widget)
    {
        auto* layer = widget->mMainWidget->getLayer();
        auto name = layer->getName();

        auto it = mLayers.find(name);
        if (it == mLayers.end())
        {
            insertLayer(name);
            it = mLayers.find(name);
            if (it == mLayers.end())
            {
                Log(Debug::Error) << "Failed to insert layer " << name;
                return;
            }
        }

        it->second->insertWidget(widget);

        if (it->second.get() != mFocusLayer)
            setPick(widget, false);
    }

    void VRGUIManager::removeLayer(const std::string& name)
    {
        auto it = mLayers.find(name);
        if (it == mLayers.end())
            return;

        auto layer = it->second;

        for (auto it2 = mSideBySideLayers.begin(); it2 < mSideBySideLayers.end(); it2++)
        {
            if (*it2 == layer)
            {
                mSideBySideLayers.erase(it2);
                updateSideBySideLayers();
            }
        }

        if (it->second.get() == mFocusLayer)
            setFocusLayer(nullptr);

        mLayers.erase(it);
    }

    void VRGUIManager::removeWidget(MWGui::Layout* widget)
    {
        auto* layer = widget->mMainWidget->getLayer();
        auto name = layer->getName();

        auto it = mLayers.find(name);
        if (it == mLayers.end())
        {
            return;
        }

        it->second->removeWidget(widget);
        if (it->second->widgetCount() == 0)
        {
            removeLayer(name);
        }
    }

    void VRGUIManager::setVisible(MWGui::Layout* widget, bool visible)
    {
        auto* layer = widget->mMainWidget->getLayer();
        auto name = layer->getName();

        if (layerBlacklist.find(name) != layerBlacklist.end())
        {
            // Never pick an invisible layer
            setPick(widget, false);
            return;
        }

        if (visible)
            insertWidget(widget);
        else
            removeWidget(widget);
    }

    void VRGUIManager::updateTracking()
    {
        mUiTracking->resetStationaryPose();
    }

    bool VRGUIManager::updateFocus()
    {
        auto* world = MWBase::Environment::get().getWorld();
        if (world)
        {
            auto& pointer = world->getUserPointer();
            if (pointer.getPointerTarget().mHit)
            {
                std::shared_ptr<VRGUILayer> newFocusLayer = nullptr;
                auto* node = pointer.getPointerTarget().mHitNode;
                if (node->getName() == "VRGUILayer")
                {
                    VRGUILayerUserData* userData = static_cast<VRGUILayerUserData*>(node->getUserData());
                    newFocusLayer = userData->mLayer.lock();
                }

                if (newFocusLayer && newFocusLayer->mLayerName != "Notification")
                {
                    setFocusLayer(newFocusLayer.get());
                    computeGuiCursor(pointer.getPointerTarget().mHitPointLocal);
                    return true;
                }
            }
        }
        return false;
    }

    void VRGUIManager::update()
    {
        auto xr = MWVR::Environment::get().getManager();
        if (xr)
            if (!xr->appShouldRender())
                updateTracking();
    }

    void VRGUIManager::setFocusLayer(VRGUILayer* layer)
    {
        if (layer == mFocusLayer)
            return;

        if (mFocusLayer)
        {
            if (!mFocusLayer->mWidgets.empty())
                setPick(mFocusLayer->mWidgets.front(), false);
        }
        mFocusLayer = layer;
        if (mFocusLayer)
        {
            if (!mFocusLayer->mWidgets.empty())
            {
                Log(Debug::Verbose) << "Set focus layer to " << mFocusLayer->mWidgets.front()->mMainWidget->getLayer()->getName();
                setPick(mFocusLayer->mWidgets.front(), true);
            }
        }
        else
        {
            Log(Debug::Verbose) << "Set focus layer to null";
        }
    }

    void VRGUIManager::setFocusWidget(MyGUI::Widget* widget)
    {
        // TODO: This relies on MyGUI internal functions and may break on any future version.
        if (widget == mFocusWidget)
            return;
        if (mFocusWidget)
            mFocusWidget->_riseMouseLostFocus(widget);
        if (widget)
            widget->_riseMouseSetFocus(mFocusWidget);
        mFocusWidget = widget;
    }

    void VRGUIManager::configUpdated(const std::string& layer)
    {
        auto it = mLayers.find(layer);
        if (it != mLayers.end())
        {
            it->second->mConfig = mLayerConfigs[layer];
        }
    }

    void VRGUIManager::notifyWidgetUnlinked(MyGUI::Widget* widget)
    {
        if (widget == mFocusWidget)
            mFocusWidget = nullptr;
    }

    bool VRGUIManager::injectMouseClick(bool onPress)
    {
        // TODO: This relies on a MyGUI internal functions and may break un any future version.
        if (mFocusWidget)
        {
            if(onPress)
                mFocusWidget->_riseMouseButtonClick();
            return true;
        }
        return false;
    }

    void VRGUIManager::processChangedSettings(const std::set<std::pair<std::string, std::string>>& changed)
    {
        for (Settings::CategorySettingVector::const_iterator it = changed.begin(); it != changed.end(); ++it)
        {
            if (it->first == "VR" && it->second == "left hand hud position")
            {
                std::string leftHudSetting = Settings::Manager::getString("left hand hud position", "VR");
                if (Misc::StringUtils::ciEqual(leftHudSetting, "top"))
                    mLayerConfigs["StatusHUD"].offset = gLeftHudOffsetTop;
                else
                    mLayerConfigs["StatusHUD"].offset = gLeftHudOffsetWrist;
                mLayerConfigs["VirtualKeyboard"].offset = mLayerConfigs["StatusHUD"].offset + osg::Vec3(0,0.0001,0);

                configUpdated("StatusHUD");
                configUpdated("VirtualKeyboard");
            }
        }
    }

    class Pickable
    {
    public:
        virtual void setPick(bool pick) = 0;
    };

    template <typename L>
    class PickLayer : public L, public Pickable
    {
    public:
        using L::L;

        void setPick(bool pick) override
        {
            L::mIsPick = pick;
        }
    };

    template <typename L>
    class MyFactory
    {
    public:
        using LayerType = L;
        using PickLayerType = PickLayer<LayerType>;
        using Delegate = MyGUI::delegates::CDelegate1<MyGUI::IObject*&>;
        static typename Delegate::IDelegate* getFactory()
        {
            return MyGUI::newDelegate(createFromFactory);
        }

        static void registerFactory()
        {
            MyGUI::FactoryManager::getInstance().registerFactory("Layer", LayerType::getClassTypeName(), getFactory());
        }

    private:
        static void createFromFactory(MyGUI::IObject*& _instance)
        {
            _instance = new PickLayerType();
        }
    };

    void VRGUIManager::registerMyGUIFactories()
    {
        MyFactory< MyGUI::OverlappedLayer >::registerFactory();
        MyFactory< MyGUI::SharedLayer >::registerFactory();
        MyFactory< osgMyGUI::AdditiveLayer >::registerFactory();
        MyFactory< osgMyGUI::AdditiveLayer >::registerFactory();
    }

    void VRGUIManager::setPick(MWGui::Layout* widget, bool pick)
    {
        auto* layer = widget->mMainWidget->getLayer();
        auto* pickable = dynamic_cast<Pickable*>(layer);
        if (pickable)
            pickable->setPick(pick);
    }

    void VRGUIManager::computeGuiCursor(osg::Vec3 hitPoint)
    {
        float x = 0;
        float y = 0;
        if (mFocusLayer)
        {
            osg::Vec2 bottomLeft = mFocusLayer->mConfig.center - osg::Vec2(0.5f, 0.5f);
            x = hitPoint.x() - bottomLeft.x();
            y = hitPoint.z() - bottomLeft.y();
            auto rect = mFocusLayer->mRealRect;
            auto viewSize = MyGUI::RenderManager::getInstance().getViewSize();
            float width = static_cast<float>(viewSize.width) * rect.width();
            float height = static_cast<float>(viewSize.height) * rect.height();
            float left = static_cast<float>(viewSize.width) * rect.left;
            float bottom = static_cast<float>(viewSize.height) * rect.bottom;
            x = width * x + left;
            y = bottom - height * y;
        }

        mGuiCursor.x() = (int)x;
        mGuiCursor.y() = (int)y;

        MyGUI::InputManager::getInstance().injectMouseMove((int)x, (int)y, 0);
        MWBase::Environment::get().getWindowManager()->setCursorActive(true);

        // The virtual keyboard must be interactive regardless of modals
        // This could be generalized with another config entry, but i don't think any other
        // widgets/layers need it so i'm hardcoding it for the VirtualKeyboard for now.
        if (
               mFocusLayer 
            && mFocusLayer->mLayerName == "VirtualKeyboard" 
            && MyGUI::InputManager::getInstance().isModalAny())
        {
            auto* widget = MyGUI::LayerManager::getInstance().getWidgetFromPoint((int)x, (int)y);
            setFocusWidget(widget);
        }
        else
            setFocusWidget(nullptr);

    }

    VRGUITracking::VRGUITracking(const std::string& source)
        : VRTrackingSource("uisource")
    {
        auto* tm = Environment::get().getTrackingManager();
        mSource = tm->getSource(source);
        mHeadPath = tm->stringToVRPath("/user/head/input/pose");
        mStationaryPath = tm->stringToVRPath("/ui/input/stationary/pose");
    }

    VRTrackingPose VRGUITracking::getTrackingPoseImpl(DisplayTime predictedDisplayTime, VRPath path, VRPath reference)
    {
        if (path == mStationaryPath)
            return mStationaryPose;
        return mSource->getTrackingPose(predictedDisplayTime, path, reference);
    }

    std::vector<VRPath> VRGUITracking::listSupportedTrackingPosePaths() const
    {
        auto paths = mSource->listSupportedTrackingPosePaths();
        paths.push_back(mStationaryPath);
        return paths;
    }

    void VRGUITracking::updateTracking(DisplayTime predictedDisplayTime)
    {
        if (mSource->availablePosesChanged())
            notifyAvailablePosesChanged();

        if (mShouldUpdateStationaryPose)
        {
            auto tp = mSource->getTrackingPose(predictedDisplayTime, mHeadPath);
            if (!!tp.status)
            {
                mShouldUpdateStationaryPose = false;
                mStationaryPose = tp;

                // Stationary UI elements should always be vertical
                auto axis = osg::Z_AXIS;
                osg::Quat vertical;
                auto local = mStationaryPose.pose.orientation * axis;
                vertical.makeRotate(local, axis);
                mStationaryPose.pose.orientation = mStationaryPose.pose.orientation * vertical;
            }
        }
    }

    void VRGUITracking::resetStationaryPose()
    {
        mShouldUpdateStationaryPose = true;
    }

}
