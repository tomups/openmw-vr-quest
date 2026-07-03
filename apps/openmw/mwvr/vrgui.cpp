#include "vrgui.hpp"
#include "vrinputmanager.hpp"

#include <algorithm>
#include <cmath>

#include "openxrinput.hpp"
#include "vrpointer.hpp"

#include <osg/BlendFunc>
#include <osg/ClipNode>
#include <osg/Depth>
#include <osg/Fog>
#include <osg/FrontFace>
#include <osg/LightModel>
#include <osg/Material>
#include <osg/Texture2D>

#include <osgViewer/Renderer>
#include <osgViewer/Viewer>

#ifdef __ANDROID__
#include <osgUtil/LineSegmentIntersector>
#endif

#include <components/misc/constants.hpp>
#include <components/myguiplatform/additivelayer.hpp>
#include <components/myguiplatform/myguirendermanager.hpp>
#include <components/myguiplatform/scalinglayer.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/rtt.hpp>
#include <components/sceneutil/shadow.hpp>
#include <components/sceneutil/visitor.hpp>
#include <components/shader/shadermanager.hpp>
#include <components/vr/rendertoswapchain.hpp>
#include <components/vr/session.hpp>
#include <components/vr/trackingmanager.hpp>
#include <components/vr/viewer.hpp>

#include "../mwrender/camera.hpp"
#include "../mwrender/renderingmanager.hpp"
#include "../mwrender/util.hpp"
#include "../mwrender/vismask.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwgui/windowbase.hpp"
#include "../mwinput/mousemanager.hpp"

#include <MyGUI_FactoryManager.h>
#include <MyGUI_ILayer.h>
#include <MyGUI_LayerManager.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_OverlappedLayer.h>
#include <MyGUI_SharedLayer.h>
#include <MyGUI_Widget.h>
#include <MyGUI_WidgetManager.h>
#include <MyGUI_Window.h>

namespace MWVR
{
    namespace
    {
        const static std::set<std ::string> resizableWindowLayers = { "InventoryCompanionWindow", "DialogueWindow",
            "InventoryWindow", "StatsWindow", "MapWindow", "SpellWindow" };

        static std::set<std::string> defaultPickableLayers = { "HUD_3D", "InventoryCompanionWindow", "InventoryWindow",
            "SpellWindow", "MapWindow", "StatsWindow", "DialogueWindow", "ServiceWindow", "JournalBooks", "Debug",
            "MainMenuBackground", "MainMenu", "Settings", "Console", "RadialMenu", "LoadingScreenBackground", "LoadingScreen", "Modal",
                  "ListBox", "Popup", "Video", "InputBlocker", "VideoPlayer", "VirtualKeyboard", "Windows" };

#ifdef __ANDROID__
        // Quest: draw the co-visible "full screen" MyGUI layers into ONE shared FBO/texture/quad instead of
        // one RTT camera per panel -- the model used by other gl4es-based Quest ports (RTCWQuest draws its
        // whole 2D UI into a single shared framebuffer). Fewer FBO passes per frame through the fragile
        // gl4es FBO seam, correct back-to-front compositing for free (MyGUI layer order), and panels opened
        // over each other (Settings over MainMenu) share one surface. These layers already use
        // byte-identical LayerConfig (opacity 0.75, autoSize false, no space -- see readConfig()), so
        // merging is safe and general -- new full-screen layers can join this set without other changes.
        // "Popup" is the MyGUI layer ComboBox drop-lists open on (layer="Popup" in MW_ComboBox); it is
        // never backed by a tracked MWGui::Layout, so as a standalone quad it would never turn visible --
        // sharing the menu canvas makes drop-lists composite over Settings exactly like flat-screen.
        // "Modal" (savegame/message dialogs) and "ListBox" (VrListBox) must also share the canvas: the
        // canvas layer is named after its first-created member ("InputBlocker", map order) and inherits
        // that near-top layer priority, so a standalone Modal/ListBox quad always depth-sorts BEHIND the
        // canvas -- the load-game dialog opened invisibly behind the main menu. Inside the canvas, MyGUI
        // layer order (openmw_layers_vr.xml) composites them correctly above the menu layers.
        const std::set<std::string> sSharedMenuLayers
            = { "MainMenu", "Settings", "InputBlocker", "Video", "LoadingScreen", "Popup", "Modal", "ListBox" };
#endif

        // Since the MODE stack cannot always inform us what should be in front, and MyGUI doesn't expose this
        // information, i have to manually re-compute what the actual priorities are
        std::map<std::string, int> autoLayerPriorities(const std::vector<std::string>& layers)
        {
            std::map<std::string, int> res;
            int priority = 0;
            for (auto& layer : layers)
                res[layer] = priority++;
            return res;
        }
        std::map<std::string, int> layerPriorities = autoLayerPriorities({
            "FadeToBlack",
            "HitOverlay",
            "HUD",
            "InventoryCompanionWindow",
            "InventoryWindow",
            "SpellWindow",
            "MapWindow",
            "StatsWindow",
            "DialogueWindow",
            "ServiceWindow",
            "JournalBooks",
            "Windows",
            "Debug",
            "Tooltip",
            "Notification",
            "DragAndDrop",
            "DrowningBar",
            "MainMenuBackground",
            "MainMenu",
            "Settings",
            "Console",
            "RadialMenu",
            "LoadingScreenBackground",
            "LoadingScreen",
            "Modal",
            "ListBox",
            "Popup",
            "Video",
            "InputBlocker",
            "VideoPlayer",
            "SpaceOffsetRecording",
            "VirtualKeyboard",
            "Pointer",
        });

        int getLayerPriority(const std::string& layer)
        {
            auto it = layerPriorities.find(layer);
            if (it == layerPriorities.end())
                return 0;
            return it->second;
        }
    }

    // When making a circle of a given radius of equally wide planes separated by a given angle, what is the width
    // static osg::Vec2 radiusAngleWidth(float radius, float angleRadian)
    // {
    //     const float width = std::fabs(2.f * radius * tanf(angleRadian / 2.f));
    //     return osg::Vec2(width, width);
    // }

    class GUIRTT : public SceneUtil::RTTNode
    {
    public:
        GUIRTT(int width, int height, osg::Vec4 clearColor, osg::ref_ptr<osg::Node> scene)
            : RTTNode(width, height, 1, true, 0, StereoAwareness::Unaware, false)
            , mScene(scene)
            , mClearColor(clearColor)
            , mTexWidth(width)
            , mTexHeight(height)
        {
        }

        void setDefaults(osg::Camera* camera) override
        {
            camera->setCullingActive(false);

            // Make the texture just a little transparent to feel more natural in the game world.
            camera->setClearColor(mClearColor);
            setColorBufferInternalFormat(GL_RGBA8);

            camera->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
            camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
            setName("GUICamera");
            camera->setName("GUICamera_");

            camera->setCullMask(MWRender::Mask_GUI);
            camera->setCullMaskLeft(MWRender::Mask_GUI);
            camera->setCullMaskRight(MWRender::Mask_GUI);

            // Although this is strictly speaking a RenderToTexture node, we cannot use the Mask_RenderToTexture mask
            // since it would cull this inappropriately.
            camera->setNodeMask(MWRender::Mask_3DGUI);

            // No need for Update traversal since the scene is already updated as part of the main scene graph
            setUpdateCallback(new MWRender::NoTraverseCallback);

            camera->addChild(mScene);

            auto* stateset = camera->getOrCreateStateSet();
            // Do not want to waste time on shadows when generating the GUI texture
            // TODO: We may be here before the shadow manager has been instantiated
            // SceneUtil::ShadowManager::instance().disableShadowsForStateSet(*stateset);

            // Put rendering as early as possible
            stateset->setRenderBinDetails(-1, "RenderBin");
        }

        osg::ref_ptr<osg::Node> mScene;
        osg::Vec4 mClearColor;
        int mTexWidth;
        int mTexHeight;
    };

    osg::ref_ptr<osg::Geometry> VRGUILayer::createLayerGeometry(osg::ref_ptr<osg::StateSet> stateset)
    {
        float left = mConfig->center.x() - 0.5f;
        float right = left + 1.f;
        float top = 0.5f + mConfig->center.y();
        float bottom = top - 1.f;

        osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();

        // Define the menu quad
        osg::Vec3 topLeft(left, 1, top);
        osg::Vec3 bottomLeft(left, 1, bottom);
        osg::Vec3 bottomRight(right, 1, bottom);
        osg::Vec3 topRight(right, 1, top);

        osg::ref_ptr<osg::Vec3Array> vertices{ new osg::Vec3Array(4) };
        (*vertices)[0] = bottomLeft;
        (*vertices)[1] = topLeft;
        (*vertices)[2] = bottomRight;
        (*vertices)[3] = topRight;
        geometry->setVertexArray(vertices);

        osg::ref_ptr<osg::Vec2Array> texCoords{ new osg::Vec2Array(4) };
        (*texCoords)[0].set(0.0f, 0.0f);
        (*texCoords)[1].set(0.0f, 1.0f);
        (*texCoords)[2].set(1.0f, 0.0f);
        (*texCoords)[3].set(1.0f, 1.0f);
        geometry->setTexCoordArray(0, texCoords);

        osg::ref_ptr<osg::Vec3Array> normals{ new osg::Vec3Array(1) };
        (*normals)[0].set(0.0f, -1.0f, 0.0f);
        geometry->setNormalArray(normals, osg::Array::BIND_OVERALL);

#ifdef __ANDROID__
        // gl4es needs an explicit colour array: without one it reads stale/aliased vertex data as the
        // colour, tinting the quad's two triangles differently (a green/red diagonal seam). Force white.
        osg::ref_ptr<osg::Vec4Array> colors{ new osg::Vec4Array(4) };
        (*colors)[0] = (*colors)[1] = (*colors)[2] = (*colors)[3] = osg::Vec4(1.f, 1.f, 1.f, 1.f);
        geometry->setColorArray(colors, osg::Array::BIND_PER_VERTEX);
#endif

        // TODO: Just use GL_TRIANGLES
        geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_STRIP, 0, 4));
        geometry->setDataVariance(osg::Object::STATIC);
        geometry->setSupportsDisplayList(false);
        geometry->setName("VRGUILayer");
        geometry->setStateSet(stateset);

        return geometry;
    }

    /// \brief osg user data used to refer back to VRGUILayer objects when intersecting with the scene graph.
    class VRGUILayerUserData : public osg::Referenced
    {
    public:
        VRGUILayerUserData(VRGUILayer* layer)
            : mLayer(layer)
        {
        }

        VRGUILayer* mLayer;
    };

    class CullVRGUILayerCallback
        : public SceneUtil::NodeCallback<CullVRGUILayerCallback, osg::Transform*, osgUtil::CullVisitor*>
    {
    public:
        CullVRGUILayerCallback(VRGUILayer* layer)
            : mLayer(layer)
        {
        }

        void operator()(osg::Transform* node, osgUtil::CullVisitor* cv)
        {
            (void)node;
            mLayer->cull(cv);
        }

        VRGUILayer* mLayer = nullptr;
    };

    VRGUILayer::VRGUILayer(osg::ref_ptr<osg::Group> geometryRoot, osg::ref_ptr<osg::Group> cameraRoot,
        std::string layerName, VRGUIManager* parent)
        : mLayerName(layerName)
        , mMemberLayerNames{ layerName }
        , mGeometryRoot(geometryRoot)
        , mCameraRoot(cameraRoot)
        , mPickable()
        , mVrLayer()
    {
        setPickable(defaultPickableLayers.contains(layerName));
    }

    void VRGUILayer::setConfig(const LayerConfig& config)
    {
        // Only rebuild the layer (RTT camera + texture + geometry) when the config actually changes.
        // setConfig is called repeatedly with an unchanged config; rebuilding every frame recreated the
        // RTT texture each frame, so the display quad sampled a freshly-created, not-yet-rendered
        // (uninitialized) texture -> garbled vertical stripes on Quest/gl4es.
        // Only rebuild when something that actually requires recreating the RTT/geometry changes
        // (pixelResolution or space). Other per-call config churn (and identical re-sets) must NOT
        // trigger a rebuild: each rebuild makes a fresh RTT camera+texture, and rebuilding repeatedly
        // leaves multiple RTT/GUICamera pairs so the display samples a different texture than the menu
        // renders into -> blank/garbled menu on Quest/gl4es.
        const bool changed = !mConfig || mConfig->pixelResolution != config.pixelResolution
            || mConfig->space != config.space;
        if (changed)
            mDirty = true;
        mConfig = config;
    }

    void VRGUILayer::clear()
    {
        removeFromSceneGraph();
    }

    void VRGUILayer::addLuaElement(const LuaUi::Element* element)
    {
        mLuaElements.push_back(element);
    }

    void VRGUILayer::removeLuaElement(const LuaUi::Element* element)
    {
        std::erase_if(mLuaElements, [element](const auto& lhs) { return lhs == element; });
    }

    void VRGUILayer::clearLua()
    {
        mLuaElements.clear();
    }

    void VRGUILayer::setPickable(bool pickable)
    {
        mPickable = pickable;
        if (pickable)
            mTransform->setNodeMask(MWRender::Mask_3DGUI);
        else
            mTransform->setNodeMask(MWRender::Mask_3DGUI_NonIntersectable);
    }

    VRGUILayer::~VRGUILayer()
    {
        clear();
    }

    // void VRGUILayer::setAngle(float angle)
    //{
    //     // mRotation = osg::Quat{ angle, osg::Z_AXIS };
    //     updatePose();
    // }

    osg::Texture2D* VRGUILayer::colorTexture() const
    {
#ifdef __ANDROID__
        return mGUIColorTexture.get();
#else
        if (!mGUIRTT)
            return nullptr;
        return static_cast<osg::Texture2D*>(static_cast<GUIRTT*>(mGUIRTT.get())->getColorTexture(nullptr));
#endif
    }

    bool VRGUILayer::updateLayer()
    {
        // TODO: Do i actually need to do anything here?
        return mVisible && mVrLayer;
    }

    void VRGUILayer::updateVisibility()
    {
        bool visible = mForceVisible || mWidgets.size() > 0;
        for (const auto* luaWidget : mLuaElements)
        {
            if (luaWidget && luaWidget->mRoot)
                visible = visible || luaWidget->mRoot->isVisible();
        }

        if (visible == mVisible)
            return;

        mVisible = visible;
        if (mVisible)
            addToSceneGraph();
        else
            removeFromSceneGraph();
    }

    void VRGUILayer::blitLayer(osg::RenderInfo& info)
    {
        if (mVrLayer && mConfig)
        {
            auto* state = info.getState();
            auto texture = colorTexture();
            mVrLayer->colorSwapchain->beginFrame(state->getGraphicsContext());
            auto glImage = mVrLayer->colorSwapchain->image()->glImage();

            int width = mConfig->pixelResolution.x();
            int height = mConfig->pixelResolution.y();

            // TODO: Definitely cache fbos when we're done checking that it WORKS
            osg::ref_ptr<osg::FrameBufferObject> targetFbo = new osg::FrameBufferObject();
            auto colorAttachment
                = Stereo::createLayerAttachmentFromHandle(state, glImage, GL_TEXTURE_2D, width, height, 0);
            targetFbo->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER, colorAttachment);
            targetFbo->apply(*state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);

            osg::ref_ptr<osg::FrameBufferObject> sourceFbo = new osg::FrameBufferObject();
            sourceFbo->setAttachment(
                osg::FrameBufferObject::BufferComponent::COLOR_BUFFER, osg::FrameBufferAttachment(texture));
            sourceFbo->apply(*state, osg::FrameBufferObject::READ_FRAMEBUFFER);

            auto* gl = osg::GLExtensions::Get(state->getContextID(), false);
            gl->glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
            gl->glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
            mVrLayer->colorSwapchain->endFrame(state->getGraphicsContext());
        }
    }

    void VRGUILayer::updatePose()
    {
        if (!mConfig)
            return;

        Stereo::Pose pose = mPose;

        if (mSpace)
        {
            auto tp = mSpace->locateInWorld();
            if (!tp.status)
            {
                if (!mSpaceIsLost)
                    Log(Debug::Warning) << "Layer " << mLayerName
                                        << " unable to track space: " << static_cast<int>(tp.status);
                mSpaceIsLost = true;
                return;
            }
            mSpaceIsLost = false;
            pose = tp.pose + pose;
        }

        if (auto priority = getLayerPriority(mLayerName))
        {
            // Implement priority by moving this UI quad slightly towards the viewer based on priority.
            auto view = OpenXRInput::instance().getSpace(OpenXRInput::DefaultReferenceSpaceView);
            auto diff = view->locateInWorld().pose.position - pose.position;
            diff = diff / 10000.f;
            pose.position = pose.position + diff * static_cast<float>(priority);
        }

        if (mVrLayer)
        {
            mVrLayer->pose = pose;
        }
        else
        {
            mTransform->setAttitude(pose.orientation);
            mTransform->setPosition(pose.position.asMWUnits());
        }
    }

#ifdef __ANDROID__
    bool VRGUILayer::worldToCanvasUv(const osg::Vec3f& worldPoint, float& u, float& v) const
    {
        // Project the world point onto the quad's world corners. Robust to the quad's orientation --
        // the OSG local intersect point loses the quad's vertical component for our GUI quads.
        osg::Geometry* geom = mGeometries[0].get();
        osg::Vec3Array* verts = geom ? dynamic_cast<osg::Vec3Array*>(geom->getVertexArray()) : nullptr;
        auto mats = mTransform ? mTransform->getWorldMatrices() : osg::MatrixList();
        if (!verts || verts->size() < 4 || mats.empty())
            return false;
        const osg::Matrix& m = mats[0];
        // createLayerGeometry vertex order: [0]=bottomLeft [1]=topLeft [2]=bottomRight [3]=topRight
        osg::Vec3 bl = (*verts)[0] * m;
        osg::Vec3 tl = (*verts)[1] * m;
        osg::Vec3 br = (*verts)[2] * m;
        osg::Vec3 edgeU = br - bl; // left -> right
        osg::Vec3 edgeV = tl - bl; // bottom -> top
        osg::Vec3 d = worldPoint - bl;
        float lu = edgeU.length2();
        float lv = edgeV.length2();
        u = lu > 0.f ? (d * edgeU) / lu : 0.f;
        v = lv > 0.f ? (d * edgeV) / lv : 0.f;
        return true;
    }
#endif

    void VRGUILayer::updateRect()
    {
        auto viewSize = MyGUI::RenderManager::getInstance().getViewSize();
        mRect.left = viewSize.width;
        mRect.top = viewSize.height;
        mRect.right = 0;
        mRect.bottom = 0;

        for (auto* widget : mWidgets)
        {
            auto rect = widget->mMainWidget->getAbsoluteRect();
            mRect.left = std::min(rect.left, mRect.left);
            mRect.top = std::min(rect.top, mRect.top);
            mRect.right = std::max(rect.right, mRect.right);
            mRect.bottom = std::max(rect.bottom, mRect.bottom);
        }

        for (const auto& element : mLuaElements)
        {
            if (!element || !element->mRoot)
                continue;
            if (!element->mRoot->isVisible())
                continue;
            const auto* widget = element->mRoot->widget();
            if (!widget)
                continue;
            auto rect = widget->getAbsoluteRect();
            mRect.left = std::min(rect.left, mRect.left);
            mRect.top = std::min(rect.top, mRect.top);
            mRect.right = std::max(rect.right, mRect.right);
            mRect.bottom = std::max(rect.bottom, mRect.bottom);
        }

        // Some widgets don't capture the full visual
        if (mLayerName == "JournalBooks")
        {
            mRect.left = 0;
            mRect.top = 0;
            mRect.right = viewSize.width;
            mRect.bottom = viewSize.height;
        }

        if (mLayerName == "Notification")
        {
            // The latest widget for notification is always the top one
            // So i just stretch the rectangle to the bottom.
            mRect.bottom = viewSize.height;
        }

        if (mLayerName == "InputBlocker" || mLayerName == "Video")
        {
            // These don't have widgets, so manually stretch them to the full rectangle
            mRect.left = 0;
            mRect.top = 0;
            mRect.right = viewSize.width;
            mRect.bottom = viewSize.height;
        }

#ifdef __ANDROID__
        // Before stretching the quad to the full canvas (below), remember where the widgets actually
        // are: picking uses this to let the pointer pass through the transparent parts of the quad
        // (VRGUIManager::castContentRay), e.g. so the meter-wide wrist-HUD quad can't block panels
        // behind it. Normalized canvas coords, MyGUI convention (y down); inverted/empty when the
        // layer has no widgets.
        mContentRealRect.left = std::clamp(static_cast<float>(mRect.left) / viewSize.width, 0.f, 1.f);
        mContentRealRect.top = std::clamp(static_cast<float>(mRect.top) / viewSize.height, 0.f, 1.f);
        mContentRealRect.right = std::clamp(static_cast<float>(mRect.right) / viewSize.width, 0.f, 1.f);
        mContentRealRect.bottom = std::clamp(static_cast<float>(mRect.bottom) / viewSize.height, 0.f, 1.f);

        // General fix for the Quest port: render every layer at the FULL canvas instead of cropping each
        // layer's quad to its widget bounding box. Upstream gives each MyGUI layer its own quad sized to its
        // bbox; that makes the background layer and the button layer separate, differently-sized, misaligned
        // quads (cropped background, buttons in the wrong place, cursor offset) -- and it breaks the same way
        // for every panel. With a full-canvas rect, all layers overlap aligned at full screen and the cursor
        // maps 1:1 to the MyGUI canvas, like the flat 2D path composites all layers.
        mRect.left = 0;
        mRect.top = 0;
        mRect.right = viewSize.width;
        mRect.bottom = viewSize.height;
#endif

        mRealRect.left = static_cast<float>(mRect.left) / static_cast<float>(viewSize.width);
        mRealRect.top = static_cast<float>(mRect.top) / static_cast<float>(viewSize.height);
        mRealRect.right = static_cast<float>(mRect.right) / static_cast<float>(viewSize.width);
        mRealRect.bottom = static_cast<float>(mRect.bottom) / static_cast<float>(viewSize.height);
    }

    void VRGUILayer::update(osg::NodeVisitor* nv)
    {
        if (!mConfig)
        {
            clear();
            return;
        }

        updateVisibility();

        if (mDirty)
        {
            clear();

            auto extent_units = mConfig->extent * Constants::UnitsPerMeter;

            // Create the camera that will render the menu texture. mMemberLayerNames is normally just
            // {mLayerName}; on Android it may hold several MyGUI layer names sharing one FBO (sSharedMenuLayers).
            std::string filter;
            for (const auto& member : mMemberLayerNames)
                // Some layers have their own background layer
                filter += (filter.empty() ? std::string() : std::string(";")) + member + ";" + member + "Background";
            MyGUIPlatform::RenderManager& renderManager
                = static_cast<MyGUIPlatform::RenderManager&>(MyGUI::RenderManager::getInstance());
#ifdef __ANDROID__
            // Quest/gl4es: render MyGUI into a SINGLE standalone PRE_RENDER FBO camera. Upstream wraps a
            // NESTED_RENDER GUI camera inside an RTTNode camera; gl4es mishandles that camera-in-camera FBO
            // (the clear never executes and the menu is drawn into nothing -> uninitialised garbage). A plain
            // FBO camera is the proven pattern (RTCW/TBXR). PCVR keeps the original nested RTTNode path.
            mMyGUICamera = renderManager.createGUICamera(osg::Camera::PRE_RENDER, filter);
            {
                const int rw = mConfig->pixelResolution.x();
                const int rh = mConfig->pixelResolution.y();
                osg::ref_ptr<osg::Texture2D> guiTex = new osg::Texture2D;
                guiTex->setTextureSize(rw, rh);
                guiTex->setInternalFormat(GL_RGBA);
                guiTex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
                guiTex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
                guiTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
                guiTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
                mGUIColorTexture = guiTex;

                osg::Camera* cam = mMyGUICamera.get();
                cam->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
                cam->setRenderOrder(osg::Camera::PRE_RENDER);
                cam->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                // Clear to fully transparent so full-canvas layer quads composite (the 3dgui shader discards
                // alpha==0). Non-content areas then show the layer/scene behind instead of a black box.
                cam->setClearColor(osg::Vec4(0.f, 0.f, 0.f, 0.f));
                cam->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
                cam->attach(osg::Camera::COLOR_BUFFER, guiTex);
            }
            mGUIRTT = mMyGUICamera;
#else
            mMyGUICamera = renderManager.createGUICamera(osg::Camera::NESTED_RENDER, filter);

            auto* rttNode = new GUIRTT(mConfig->pixelResolution.x(), mConfig->pixelResolution.y(),
                osg::Vec4(0, 0, 0, mConfig->opacity), mMyGUICamera);
            mGUIRTT = rttNode;
#endif

            if (!mConfig->space.empty())
                mSpace = OpenXRInput::instance().getSpace(mConfig->space);

            if (mVrLayer)
            {
                mVrLayer->colorSwapchain = VR::Session::instance().createSwapchain(mConfig->pixelResolution.x(),
                    mConfig->pixelResolution.y(), 1, 1, VR::Swapchain::Attachment::Color, mLayerName + "Swapchain");
                mVrLayer->extent = mConfig->extent;
                mVrLayer->space = XR_NULL_HANDLE;
            }
            else
            {
                mStateset = new osg::StateSet;

                Shader::ShaderManager::DefineMap defineMap;
                Stereo::shaderStereoDefines(defineMap);
                auto& shaderManager
                    = MWBase::Environment::get().getResourceSystem()->getSceneManager()->getShaderManager();

                osg::ref_ptr<osg::Shader> vertexShader
                    = shaderManager.getShader("3dgui_vertex.glsl", defineMap, osg::Shader::VERTEX);
                osg::ref_ptr<osg::Shader> fragmentShader
                    = shaderManager.getShader("3dgui_fragment.glsl", defineMap, osg::Shader::FRAGMENT);

                if (vertexShader && fragmentShader)
                {
                    mStateset->setAttributeAndModes(shaderManager.getProgram(vertexShader, fragmentShader));
                }

#ifdef __ANDROID__
                osg::Texture2D* texture = mGUIColorTexture.get();
#else
                auto texture = rttNode->getColorTexture(nullptr);
#endif
                texture->setName("diffuseMap");
                mStateset->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);

                osg::ref_ptr<osg::Material> mat = new osg::Material;
                mat->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
                mStateset->setAttribute(mat);

                mStateset->setMode(GL_ALPHA_TEST, osg::StateAttribute::OFF);

#ifdef __ANDROID__
                // gl4es does not reliably apply the 3dgui program, so the quad falls back to fixed-function
                // and gets FLAT-shaded lighting from the (dim) scene lights -> a dark, diagonal two-triangle
                // seam. Force the quad unlit so it shows the menu texture at full brightness.
                mStateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED);
                mStateset->setMode(GL_FOG, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED);
#endif

                mGeometries[0] = createLayerGeometry(mStateset);
                mGeometries[0]->setUserData(new VRGUILayerUserData(this));
                mGeometries[1] = createLayerGeometry(mStateset);
                mGeometries[1]->setUserData(mGeometries[0]->getUserData());

                // Position in the game world
                mTransform->setScale(osg::Vec3(extent_units.x(), 1.f, extent_units.y()));
                mTransform->setCullCallback(new CullVRGUILayerCallback(this));
                mTransform->setCullingActive(false);
            }
            mDirty = false;
            if (mVisible)
                addToSceneGraph();
        }

        if (!mVrLayer)
        {
            if (mTransform->getNumChildren() > 0)
                mTransform->removeChild(0, mTransform->getNumChildren());

            auto index = nv->getFrameStamp()->getFrameNumber() % 2;
            auto* geometry = mGeometries[index].get();

            if (mVisible)
                mTransform->addChild(geometry);
        }

        updateRect();

        //// Pixels per unit
        float res = static_cast<float>(mConfig->spatialResolution) / Constants::UnitsPerMeter;
        float w = static_cast<float>(mRect.right - mRect.left);
        float h = static_cast<float>(mRect.bottom - mRect.top);

        if (mConfig->autoSize)
        {
            if (mVrLayer)
                mVrLayer->extent = osg::Vec2f(w / res, h / res) / Constants::UnitsPerMeter;
            else
                mTransform->setScale(osg::Vec3(w / res, 1.f, h / res));
        }
        if (mLayerName == "Notification")
        {
            auto viewSize = MyGUI::RenderManager::getInstance().getViewSize();
            h = (1.f - mRealRect.top) * static_cast<float>(viewSize.height);
            if (mVrLayer)
                mVrLayer->extent = osg::Vec2f(w / res, h / res) / Constants::UnitsPerMeter;
            else
                mTransform->setScale(osg::Vec3(w / res, 1.f, h / res));
        }

        if (mVrLayer)
        {
            VR::SubImage subImage;
            subImage.x = mRect.left;
            subImage.y = mRect.top;
            subImage.index = 0;
            subImage.width = mRect.right - mRect.left;
            subImage.height = mRect.bottom - mRect.top;
            mVrLayer->subImage = subImage;
        }
    }

    void VRGUILayer::insertWidget(MWGui::Layout* widget)
    {
        for (auto* w : mWidgets)
            if (w == widget)
                return;
        mWidgets.push_back(widget);
    }

    void VRGUILayer::removeWidget(MWGui::Layout* widget)
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

    void VRGUILayer::cull(osgUtil::CullVisitor* cv)
    {
        // Nothing to draw
        // Return without traversing
        if (!mVisible)
            return;

        if (!mVrLayer)
        {
            auto index = cv->getFrameStamp()->getFrameNumber() % 2;
            auto* geometry = mGeometries[index].get();

            osg::Vec2Array* texCoords = static_cast<osg::Vec2Array*>(geometry->getTexCoordArray(0));
            // The standalone GUI RTT camera produces a normal bottom-up OSG texture (on Android too, now
            // that we no longer use the nested gl4es RTTNode), so sample with the standard V flip.
            (*texCoords)[0].set(mRealRect.left, 1.f - mRealRect.bottom);
            (*texCoords)[1].set(mRealRect.left, 1.f - mRealRect.top);
            (*texCoords)[2].set(mRealRect.right, 1.f - mRealRect.bottom);
            (*texCoords)[3].set(mRealRect.right, 1.f - mRealRect.top);
            texCoords->dirty();

            geometry->accept(*cv);
        }
    }

    void VRGUILayer::removeFromSceneGraph()
    {
        mCameraRoot->removeChild(mGUIRTT);
        if (mVrLayer)
            VR::Viewer::instance().removeLayer(mVrLayer);
        else
            mGeometryRoot->removeChild(mTransform);
    }

    void VRGUILayer::addToSceneGraph()
    {
        mCameraRoot->addChild(mGUIRTT);
        if (mVrLayer)
            VR::Viewer::instance().insertLayer(mVrLayer);
        else
            mGeometryRoot->addChild(mTransform);
    }

    static const LayerConfig createDefaultConfig(bool background = true, bool autoSize = true)
    {
        return LayerConfig{ background ? 0.75f : 0.f, // background
                                                      // Stereo::Position::fromMeters(0.f, 0.66f, -.25f), // offset
            osg::Vec2(0.f, 0.f), // center (model space)
            osg::Vec2(1.f, 1.f), // extent (meters)
            1024, // Spatial resolution (pixels per meter)
            osg::Vec2i(1024, 768), // Texture resolution (4:3 to match the GUI canvas aspect)
            osg::Vec2(1, 1), autoSize, "" };
    }

    VRGUIManager* sManager = nullptr;
    VRGUIManager& VRGUIManager::instance()
    {
        assert(sManager);
        return *sManager;
    }

    class LayerUpdateCallback : public SceneUtil::NodeCallback<LayerUpdateCallback, osg::Node*>
    {
    public:
        LayerUpdateCallback(VRGUIManager* manager)
            : mManager(manager)
        {
        }

        void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            mManager->update(nv);
            traverse(node, nv);
        }

    private:
        VRGUIManager* mManager;
    };

    VRGUIManager::VRGUIManager(osg::Group* rootNode)
        : mRootNode(rootNode)
    {
        if (!sManager)
            sManager = this;
        else
            throw std::logic_error("Duplicated MWVR::VRGUIManager singleton");

        mGeometries->setName("VR GUI Geometry Root");
        mGeometries->setCullCallback(new LayerUpdateCallback(this));
        mGeometries->setNodeMask(MWRender::VisMask::Mask_3DGUI);
        mGeometries->setCullingActive(false);
        mRootNode->addChild(mGeometries);

        mGUICameras->setName("VR GUI Cameras Root");
        mGUICameras->setNodeMask(MWRender::VisMask::Mask_3DGUI);
        mRootNode->addChild(mGUICameras);

        readConfig();
    }

    VRGUIManager::~VRGUIManager(void)
    {
        sManager = nullptr;
    }

    void VRGUIManager::initScene()
    {
        auto stateset = mGeometries->getOrCreateStateSet();
        stateset->setMode(GL_BLEND, osg::StateAttribute::ON);
        stateset->setAttributeAndModes(new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
#ifdef __ANDROID__
        // UI panels must never be occluded by or intersect world geometry: draw them after the
        // world in a late depth-sorted bin (so panels still order correctly among themselves)
        // with the depth test forced to pass. Depth writes stay off so they don't clip anything.
        stateset->setRenderBinDetails(20, "DepthSortedBin");
        stateset->setAttributeAndModes(new osg::Depth(osg::Depth::ALWAYS, 0.0, 1.0, false),
            osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
#endif
        // assign large value to effectively turn off fog
        // shaders don't respect glDisable(GL_FOG)
        osg::ref_ptr<osg::Fog> fog(new osg::Fog);
        fog->setStart(10000000);
        fog->setEnd(10000000);
        stateset->setAttributeAndModes(fog, osg::StateAttribute::OFF);

        osg::ref_ptr<osg::LightModel> lightmodel = new osg::LightModel;
        lightmodel->setAmbientIntensity(osg::Vec4(1.0, 1.0, 1.0, 1.0));
        stateset->setAttributeAndModes(lightmodel, osg::StateAttribute::ON);

        SceneUtil::ShadowManager::instance().disableShadowsForStateSet(*stateset);
        mGeometries->setStateSet(stateset);
    }

    void VRGUIManager::readConfig()
    {
        clear();

        LayerConfig defaultConfig = createDefaultConfig();
        LayerConfig loadingScreenConfig = createDefaultConfig(true, false);
        // autoSize=false (like the loading screen, which renders with the correct aspect) -- a fixed-extent
        // quad. autoSize=true produced a vertically-squished menu and a vertical click offset on the Quest.
        LayerConfig mainMenuConfig = createDefaultConfig(true, false);
        LayerConfig settingsConfig = createDefaultConfig(true, false);
        LayerConfig defaultWindowsConfig = createDefaultConfig(true);
        LayerConfig videoPlayerConfig = createDefaultConfig(true, false);
        LayerConfig messageBoxConfig = createDefaultConfig(false, true);
        LayerConfig listBoxConfig = createDefaultConfig(true);
        LayerConfig consoleConfig = createDefaultConfig(true);
        mDefaultLayerConfigs = {
            { "DefaultConfig", defaultConfig },
            { "Modal", messageBoxConfig },
            { "Windows", defaultWindowsConfig }, { "ListBox", listBoxConfig }, { "MainMenu", mainMenuConfig },
            { "Settings", settingsConfig },
            { "InputBlocker", videoPlayerConfig }, { "Video", videoPlayerConfig }, { "Console", consoleConfig },
            { "LoadingScreen", loadingScreenConfig },
#ifdef __ANDROID__
            // Registers "Popup" (ComboBox drop-lists) up front so it joins the shared menu canvas
            // deterministically during this loop, before the shared FBO camera is first built.
            { "Popup", videoPlayerConfig },
#endif
        };

        Stereo::Pose defaultUiPose = {};
        // Place the menu straight ahead at eye level and a comfortable distance. The old offset (0.66m up,
        // 0.25m forward) put it close and above eye level, so it was viewed at a steep angle -> the quad
        // foreshortened (rendered ~16:9 instead of 4:3) and the vertical click mapping was thrown off.
        defaultUiPose.position = Stereo::Position::fromMeters(0.f, 1.5f, 0.f);
       
        for (auto& config : mDefaultLayerConfigs)
        {
            getLayer(config.first)->setConfig(config.second);
            getLayer(config.first)->mPose = defaultUiPose;
        }
    }

    void VRGUIManager::clear()
    {
        mGeometries->removeChildren(0, mGeometries->getNumChildren());
        mGUICameras->removeChildren(0, mGUICameras->getNumChildren());
        setFocusLayer(nullptr);
        mLayers.clear();
    }

    void VRGUIManager::clearLua() {
        for (auto& layer : mLayers)
            layer.second->clearLua();
    }

    static std::set<std::string> layerBlacklist = {
        "Overlay",
        "AdditiveOverlay",
        "FadeToBlack",
        "HitOverlay",
    };

    void VRGUIManager::insertWidget(MWGui::Layout* widget)
    {
        auto* layer = widget->mMainWidget->getLayer();
        getLayer(layer->getName())->insertWidget(widget);
    }

    void VRGUIManager::removeWidget(MWGui::Layout* widget)
    {
        auto* layer = widget->mMainWidget->getLayer();
        auto* vrLayer = getLayer(layer->getName());
        vrLayer->removeWidget(widget);
        if (vrLayer == mFocusLayer)
            setFocusLayer(nullptr);
    }

    void VRGUIManager::setVisible(MWGui::Layout* widget, bool visible)
    {
        auto* layer = widget->mMainWidget->getLayer();
        if (!layer)
            return;
        auto name = layer->getName();

        if (layerBlacklist.find(name) != layerBlacklist.end())
        {
            // Never pick an invisible layer
            setPick(name, false);
            return;
        }

        if (visible)
            insertWidget(widget);
        else
            removeWidget(widget);
    }

    void VRGUIManager::onFrameEnd(osg::RenderInfo& info, VR::Frame& frame)
    {
        std::scoped_lock lock(mMutex);
        for (auto& it : mLayers)
        {
            if (!it.second->visible())
                continue;
            it.second->blitLayer(info);
        }
    }

    void VRGUIManager::setForceLayerVisible(std::string_view layerName, bool visible) {
        getLayer(std::string(layerName))->setForceVisible(visible);
    }

    void VRGUIManager::onFrameUpdate(VR::Frame& frame)
    {
        std::scoped_lock lock(mMutex);
        for (auto& it : mLayers)
        {
            if (!it.second->visible())
                continue;
            if (it.second->updateLayer())
            {
                frame.layers.push_back(it.second->mVrLayer);
            }
        }
    }

    void VRGUIManager::onSpaceUpdate()
    {
        std::scoped_lock lock(mMutex);
        for (auto& it : mLayers)
            it.second->updatePose();
    }

    void VRGUIManager::updateFocus(osg::Node* focusNode, osg::Vec3f hitPoint)
    {
        if (focusNode && focusNode->getName() == "VRGUILayer")
        {
            VRGUILayerUserData* userData = static_cast<VRGUILayerUserData*>(focusNode->getUserData());
            setFocusLayer(userData->mLayer);
            computeGuiCursor(hitPoint);
        }
        else
        {
            setFocusLayer(nullptr);
            computeGuiCursor(osg::Vec3(0, 0, 0));
        }
    }

    bool VRGUIManager::hasFocus() const
    {
        return mFocusLayer != nullptr;
    }

    void VRGUIManager::update(osg::NodeVisitor* nv)
    {
        for (auto& layer : mLayers)
            layer.second->update(nv);
    }

    void VRGUIManager::setFocusLayer(osg::ref_ptr<VRGUILayer> layer)
    {
        if (layer == mFocusLayer)
            return;

        // Pick every real MyGUI layer sharing mFocusLayer's FBO (mMemberLayerNames is just {mLayerName}
        // outside a merged group), so clicks reach whichever member is actually on top.
        if (mFocusLayer)
            for (const auto& member : mFocusLayer->mMemberLayerNames)
                setPick(member, false);
        mFocusLayer = layer;
        if (mFocusLayer)
            for (const auto& member : mFocusLayer->mMemberLayerNames)
                setPick(member, true);
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

    VRGUILayer* VRGUIManager::getLayer(const std::string& name)
    {
        // TODO: insert return statement here
        auto it = mLayers.find(name);
        if (it != mLayers.end())
            return it->second;

        setPick(name, false);

#ifdef __ANDROID__
        // Alias this name onto an already-existing shared-group VRGUILayer if one of its group-mates has
        // already been created (see sSharedMenuLayers above).
        if (sSharedMenuLayers.count(name))
        {
            for (const auto& groupMate : sSharedMenuLayers)
            {
                auto groupIt = mLayers.find(groupMate);
                if (groupIt == mLayers.end())
                    continue;
                groupIt->second->mMemberLayerNames.push_back(name);
                mLayers[name] = groupIt->second;
                auto defaultConfig = mDefaultLayerConfigs.find(name);
                if (defaultConfig != mDefaultLayerConfigs.end())
                    groupIt->second->setConfig(defaultConfig->second);
                return groupIt->second;
            }
        }
#endif

        auto layer = osg::ref_ptr<VRGUILayer>(new VRGUILayer(mGeometries, mGUICameras, name, this));
        mLayers[name] = layer;
        auto defaultConfig = mDefaultLayerConfigs.find(name);
        if (defaultConfig != mDefaultLayerConfigs.end())
            layer->setConfig(defaultConfig->second);
        return layer;
    }

    bool VRGUIManager::injectMouseClick()
    {
        // TODO: This relies on a MyGUI internal functions and may break un any future version.
        if (hasFocus() && mFocusWidget != nullptr)
        {
            mFocusWidget->_riseMouseButtonClick();
            return true;
        }
        return false;
    }

    void VRGUIManager::processChangedSettings(const std::set<std::pair<std::string, std::string>>& changed)
    {
        bool needsToReconfigure = false;

        for (Settings::CategorySettingVector::const_iterator it = changed.begin(); it != changed.end(); ++it)
        {
            if (it->first == "VR" && it->second.find(std::string("hud")) != std::string::npos)
                needsToReconfigure = true;
            if (it->first == "VR" && it->second.find(std::string("tooltip")) != std::string::npos)
                needsToReconfigure = true;
        }

        if (needsToReconfigure)
        {
            // Make a back up list of visible widgets
            std::vector<MWGui::Layout*> widgets;
            for (auto& layer : mLayers)
            {
                widgets.insert(widgets.end(), layer.second->mWidgets.begin(), layer.second->mWidgets.end());
                layer.second->removeFromSceneGraph();
            }

            readConfig();

            // Re-populate layers
            for (auto widget : widgets)
                setVisible(widget, true);
        }
    }

    class Pickable
    {
    public:
        virtual ~Pickable() {}

        virtual void setPick(bool pick) = 0;
    };

    template <typename L>
    class PickLayer : public L, public Pickable
    {
    public:
        using L::L;

        void setPick(bool pick) override { L::mIsPick = pick; }
    };

    template <typename L>
    class MyFactory
    {
    public:
        using LayerType = L;
        using PickLayerType = PickLayer<LayerType>;
        using Delegate = MyGUI::delegates::Delegate<MyGUI::IObject*&>;
        static typename Delegate::IDelegate* getFactory() { return MyGUI::newDelegate(createFromFactory); }

        static void registerFactory()
        {
            MyGUI::FactoryManager::getInstance().registerFactory("Layer", LayerType::getClassTypeName(), getFactory());
        }

    private:
        static void createFromFactory(MyGUI::IObject*& _instance) { _instance = new PickLayerType(); }
    };

    void VRGUIManager::registerMyGUIFactories()
    {
        MyFactory<MyGUI::OverlappedLayer>::registerFactory();
        MyFactory<MyGUI::SharedLayer>::registerFactory();
        MyFactory<MyGUIPlatform::AdditiveLayer>::registerFactory();
    }


    void VRGUIManager::setPick(const std::string& name, bool pick)
    {
        for (auto& mwLayer : mLayers)
        {
            if (mwLayer.second->mLayerName == name && mwLayer.second->mWidgets.size() > 0)
            {
                auto* layer = mwLayer.second->mWidgets.front()->mMainWidget->getLayer();
                auto* pickable = dynamic_cast<Pickable*>(layer);
                if (pickable)
                    pickable->setPick(pick);
            }
        }

        auto& layerManager = MyGUI::LayerManager::getInstance();
        auto* layer = layerManager.getByName(name, false);
        if (!layer)
            return;
        auto* pickable = dynamic_cast<Pickable*>(layer);
        if (pickable)
            pickable->setPick(pick);
        else
            Log(Debug::Verbose) << "Tried to modify pick on unknown layer type " << layer->getTypeName() << " ("
                                << layer->getClassTypeName() << ")";
    }

    void VRGUIManager::setLayerPose(const std::string& name, const Stereo::Pose& pose)
    {
        std::scoped_lock lock(mMutex);
        getLayer(name)->mPose = pose;
    }

    void VRGUIManager::setLayerPickable(const std::string &name, bool pickable)
    {
        std::scoped_lock lock(mMutex);
        getLayer(name)->setPickable(pickable);
    }

    void VRGUIManager::setLayerConfig(const std::string& name, const LayerConfig& config)
    {
        std::scoped_lock lock(mMutex);
        getLayer(name)->setConfig(config);
    }

    void VRGUIManager::registerLuaElement(const LuaUi::Element* element)
    {
        const auto& layer = element->mLayer;
        if (layer.empty())
        {
            // Don't know what to do with these
            return;
        }

        auto* vrLayer = getLayer(layer);
        vrLayer->addLuaElement(element);
    }

    void VRGUIManager::deregisterLuaElement(const LuaUi::Element* element)
    {
        const auto& layer = element->mLayer;
        if (layer.empty())
        {
            // Don't know what to do with these
            return;
        }

        auto* vrLayer = getLayer(layer);
        vrLayer->removeLuaElement(element);
    }

    bool VRGUIManager::isLayerRendering(const std::string& layer) {
        return getLayer(layer)->visible();
    }

    void VRGUIManager::computeGuiCursor(osg::Vec3 hitPoint)
    {
        float x = 0;
        float y = 0;
        if (mFocusLayer && mFocusLayer->mConfig)
        {
            auto viewSize = MyGUI::RenderManager::getInstance().getViewSize();
#ifdef __ANDROID__
            // hitPoint is the WORLD hit; worldToCanvasUv projects it onto the quad's world corners.
            float u = 0.f, v = 0.f;
            mFocusLayer->worldToCanvasUv(hitPoint, u, v);
            x = u * static_cast<float>(viewSize.width);
            y = (1.f - v) * static_cast<float>(viewSize.height); // quad top (v=1) -> MyGUI y=0
#else
            osg::Vec2 bottomLeft = mFocusLayer->mConfig->center - osg::Vec2(0.5f, 0.5f);
            x = hitPoint.x() - bottomLeft.x();
            y = hitPoint.z() - bottomLeft.y();
            auto rect = mFocusLayer->mRealRect;
            float width = static_cast<float>(viewSize.width) * rect.width();
            float height = static_cast<float>(viewSize.height) * rect.height();
            float left = static_cast<float>(viewSize.width) * rect.left;
            float bottom = static_cast<float>(viewSize.height) * rect.bottom;
            x = width * x + left;
            y = bottom - height * y;
#endif
        }

        mGuiCursor.x() = (int)x;
        mGuiCursor.y() = (int)y;
        MWBase::Environment::get().getWindowManager()->setCursorActive(true);

        // The virtual keyboard must be interactive regardless of modals
        // This could be generalized with another config entry, but i don't think any other
        // widgets/layers need it so i'm hardcoding it for the VirtualKeyboard for now.
        if (mFocusLayer && mFocusLayer->mLayerName == "VirtualKeyboard")
        {
            auto* widget = MyGUI::LayerManager::getInstance().getWidgetFromPoint((int)x, (int)y);
            setFocusWidget(widget);
        }
        else
            setFocusWidget(nullptr);
    }

#ifdef __ANDROID__
    float VRGUIManager::castContentRay(const Stereo::Pose& pose, MWRender::RayResult& result)
    {
        result = MWRender::RayResult{};

        auto world = MWBase::Environment::get().getWorld();
        auto wm = MWBase::Environment::get().getWindowManager();
        float maxDistance = world->getMaxActivationDistance();
        if (wm->isGuiMode() && wm->isConsoleMode())
            maxDistance *= 50.f;

        osg::Vec3f origin = pose.position.asMWUnits();
        osg::Vec3f direction = pose.orientation * osg::Vec3f(0, 1, 0);
        direction.normalize();

        osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector = new osgUtil::LineSegmentIntersector(
            osgUtil::LineSegmentIntersector::MODEL, origin, origin + direction * maxDistance);
        intersector->setIntersectionLimit(osgUtil::LineSegmentIntersector::NO_LIMIT);
        osgUtil::IntersectionVisitor visitor(intersector);
        // Non-pickable layers carry Mask_3DGUI_NonIntersectable instead (VRGUILayer::setPickable).
        visitor.setTraversalMask(MWRender::Mask_3DGUI);
        mGeometries->accept(visitor);

        // Intersections come sorted nearest-first; take the first quad whose canvas point has content.
        for (const auto& intersection : intersector->getIntersections())
        {
            if (intersection.nodePath.empty())
                continue;
            osg::Node* node = intersection.nodePath.back();
            if (node->getName() != "VRGUILayer")
                continue;
            auto* userData = static_cast<VRGUILayerUserData*>(node->getUserData());
            if (!userData || !userData->mLayer)
                continue;
            VRGUILayer* layer = userData->mLayer;

            osg::Vec3f worldPoint = intersection.getWorldIntersectPoint();
            float u = 0.f, v = 0.f;
            if (!layer->worldToCanvasUv(worldPoint, u, v))
                continue;
            const auto& content = layer->mContentRealRect;
            const float cx = u;
            const float cy = 1.f - v; // MyGUI canvas y grows downward
            if (cx < content.left || cx > content.right || cy < content.top || cy > content.bottom)
                continue;

            result.mHit = true;
            result.mHitNode = node;
            result.mHitPointWorld = worldPoint;
            result.mHitPointLocal = intersection.getLocalIntersectPoint();
            result.mHitNormalWorld = intersection.getWorldIntersectNormal();
            result.mRatio = static_cast<float>(intersection.ratio);
            return (worldPoint - origin).length();
        }
        return 0.f;
    }
#endif
}
