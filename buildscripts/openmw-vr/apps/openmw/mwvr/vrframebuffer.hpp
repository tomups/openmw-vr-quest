#ifndef MWVR_OPENRXTEXTURE_H
#define MWVR_OPENRXTEXTURE_H

#include <memory>
#include <osg/Group>
#include <osg/Camera>
#include <osgViewer/Viewer>

#include "openxrmanager.hpp"

namespace MWVR
{
    /// \brief Manages an opengl framebuffer
    ///
    /// Intended for managing the vr swapchain, but is also use to manage the mirror texture as a convenience.
    class VRFramebuffer
    {
    private:
        struct Texture
        {
            uint32_t mImage = 0;
            bool mOwner = false;

            void delet();
            void setTexture(uint32_t image, bool owner);
        };

    public:
        VRFramebuffer(osg::ref_ptr<osg::State> state, std::size_t width, std::size_t height, uint32_t msaaSamples);
        ~VRFramebuffer();

        void destroy(osg::State* state);

        auto width() const { return mWidth; }
        auto height() const { return mHeight; }
        auto msaaSamples() const { return mSamples; }

        void bindFramebuffer(osg::GraphicsContext* gc, uint32_t target);

        void setColorBuffer(osg::GraphicsContext* gc, uint32_t colorBuffer, bool takeOwnership);
        void setDepthBuffer(osg::GraphicsContext* gc, uint32_t depthBuffer, bool takeOwnership);
        void createColorBuffer(osg::GraphicsContext* gc);
        void createDepthBuffer(osg::GraphicsContext* gc);

        //! ref glBlitFramebuffer
        void blit(osg::GraphicsContext* gc, int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0, int dstX1, int dstY1, uint32_t bits, uint32_t filter = GL_LINEAR);

        uint32_t colorBuffer() const { return mColorBuffer.mImage; };
        uint32_t depthBuffer() const { return mDepthBuffer.mImage; };

    private:
        uint32_t createImage(osg::GraphicsContext* gc, uint32_t formatInternal, uint32_t format);

        // Set aside a weak pointer to the constructor state to use when freeing FBOs, if no state is given to destroy()
        osg::observer_ptr<osg::State> mState;

        // Metadata
        std::size_t mWidth = 0;
        std::size_t mHeight = 0;

        // Render Target
        uint32_t mFBO = 0;
        Texture mDepthBuffer;
        Texture mColorBuffer;
        uint32_t mSamples = 0;
        uint32_t mTextureTarget = 0;
    };
}

#endif
