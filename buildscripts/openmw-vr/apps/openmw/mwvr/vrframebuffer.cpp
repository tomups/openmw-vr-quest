#include "vrframebuffer.hpp"

#include <osg/Texture2D>

#include <components/debug/debuglog.hpp>

#ifndef GL_TEXTURE_MAX_LEVEL
#define GL_TEXTURE_MAX_LEVEL 0x813D
#endif

namespace MWVR
{

    VRFramebuffer::VRFramebuffer(osg::ref_ptr<osg::State> state, std::size_t width, std::size_t height, uint32_t msaaSamples)
        : mState(state)
        , mWidth(width)
        , mHeight(height)
        , mDepthBuffer()
        , mColorBuffer()
        , mSamples(msaaSamples)
    {
        auto* gl = osg::GLExtensions::Get(state->getContextID(), false);

        gl->glGenFramebuffers(1, &mFBO);

        if (mSamples <= 1)
            mTextureTarget = GL_TEXTURE_2D;
        else
            mTextureTarget = GL_TEXTURE_2D_MULTISAMPLE;
    }

    void VRFramebuffer::setColorBuffer(osg::GraphicsContext* gc, uint32_t colorBuffer, bool takeOwnership)
    {
        auto* gl = osg::GLExtensions::Get(gc->getState()->getContextID(), false);
        mColorBuffer.setTexture(colorBuffer, takeOwnership);
        bindFramebuffer(gc, GL_FRAMEBUFFER_EXT);
        gl->glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, mTextureTarget, mColorBuffer.mImage, 0);
    }

    void VRFramebuffer::setDepthBuffer(osg::GraphicsContext* gc, uint32_t depthBuffer, bool takeOwnership)
    {
        auto* gl = osg::GLExtensions::Get(gc->getState()->getContextID(), false);
        mDepthBuffer.setTexture(depthBuffer, takeOwnership);
        bindFramebuffer(gc, GL_FRAMEBUFFER_EXT);
        gl->glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, mTextureTarget, mDepthBuffer.mImage, 0);
    }

    void VRFramebuffer::createColorBuffer(osg::GraphicsContext* gc)
    {
        auto colorBuffer = createImage(gc, GL_RGBA8, GL_RGBA);
        setColorBuffer(gc, colorBuffer, true);
    }

    void VRFramebuffer::createDepthBuffer(osg::GraphicsContext* gc)
    {
        auto depthBuffer = createImage(gc, GL_DEPTH24_STENCIL8_EXT, GL_DEPTH_COMPONENT);
        setDepthBuffer(gc, depthBuffer, true);
    }

    VRFramebuffer::~VRFramebuffer()
    {
        destroy(nullptr);
    }

    void VRFramebuffer::destroy(osg::State* state)
    {
        if (!state)
        {
            // Try re-using the state received during construction
            state = mState.get();
        }

        if (state)
        {
            auto* gl = osg::GLExtensions::Get(state->getContextID(), false);
            if (mFBO)
                gl->glDeleteFramebuffers(1, &mFBO);
        }
        else if (mFBO)
            // Without access to glDeleteFramebuffers, i'll have to leak FBOs.
            Log(Debug::Warning) << "destroy() called without a State. Leaking FBO";
        mFBO = 0;

        mColorBuffer.delet();
        mDepthBuffer.delet();
    }

    void VRFramebuffer::bindFramebuffer(osg::GraphicsContext* gc, uint32_t target)
    {
        auto state = gc->getState();
        auto* gl = osg::GLExtensions::Get(state->getContextID(), false);
        //if (gl->glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
        //    throw std::runtime_error("Tried to bind incomplete framebuffer");
        gl->glBindFramebuffer(target, mFBO);
    }

    void VRFramebuffer::blit(osg::GraphicsContext* gc, int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0, int dstX1, int dstY1, uint32_t bits, uint32_t filter)
    {
        auto* state = gc->getState();
        auto* gl = osg::GLExtensions::Get(state->getContextID(), false);
        gl->glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, mFBO);
        gl->glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, bits, filter);
        gl->glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, 0);
    }

    uint32_t VRFramebuffer::createImage(osg::GraphicsContext* gc, uint32_t formatInternal, uint32_t format)
    {
        auto* gl = osg::GLExtensions::Get(gc->getState()->getContextID(), false);
        uint32_t image;
        glGenTextures(1, &image);
        glBindTexture(mTextureTarget, image);
        if (mSamples <= 1)
        {
            glTexImage2D(mTextureTarget, 0, formatInternal, mWidth, mHeight, 0, format, GL_UNSIGNED_INT, nullptr);
            glTexParameteri(mTextureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER_ARB);
            glTexParameteri(mTextureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER_ARB);
            glTexParameteri(mTextureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(mTextureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        else
            gl->glTexImage2DMultisample(mTextureTarget, mSamples, format, mWidth, mHeight, false);
        
        glTexParameteri(mTextureTarget, GL_TEXTURE_MAX_LEVEL, 0);

        return image;
    }

    void VRFramebuffer::Texture::delet()
    {
        if (mOwner)
            glDeleteTextures(1, &mImage);

        mImage = 0;
        mOwner = false;
    }

    void VRFramebuffer::Texture::setTexture(uint32_t image, bool owner)
    {
        if (mImage)
            delet();

        mImage = image;
        mOwner = owner;
    }
}
