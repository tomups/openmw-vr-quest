#include "vrgui.hpp"

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

#include <MyGUI_FactoryManager.h>
#include <MyGUI_ILayer.h>
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
        // Translate a mode to the list of mygui layers that can be visible in that mode
        // (Often differs from the list of windows)
        const static std::map<std::string, std::vector<std::string>> modeToLayers = {
            { "Interface", { "StatsWindow", "InventoryWindow", "MapWindow", "SpellWindow" } },
            { "Container", { "InventoryWindow", "InventoryCompanionWindow" } },
            { "Companion", { "InventoryWindow", "InventoryCompanionWindow" } },
            { "MainMenu", { "Settings", "MainMenu", "MainMenuBackground" } },
            { "Journal", { "JournalBooks" } },
            { "Scroll", { "JournalBooks" } },
            { "Book", { "JournalBooks" } },
            { "Alchemy", { "Windows" } },
            { "Repair", { "Windows" } },
            { "Dialogue", { "DialogueWindow" } },
            { "Barter", { "InventoryWindow", "InventoryCompanionWindow" } },
            { "Rest", { "Windows" } },
            { "SpellBuying", { "Windows" } },
            { "Travel", { "Windows" } },
            { "SpellCreation", { "Windows" } },
            { "Enchanting", { "Windows" } },
            { "Recharge", { "Windows" } },
            { "Training", { "Windows" } },
            { "MerchantRepair", { "Windows" } },
            { "LevelUp", { "Windows" } },
            { "ChargenName", { "Windows" } },
            { "ChargenRace", { "Windows" } },
            { "ChargenBirth", { "Windows" } },
            { "ChargenClass", { "Windows" } },
            { "ChargenClassGenerate", { "Windows" } },
            { "ChargenClassPick", { "Windows" } },
            { "ChargenClassCreate", { "Windows" } },
            { "ChargenClassReview", { "Windows" } },
            { "Loading", { "LoadingScreen" } },
            { "LoadingWallpaper", { "LoadingScreen", "LoadingScreenBackground" } },
            { "Jail", { "Windows" } },
            { "QuickKeysMenu", { "Windows" } },
            { "VrRadialMenu", { "RadialMenu" } },
            { "VrMetaMenu", { "MainMenu", "MainMenuBackground" } }
        };

        // Translate a window to its corresponding layer
        const static std::map<std::string, std::string> windowToLayers = {
            { "Alchemy", "Windows" },
            { "Book", "JournalBooks" },
            { "Companion", "InventoryCompanionWindow" },
            { "Container", "InventoryCompanionWindow" },
            { "Dialogue", "DialogueWindow" },
            { "EnchantingDialog", "Windows" },
            { "Inventory", "InventoryWindow" },
            { "JailScreen", "Windows" },
            { "Journal", "JournalBooks" },
            { "LevelUpDialog", "Windows" },
            { "Magic", "SpellWindow" },
            { "Map", "MapWindow" },
            { "MerchantRepair", "Windows" },
            { "QuickKeys", "Windows" },
            { "Recharge", "Windows" },
            { "Repair", "Windows" },
            { "Scroll", "JournalBooks" },
            { "SpellBuying", "Windows" },
            { "SpellCreationDialog", "Windows" },
            { "Stats", "StatsWindow" },
            { "Trade", "InventoryCompanionWindow" },
            { "Training", "Windows" },
            { "Travel", "Windows" },
            { "WaitDialog", "Windows" },
        };

        const static std::set<std ::string> resizableWindowLayers = { "InventoryCompanionWindow", "DialogueWindow",
            "InventoryWindow", "StatsWindow", "MapWindow", "SpellWindow" };

        // Since the MODE stack cannot always inform us what should be in front, and MyGUI doesn't expose this information, i
        // have to manually re-compute what the actual priorities are
        std::map<std::string, int> autoLayerPriorities(const std::vector<std::string>& layers)
        {
            std::map<std::string, int> res;
            int priority = 0;
            for (auto layer : layers)
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
    static osg::Vec2 radiusAngleWidth(float radius, float angleRadian)
    {
        const float width = std::fabs(2.f * radius * tanf(angleRadian / 2.f));
        return osg::Vec2(width, width);
    }

    class GUIRTT : public SceneUtil::RTTNode
    {
    public:
        GUIRTT(int width, int height, osg::Vec4 clearColor, osg::ref_ptr<osg::Node> scene)
            : RTTNode(width, height, 1, true, 0, StereoAwareness::Unaware, false)
            , mScene(scene)
            , mClearColor(clearColor)
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
    };

    osg::ref_ptr<osg::Geometry> VRGUILayer::createLayerGeometry(osg::ref_ptr<osg::StateSet> stateset)
    {
        float left = activeConfig()->center.x() - 0.5;
        float right = left + 1.f;
        float top = 0.5f + activeConfig()->center.y();
        float bottom = top - 1.f;

        osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();

        // Define the menu quad
        osg::Vec3 top_left(left, 1, top);
        osg::Vec3 bottom_left(left, 1, bottom);
        osg::Vec3 bottom_right(right, 1, bottom);
        osg::Vec3 top_right(right, 1, top);

        osg::ref_ptr<osg::Vec3Array> vertices{ new osg::Vec3Array(4) };
        (*vertices)[0] = bottom_left;
        (*vertices)[1] = top_left;
        (*vertices)[2] = bottom_right;
        (*vertices)[3] = top_right;
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
        , mGeometryRoot(geometryRoot)
        , mCameraRoot(cameraRoot)
        , mVrLayer()
    {

        // Edit offset to account for priority

        // Define state set that allows rendering with transparency
    }

    void VRGUILayer::setConfig(const std::string& mode, const LayerConfig& config)
    {
        mPerModeConfigs[mode] = config;
        if (mode == mActiveMode)
            mDirty = true;
    }

    void VRGUILayer::setMode(const std::string& mode) {
        if (mode != mActiveMode)
            mDirty = true;
        mActiveMode = mode;
    }

    void VRGUILayer::clear()
    {
        removeFromSceneGraph();
    }

    const LayerConfig* VRGUILayer::activeConfig()
    {
        auto it = mPerModeConfigs.find(mActiveMode);
        if (it == mPerModeConfigs.end())
            return nullptr;
        return &it->second;
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
        if (!mGUIRTT)
            return nullptr;
        return static_cast<osg::Texture2D*>(static_cast<GUIRTT*>(mGUIRTT.get())->getColorTexture(nullptr));
    }

    bool VRGUILayer::updateLayer()
    {
        // TODO: Do i actually need to do anything here?
        return mVisible && mVrLayer;
    }

    void VRGUILayer::blitLayer(osg::RenderInfo& info)
    {
        auto config = activeConfig();
        if (mVrLayer && config)
        {
            auto* state = info.getState();
            auto texture = colorTexture();
            mVrLayer->colorSwapchain->beginFrame(state->getGraphicsContext());
            auto glImage = mVrLayer->colorSwapchain->image()->glImage();

            int width = config->pixelResolution.x();
            int height = config->pixelResolution.y();

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
        auto config = activeConfig();
        if (!config)
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
            pose.position = pose.position + diff * priority;
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

        mRealRect.left = static_cast<float>(mRect.left) / static_cast<float>(viewSize.width);
        mRealRect.top = static_cast<float>(mRect.top) / static_cast<float>(viewSize.height);
        mRealRect.right = static_cast<float>(mRect.right) / static_cast<float>(viewSize.width);
        mRealRect.bottom = static_cast<float>(mRect.bottom) / static_cast<float>(viewSize.height);
    }

    void VRGUILayer::update(osg::NodeVisitor* nv)
    {
        auto config = activeConfig();
        if (!config)
        {
            clear();
            return;
        }

        if (mDirty)
        {
            clear();

            auto extent_units = config->extent * Constants::UnitsPerMeter;

            // Create the camera that will render the menu texture
            std::string filter = mLayerName;
            // Some layers have their own background layer
            filter = filter + ";" + mLayerName + "Background";
            osgMyGUI::RenderManager& renderManager
                = static_cast<osgMyGUI::RenderManager&>(MyGUI::RenderManager::getInstance());
            mMyGUICamera = renderManager.createGUICamera(osg::Camera::NESTED_RENDER, filter);

            auto* rttNode = new GUIRTT(config->pixelResolution.x(), config->pixelResolution.y(),
                osg::Vec4(0, 0, 0, config->opacity), mMyGUICamera);
            mGUIRTT = rttNode;

            if (!config->space.empty())
                mSpace = OpenXRInput::instance().getSpace(config->space);

            if (mVrLayer)
            {
                mVrLayer->colorSwapchain = VR::Session::instance().createSwapchain(config->pixelResolution.x(),
                    config->pixelResolution.y(), 1, 1, VR::Swapchain::Attachment::Color, mLayerName + "Swapchain");
                mVrLayer->extent = config->extent;
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

                auto texture = rttNode->getColorTexture(nullptr);
                texture->setName("diffuseMap");
                mStateset->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);

                osg::ref_ptr<osg::Material> mat = new osg::Material;
                mat->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
                mStateset->setAttribute(mat);

                mStateset->setMode(GL_ALPHA_TEST, osg::StateAttribute::OFF);

                mGeometries[0] = createLayerGeometry(mStateset);
                mGeometries[0]->setUserData(new VRGUILayerUserData(this));
                mGeometries[1] = createLayerGeometry(mStateset);
                mGeometries[1]->setUserData(mGeometries[0]->getUserData());

                // Position in the game world
                mTransform->setScale(osg::Vec3(extent_units.x(), 1.f, extent_units.y()));
                mTransform->setCullCallback(new CullVRGUILayerCallback(this));
                mTransform->setCullingActive(false);
                if (!config->intersectable)
                    mTransform->setNodeMask(MWRender::Mask_Effect);
                // configureGeometry();
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

        if (!mWidgets.empty())
        {
            // if (mConfig->sideBySide && !mWidgets.empty())
            //{
            // Auto stretch resizeable windows
            // Stretch according to config
            // This genre of layer should only ever have 1 widget as it will cover the full layer
            auto* widget = mWidgets.front();
            auto* myGUIWindow = dynamic_cast<MyGUI::Window*>(widget->mMainWidget);
            auto* windowBase = dynamic_cast<MWGui::WindowBase*>(widget);
            if (windowBase && myGUIWindow)
            {
                // auto w = mConfig.myGUIViewSize.x();
                // auto h = mConfig.myGUIViewSize.y();
                //windowBase->setCoordf(0.f, 0.f, 0.7f, 0.7f);
                //windowBase->onWindowResize(myGUIWindow);
            }
            // }
        }
        updateRect();

        //// Pixels per unit
        float res = static_cast<float>(config->spatialResolution) / Constants::UnitsPerMeter;
        float w = static_cast<float>(mRect.right - mRect.left);
        float h = static_cast<float>(mRect.bottom - mRect.top);

        if (config->autoSize)
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
            (*texCoords)[0].set(mRealRect.left, 1.f - mRealRect.bottom);
            (*texCoords)[1].set(mRealRect.left, 1.f - mRealRect.top);
            (*texCoords)[2].set(mRealRect.right, 1.f - mRealRect.bottom);
            (*texCoords)[3].set(mRealRect.right, 1.f - mRealRect.top);
            texCoords->dirty();

            geometry->accept(*cv);
        }
    }

    void VRGUILayer::setVisible(bool visible)
    {
        if (visible == mVisible)
            return;
        mVisible = visible;

        if (mVisible)
            addToSceneGraph();
        else
            removeFromSceneGraph();
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

    static const LayerConfig createDefaultConfig(bool intersectable = true, bool background = true, bool autoSize = true)
    {
        return LayerConfig{ background ? 0.75f : 0.f, // background
            // Stereo::Position::fromMeters(0.f, 0.66f, -.25f), // offset
            osg::Vec2(0.f, 0.f), // center (model space)
            osg::Vec2(1.f, 1.f), // extent (meters)
            1024, // Spatial resolution (pixels per meter)
            osg::Vec2i(1024, 1024), // Texture resolution
            osg::Vec2(1, 1), autoSize, "", intersectable };
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

    VRGUIManager::VRGUIManager(Resource::ResourceSystem* resourceSystem, osg::Group* rootNode)
        : mResourceSystem(resourceSystem)
        , mRootNode(rootNode)
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
        //                mHUDKeyboardPose = tp;
        //                mHUDKeyboardPose.pose.position
        //                    += mHUDKeyboardPose.pose.orientation * Stereo::Position::fromMWUnits(osg::Vec3(0, 35,
        //                    -40));
        //                // Tilt the keyboard slightly for easier typing.
        //                mHUDKeyboardPose.pose.orientation
        //                    = osg::Quat(osg::PI_4 / 2.f, osg::Vec3(-1, 0, 0)) * mHUDKeyboardPose.pose.orientation;
        //            }
        //            else if (mShouldUpdateStationaryPoseHeight)
        //            {
        //                mStationaryPose.pose.position.mZ = tp.pose.position.mZ;
        //                mHUDKeyboardPose.pose.position.mZ = tp.pose.position.mZ
        //                    + (mHUDKeyboardPose.pose.orientation * Stereo::Position::fromMWUnits(osg::Vec3(0, 35,
        //                    -40))).mZ;
        // Stereo::Pose defaultVKeyboardPose = {};
        // defaultVKeyboardPose.position = Stereo::Position::fromMWUnits(osg::Vec3(0, 35, -40));
        // defaultVKeyboardPose.orientation = osg::Quat(osg::PI_4 / 2.f, osg::Vec3(-1, 0, 0));

        // defaultVKeyboardPose.position

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
        LayerConfig loadingScreenConfig = createDefaultConfig(true, true, false);
        LayerConfig mainMenuConfig = createDefaultConfig(true, true, true);
        LayerConfig settingsConfig = createDefaultConfig(true, true, true);
        // LayerConfig journalBooksConfig = createDefaultConfig(2, true, false, false);
        LayerConfig defaultWindowsConfig = createDefaultConfig(true, true);
        LayerConfig videoPlayerConfig = createDefaultConfig(true, true, false);
        LayerConfig messageBoxConfig = createDefaultConfig(true, false, true);
        // LayerConfig notificationConfig = createDefaultConfig(7, false, false, false);
        LayerConfig listBoxConfig = createDefaultConfig(true, true);
        LayerConfig consoleConfig = createDefaultConfig(true, true);
        // LayerConfig radialMenuConfig = createDefaultConfig(11, true, false);
        //  TODO: Track around wrist instead of being a regular menu quad?
        //  radialMenuConfig.offset = osg::Vec3(0.f, 0.66f, 0.f);
        //  radialMenuConfig.trackingPath = Paths::sWristTopRightStr;

        // LayerConfig statsWindowConfig = createSideBySideConfig(0);
        // LayerConfig inventoryWindowConfig = createSideBySideConfig(1);
        // LayerConfig spellWindowConfig = createSideBySideConfig(2);
        // LayerConfig mapWindowConfig = createSideBySideConfig(3);
        // LayerConfig inventoryCompanionWindowConfig = createSideBySideConfig(4);
        // LayerConfig dialogueWindowConfig = createSideBySideConfig(5);

        // auto positionSettingToPath = [](const std::string& setting) -> std::string {
        //     if (Misc::StringUtils::ciEqual(setting, "left wrist inner"))
        //     {
        //         return Paths::sWristInnerLeftStr;
        //     }
        //     if (Misc::StringUtils::ciEqual(setting, "right wrist inner"))
        //     {
        //         return Paths::sWristInnerRightStr;
        //     }
        //     if (Misc::StringUtils::ciEqual(setting, "left wrist top"))
        //     {
        //         return Paths::sWristTopLeftStr;
        //     }
        //     if (Misc::StringUtils::ciEqual(setting, "right wrist top"))
        //     {
        //         return Paths::sWristTopRightStr;
        //     }
        //     if (Misc::StringUtils::ciEqual(setting, "top left"))
        //     {
        //         return Paths::sHUDTopLeftStr;
        //     }
        //     if (Misc::StringUtils::ciEqual(setting, "top right"))
        //     {
        //         return Paths::sHUDTopRightStr;
        //     }
        //     if (Misc::StringUtils::ciEqual(setting, "bottom left"))
        //     {
        //         return Paths::sHUDBottomLeftStr;
        //     }
        //     if (Misc::StringUtils::ciEqual(setting, "bottom right"))
        //     {
        //         return Paths::sHUDBottomRightStr;
        //     }

        //    return Paths::sHUDTopLeftStr;
        //};

        // auto hudOffset = Stereo::Position::fromMeters(
        //     osg::Vec3(Settings::Manager::getFloat("hud offset x", "VR"),
        //         Settings::Manager::getFloat("hud offset y", "VR"), Settings::Manager::getFloat("hud offset z", "VR"))
        //     - osg::Vec3(0.5, 0.5, 0.5));

        // auto tooltipOffset
        //     = Stereo::Position::fromMeters(osg::Vec3(Settings::Manager::getFloat("tooltip offset x", "VR"),
        //                                        Settings::Manager::getFloat("tooltip offset y", "VR"),
        //                                        Settings::Manager::getFloat("tooltip offset z", "VR"))
        //         - osg::Vec3(0.5, 0.5, 0.5));

        // std::string hudPath = positionSettingToPath(Settings::Manager::getString("hud position", "VR"));
        // std::string tooltipPath = positionSettingToPath(Settings::Manager::getString("tooltip position", "VR"));

        // LayerConfig virtualKeyboardConfig = LayerConfig{ 10, .75f,
        //     //{}, // offset (meters)
        //     osg::Vec2(0.f, 1.f), // center (model space)
        //     osg::Vec2(.25f, .25f), // extent (meters)
        //     2048, // Spatial resolution (pixels per meter)
        //     osg::Vec2i(1024, 1024), // Texture resolution
        //     osg::Vec2(1, 1), true, DefaultVKeyboardSpace,
        //     // Paths::sHUDKeyboardStr,
        //     true };

        // LayerConfig HUDConfig = LayerConfig{ 0,
        //     false, // side-by-side
        //     0.f, // background
        //     hudOffset,
        //     // osg::Vec3(0,0,0), // offset (meters)
        //     osg::Vec2(0.f, 0.5f), // center (model space)
        //     osg::Vec2(.033f, .033f), // extent (meters)
        //     1024, // resolution (pixels per meter)
        //     osg::Vec2i(1024, 1024), defaultConfig.myGUIViewSize, true, hudPath, "", true };

        // LayerConfig tooltipConfig = LayerConfig{ 0,
        //     false, // side-by-side
        //     0.f, // background
        //     tooltipOffset, osg::Vec2(0.f, 0.5f), // center (model space)
        //     osg::Vec2(.33f, .33f), // extent (meters)
        //     1024, // resolution (pixels per meter)
        //     osg::Vec2i(1024, 1024), defaultConfig.myGUIViewSize, true, tooltipPath, "", false };

        mDefaultLayerConfigs = {
            { "DefaultConfig", defaultConfig },
            //{ "HUD", HUDConfig },
            //{ "Tooltip", tooltipConfig },
            //{ "JournalBooks", journalBooksConfig },
            //{ "InventoryCompanionWindow", inventoryCompanionWindowConfig },
            //{ "InventoryWindow", inventoryWindowConfig },
            //{ "SpellWindow", spellWindowConfig },
            //{ "MapWindow", mapWindowConfig },
            //{ "StatsWindow", statsWindowConfig },
            //{ "DialogueWindow", dialogueWindowConfig },
            { "Modal", messageBoxConfig },
            //            { "RadialMenu", radialMenuConfig },
            { "Windows", defaultWindowsConfig },
            { "ListBox", listBoxConfig },
            { "MainMenu", mainMenuConfig },
            { "Settings", settingsConfig },
            //          { "Notification", notificationConfig },
            { "InputBlocker", videoPlayerConfig },
            { "Video", videoPlayerConfig },
            { "Console", consoleConfig },
            { "LoadingScreen", loadingScreenConfig },
            //{ "VirtualKeyboard", virtualKeyboardConfig },
        };

        
        Stereo::Pose defaultUiPose = {};
        defaultUiPose.position = Stereo::Position::fromMeters(0.f, 0.66f, -.25f);
        // OpenXRInput::instance().createDerivedSpace(DefaultUiSpace, VR::ReferenceSpace::View, defaultUiPose);
        for (auto& config : mDefaultLayerConfigs)
        {
            getLayer(config.first).setConfig("", config.second);
            getLayer(config.first).mPose = defaultUiPose;
        }
    }

    void VRGUIManager::clear()
    {
        mGeometries->removeChildren(0, mGeometries->getNumChildren());
        mGUICameras->removeChildren(0, mGUICameras->getNumChildren());
        setFocusLayer(nullptr);
        mLayers.clear();
        mSideBySideLayers.clear();
    }

    static std::set<std::string> layerBlacklist = {
        "Overlay",
        "AdditiveOverlay",
        "FadeToBlack",
        "HitOverlay",
    };

    // void VRGUIManager::updateSideBySideLayers()
    //{
    //     // Nothing to update
    //     if (mSideBySideLayers.size() == 0)
    //         return;

    //    std::sort(mSideBySideLayers.begin(), mSideBySideLayers.end(),
    //        [](const auto& lhs, const auto& rhs) { return *lhs < *rhs; });

    //    int n = mSideBySideLayers.size();

    //    float span
    //        = sSideBySideAzimuthInterval * static_cast<float>(n - 1); // zero index, places lone layers straight ahead
    //    float low = -span / 2;

    //    for (unsigned i = 0; i < mSideBySideLayers.size(); i++)
    //    {
    //        mSideBySideLayers[i]->setAngle(low + static_cast<float>(i) * sSideBySideAzimuthInterval);
    //    }
    //}

    void VRGUIManager::showLayer(const std::string& name)
    {
        auto& layer = getLayer(name);
        layer.setVisible(true);

        // if (it->second->mConfig->sideBySide)
        //{
        //     bool found = false;
        //     for (auto layer : mSideBySideLayers)
        //         if (layer == it->second)
        //             found = true;
        //     if (!found)
        //     {
        //         mSideBySideLayers.push_back(it->second);
        //         updateSideBySideLayers();
        //     }
        // }
    }

    void VRGUIManager::insertWidget(MWGui::Layout* widget)
    {
        auto* layer = widget->mMainWidget->getLayer();
        auto name = layer->getName();
        showLayer(name);

        auto it = mLayers.find(name);
        it->second->insertWidget(widget);

        if (it->second != mFocusLayer)
            setPick(widget, false);

        it->second->setVisible(true);
    }

    void VRGUIManager::hideLayer(const std::string& name)
    {
        auto it = mLayers.find(name);
        if (it == mLayers.end())
            return;

        // for (auto it2 = mSideBySideLayers.begin(); it2 != mSideBySideLayers.end();)
        //{
        //     if (*it2 == it->second)
        //     {
        //         it2 = mSideBySideLayers.erase(it2);
        //         updateSideBySideLayers();
        //     }
        //     else
        //     {
        //         ++it2;
        //     }
        // }

        if (it->second == mFocusLayer)
            setFocusLayer(nullptr);

        it->second->setVisible(false);
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
            hideLayer(name);
        }
    }

    void VRGUIManager::setVisible(MWGui::Layout* widget, bool visible)
    {
        auto* layer = widget->mMainWidget->getLayer();
        auto name = layer->getName();

        Log(Debug::Verbose) << "SetVisible: " << name << ": " << visible;

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
        auto wm = MWBase::Environment::get().getWindowManager();
        for (MWGui::GuiMode m : wm->getGuiModeStack())
        {
            auto mode = wm->guiModeToName().at(m);
            auto it = mModeConfigs.find(std::string(mode));
            if (it == mModeConfigs.end())
                continue;

            auto windows = wm->getAllowedWindowIds(m);
            for (auto window : windows)
            {
                auto layer = windowToLayers.at(std::string(window));
                getLayer(layer).setMode(std::string(mode));
            }
        }

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

        if (mFocusLayer)
        {
            if (!mFocusLayer->mWidgets.empty())
                setPick(mFocusLayer->mWidgets.front(), false);
        }
        mFocusLayer = layer;
        if (mFocusLayer)
        {
            Log(Debug::Debug) << "setFocusLayer: " << mFocusLayer->mLayerName;
            if (!mFocusLayer->mWidgets.empty())
            {
                setPick(mFocusLayer->mWidgets.front(), true);
            }
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

    VRGUILayer& VRGUIManager::getLayer(const std::string& name)
    {
        // TODO: insert return statement here
        auto it = mLayers.find(name);
        if (it != mLayers.end())
            return *it->second;

        auto layer = osg::ref_ptr<VRGUILayer>(new VRGUILayer(mGeometries, mGUICameras, name, this));
        mLayers[name] = layer;
        auto defaultConfig = mDefaultLayerConfigs.find(name);
        if (defaultConfig != mDefaultLayerConfigs.end())
            layer->setConfig("", defaultConfig->second);
        return *layer;
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
            for (auto layer : mLayers)
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
        MyFactory<osgMyGUI::AdditiveLayer>::registerFactory();
    }

    void VRGUIManager::setPick(MWGui::Layout* widget, bool pick)
    {
        auto* layer = widget->mMainWidget->getLayer();
        auto* pickable = dynamic_cast<Pickable*>(layer);
        if (pickable)
            pickable->setPick(pick);
    }

    void VRGUIManager::setLayerPose(const std::string& name, const Stereo::Pose& pose)
    {
        std::scoped_lock lock(mMutex);
        getLayer(name).mPose = pose;
    }

    void VRGUIManager::setLayerConfig(const std::string& name, const LayerConfig& config)
    {
        std::scoped_lock lock(mMutex);
        getLayer(name).setConfig("", config);
    }

    void VRGUIManager::setModeConfig(const std::string& mode, const LayerConfig& config) 
    {
        auto it = modeToLayers.find(mode);
        if (it == modeToLayers.end())
        {
            Log(Debug::Error) << "Mode not recognized: " << mode;
            return;
        }
        mModeConfigs[mode] = config;
        for (auto& layer : it->second)
        {
            getLayer(layer).setConfig(mode, config);
        }
    }

    void VRGUIManager::setModePose(const std::string& mode, const Stereo::Pose& pose, const std::string window) 
    {
        if (window.empty())
        {
            for (auto& layer : modeToLayers.at(mode))
                getLayer(layer).mPose = pose;
        }
        else
            getLayer(windowToLayers.at(window)).mPose = pose;
    }

    void VRGUIManager::computeGuiCursor(osg::Vec3 hitPoint)
    {
        float x = 0;
        float y = 0;
        if (mFocusLayer && mFocusLayer->activeConfig())
        {
            osg::Vec2 bottomLeft = mFocusLayer->activeConfig()->center - osg::Vec2(0.5f, 0.5f);
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
        if (mFocusLayer && mFocusLayer->mLayerName == "VirtualKeyboard")
        {
            auto* widget = MyGUI::LayerManager::getInstance().getWidgetFromPoint((int)x, (int)y);
            setFocusWidget(widget);
        }
        else
            setFocusWidget(nullptr);
    }
}
