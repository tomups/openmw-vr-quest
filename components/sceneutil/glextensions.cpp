#include "glextensions.hpp"

#include <osg/GraphicsContext>

namespace SceneUtil
{
    namespace
    {
        std::set<osg::observer_ptr<osg::GLExtensions>> sGLExtensions;

        class GLExtensionsObserver : public osg::Observer
        {
        public:
            static GLExtensionsObserver sInstance;

            ~GLExtensionsObserver() override
            {
                for (auto& ptr : sGLExtensions)
                {
                    osg::ref_ptr<osg::GLExtensions> ref;
                    if (ptr.lock(ref))
                        ref->removeObserver(this);
                }
            }

            void objectDeleted(void* referenced) override
            {
                sGLExtensions.erase(static_cast<osg::GLExtensions*>(referenced));
            }
        };

        // construct after sGLExtensions so this gets destroyed first.
        GLExtensionsObserver GLExtensionsObserver::sInstance{};
    }

    bool glExtensionsReady()
    {
        return !sGLExtensions.empty();
    }

    osg::GLExtensions& getGLExtensions()
    {
        if (sGLExtensions.empty())
            throw std::runtime_error(
                "GetGLExtensionsOperation was not used when the current context was created or there is no current "
                "context");
        return **sGLExtensions.begin();
    }

    GetGLExtensionsOperation::GetGLExtensionsOperation()
        : GraphicsOperation("GetGLExtensionsOperation", false)
    {
    }

    void GetGLExtensionsOperation::operator()(osg::GraphicsContext* graphicsContext)
    {
        osg::GLExtensions* exts = graphicsContext->getState()->get<osg::GLExtensions>();
#ifdef __ANDROID__
        // gl4es advertises desktop-GL extensions (GL_EXT_gpu_shader4, GL_ARB_uniform_buffer_object)
        // that the underlying GLES2 shader compiler cannot honor ("#extension ... : require" fails,
        // producing a black screen). Force them off so OpenMW uses its GLES-compatible shader paths.
        exts->isGpuShader4Supported = false;
        exts->isUniformBufferObjectSupported = false;
        // gl4es reports S3TC/DXT support but its block decode garbles textures (striped/sheared).
        // Forcing it off makes OpenMW software-decompress DDS via OPENMW_DECOMPRESS_TEXTURES instead.
        exts->isTextureCompressionS3TCSupported = false;
#endif
        auto [itr, _] = sGLExtensions.emplace(exts);
        (*itr)->addObserver(&GLExtensionsObserver::sInstance);
    }
}
