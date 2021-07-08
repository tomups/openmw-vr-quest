#ifndef OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUIRENDERMANAGER_H
#define OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUIRENDERMANAGER_H

#include <MyGUI_RenderManager.h>

#include <osg/ref_ptr>
#include <set>

#include "myguicompat.h"

namespace Resource
{
    class ImageManager;
}

namespace osgViewer
{
    class Viewer;
}

namespace osg
{
    class Group;
    class Camera;
    class RenderInfo;
    class StateSet;
}

namespace osgMyGUI
{

class Drawable;
class GUICamera;

class StateInjectableRenderTarget : public MyGUI::IRenderTarget
{
public:
    StateInjectableRenderTarget() = default;
    ~StateInjectableRenderTarget() = default;

    /** specify a StateSet to inject for rendering. The StateSet will be used by future doRender calls until you reset it to nullptr again. */
    void setInjectState(osg::StateSet* stateSet);

protected:
    osg::StateSet* mInjectState{ nullptr };
};

class RenderManager : public MyGUI::RenderManager
{
    osg::ref_ptr<osgViewer::Viewer> mViewer;
    osg::ref_ptr<osg::Group> mSceneRoot;
    osg::ref_ptr<GUICamera> mGuiCamera;
    std::set<GUICamera*> mGuiCameras;
    Resource::ImageManager* mImageManager;
    MyGUI::IntSize mViewSize;

    MyGUI::VertexColourType mVertexFormat;
    MyGUI::RenderTargetInfo mInfo;


    typedef std::map<std::string, MyGUI::ITexture*> MapTexture;
    MapTexture mTextures;

    bool mIsInitialise;

    float mInvScalingFactor;


    bool mVRMode;

    void destroyAllResources();

public:
    RenderManager(osgViewer::Viewer *viewer, osg::Group *sceneroot, Resource::ImageManager* imageManager, float scalingFactor);
    virtual ~RenderManager();

    void initialise();
    void shutdown();

    void setScalingFactor(float factor);

    static RenderManager& getInstance() { return *getInstancePtr(); }
    static RenderManager* getInstancePtr()
    { return static_cast<RenderManager*>(MyGUI::RenderManager::getInstancePtr()); }

    /** @see RenderManager::getViewSize */
    const MyGUI::IntSize& getViewSize() const override { return mViewSize; }

    /** @see RenderManager::getVertexFormat */
    MyGUI::VertexColourType getVertexFormat() OPENMW_MYGUI_CONST_GETTER_3_4_1 override
    { return mVertexFormat; }

    /** @see RenderManager::isFormatSupported */
    bool isFormatSupported(MyGUI::PixelFormat format, MyGUI::TextureUsage usage) override;

    /** @see RenderManager::createVertexBuffer */
    MyGUI::IVertexBuffer* createVertexBuffer() override;
    /** @see RenderManager::destroyVertexBuffer */
    void destroyVertexBuffer(MyGUI::IVertexBuffer *buffer) override;

    /** @see RenderManager::createTexture */
    MyGUI::ITexture* createTexture(const std::string &name) override;
    /** @see RenderManager::destroyTexture */
    void destroyTexture(MyGUI::ITexture* _texture) override;
    /** @see RenderManager::getTexture */
    MyGUI::ITexture* getTexture(const std::string &name) override;

    // Called by the update traversal
    void update();

    bool checkTexture(MyGUI::ITexture* _texture);

    // setViewSize() is a part of MyGUI::RenderManager interface since 3.4.0 release
#if MYGUI_VERSION < MYGUI_DEFINE_VERSION(3, 4, 0)
    void setViewSize(int width, int height);
#else
    void setViewSize(int width, int height) override;
#endif

    // registerShader() is a part of MyGUI::RenderManager interface since 3.4.1 release
#if MYGUI_VERSION > MYGUI_DEFINE_VERSION(3, 4, 0)
    void registerShader(const std::string& _shaderName, const std::string& _vertexProgramFile, const std::string& _fragmentProgramFile) override;
#endif

/*internal:*/

    void collectDrawCalls();
    osg::ref_ptr<osg::Camera> createGUICamera(int order, std::string layerFilter);
    void deleteGUICamera(GUICamera* camera);
};

}

#endif
