#ifndef OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUIRENDERMANAGER_H
#define OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUIRENDERMANAGER_H

#include <MyGUI_RenderManager.h>

#include <osg/ref_ptr>

#include <set>

namespace Resource
{
    class ImageManager;
}

namespace Shader
{
    class ShaderManager;
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
    class Program;
}

namespace osgMyGUI
{

    class Drawable;
    class OSGTexture;
//## VR_PATCH BEGIN
// Make a separate class inherit IRenderTarget and handle the inject state
    class GUICamera;

    class StateInjectableRenderTarget : public MyGUI::IRenderTarget
    {
    public:
        StateInjectableRenderTarget() = default;
        ~StateInjectableRenderTarget() = default;

        /** specify a StateSet to inject for rendering. The StateSet will be used by future doRender calls until you
         * reset it to nullptr again. */
        void setInjectState(osg::StateSet* stateSet);

    protected:
        osg::StateSet* mInjectState{ nullptr };
    };

    class RenderManager : public MyGUI::RenderManager
    {
        osg::ref_ptr<osgViewer::Viewer> mViewer;
        osg::ref_ptr<osg::StateSet> mGuiStateSet;
        osg::ref_ptr<osg::Group> mSceneRoot;
        Resource::ImageManager* mImageManager;
        MyGUI::IntSize mViewSize;

        MyGUI::VertexColourType mVertexFormat;

        std::map<std::string, OSGTexture> mTextures;

        bool mIsInitialise;

        float mInvScalingFactor;

    public:
//## VR_PATCH END
        RenderManager(osgViewer::Viewer* viewer, osg::Group* sceneroot, Resource::ImageManager* imageManager,
            float scalingFactor);
        virtual ~RenderManager();

        void initialise();
        void shutdown();

        void enableShaders(Shader::ShaderManager& shaderManager);

        static RenderManager& getInstance() { return *getInstancePtr(); }
        static RenderManager* getInstancePtr()
        {
            return static_cast<RenderManager*>(MyGUI::RenderManager::getInstancePtr());
        }

        bool checkTexture(MyGUI::ITexture* _texture) override;

        /** @see RenderManager::getViewSize */
        const MyGUI::IntSize& getViewSize() const override { return mViewSize; }

        /** @see RenderManager::getVertexFormat */
        MyGUI::VertexColourType getVertexFormat() const override { return mVertexFormat; }

        /** @see RenderManager::isFormatSupported */
        bool isFormatSupported(MyGUI::PixelFormat format, MyGUI::TextureUsage usage) override;

        /** @see RenderManager::createVertexBuffer */
        MyGUI::IVertexBuffer* createVertexBuffer() override;
        /** @see RenderManager::destroyVertexBuffer */
        void destroyVertexBuffer(MyGUI::IVertexBuffer* buffer) override;

        /** @see RenderManager::createTexture */
        MyGUI::ITexture* createTexture(const std::string& name) override;
        /** @see RenderManager::destroyTexture */
        void destroyTexture(MyGUI::ITexture* _texture) override;
        /** @see RenderManager::getTexture */
        MyGUI::ITexture* getTexture(const std::string& name) override;

        // Called by the update traversal
        void update();

//## VR_PATCH BEGIN

        void setViewSize(int width, int height) override;

        void registerShader(const std::string& _shaderName, const std::string& _vertexProgramFile,
            const std::string& _fragmentProgramFile) override;

        osg::ref_ptr<osg::Camera> createGUICamera(int order, std::string layerFilter);
//## VR_PATCH END
    };

}

#endif
