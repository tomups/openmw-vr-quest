#include "swapchain.hpp"
#include "frame.hpp"

#include <components/stereo/multiview.hpp>

#include "swapchain.hpp"
#include <osg/FrameBufferObject>
#include <osg/RenderInfo>

namespace VR
{
    Swapchain::Swapchain(uint32_t width, uint32_t height, uint32_t samples, uint32_t arraySize, Attachment attachment,
        const std::string& name)
        : mWidth(width)
        , mHeight(height)
        , mSamples(samples)
        , mFormat(0)
        , mArraySize(arraySize)
        , mTextureTarget(arraySize > 1 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D)
        , mAttachment(attachment)
        , mName(name)
    {
    }

    void Swapchain::beginFrame(osg::GraphicsContext* gc) {}

    void Swapchain::endFrame(osg::GraphicsContext* gc) {}

     SwapchainToFrameBufferObjectMapper::SwapchainToFrameBufferObjectMapper(std::shared_ptr<Swapchain> colorSwapchain,
     std::shared_ptr<Swapchain> depthSwapchain)
         : mColorSwapchain(colorSwapchain)
         , mDepthSwapchain(depthSwapchain)
    {
     }

     SwapchainToFrameBufferObjectMapper::~SwapchainToFrameBufferObjectMapper()
    {
     }

     osg::FrameBufferObject* SwapchainToFrameBufferObjectMapper::beginFrame(osg::RenderInfo& renderInfo)
    {
         auto* state = renderInfo.getState();
         auto* gc = state->getGraphicsContext();
         uint32_t colorTexture = 0;
         uint32_t depthTexture = 0;

        if (mColorSwapchain)
         {
             mColorSwapchain->beginFrame(gc);
             colorTexture = mColorSwapchain->image()->glImage();
         }
         if (mDepthSwapchain)
         {
             mDepthSwapchain->beginFrame(gc);
             depthTexture = mDepthSwapchain->image()->glImage();
         }

        auto pair = ColorDepthTexturePair(colorTexture, depthTexture);

        auto it = mFramebuffers.find(pair);
        if (it != mFramebuffers.end())
            return it->second.get();

        osg::FrameBufferObject* fbo = new osg::FrameBufferObject;
        mFramebuffers[pair] = fbo;

        if (colorTexture)
        {
            auto textureTarget = mColorSwapchain->textureTarget();
            auto width = mColorSwapchain->width();
            auto height = mColorSwapchain->height();
            auto colorAttachment = Stereo::createLayerAttachmentFromHandle(state, colorTexture, textureTarget, width,
            height, 0); fbo->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER, colorAttachment);

        }

        if (depthTexture != 0)
        {
            auto textureTarget = mDepthSwapchain->textureTarget();
            auto width = mDepthSwapchain->width();
            auto height = mDepthSwapchain->height();
            auto depthAttachment = Stereo::createLayerAttachmentFromHandle(state, depthTexture, textureTarget, width,
            height, 0); fbo->setAttachment(osg::FrameBufferObject::BufferComponent::DEPTH_BUFFER, depthAttachment);
        }

        return fbo;
    }

     void SwapchainToFrameBufferObjectMapper::endFrame(osg::RenderInfo& renderInfo)
    {
         auto* state = renderInfo.getState();
         auto* gc = state->getGraphicsContext();
         if (mColorSwapchain)
             mColorSwapchain->endFrame(gc);
         if (mDepthSwapchain)
             mDepthSwapchain->endFrame(gc);
     }
}
