#ifndef VR_SWAPCHAIN_H
#define VR_SWAPCHAIN_H

#include <cstdint>
#include <string>
#include <memory>
#include <map>

#include <osg/ref_ptr>

namespace osg
{
    class GraphicsContext;
    class FrameBufferObject;
    class RenderInfo;
}

namespace VR
{
    struct SwapchainConfig
    {
        uint32_t recommendedWidth = 0;
        uint32_t recommendedHeight = 0;
        uint32_t recommendedSamples = 0;
        uint32_t maxWidth = 0;
        uint32_t maxHeight = 0;
        uint32_t maxSamples = 0;
    };

    class SwapchainImage
    {
    public:
        virtual ~SwapchainImage() {}
        virtual void beginFrame(osg::GraphicsContext* gc) {}
        virtual void endFrame(osg::GraphicsContext* gc) {}

        virtual uint32_t glImage() const = 0;
    };

    class SwapchainImageGL : public VR::SwapchainImage
    {
    public:
        SwapchainImageGL(uint32_t image)
            : mImage(image)
        {
        }
        uint32_t glImage() const override { return mImage; }

        uint32_t mImage;
    };

    class Swapchain
    {
    public:
        enum class Attachment
        {
            Color,
            DepthStencil
        };

        Swapchain(uint32_t width, uint32_t height, uint32_t samples, uint32_t arraySize, Attachment attachment,
            const std::string& name);

        virtual ~Swapchain(){}

        //! Must be called before accessing an image from this swapchain
        virtual void beginFrame(osg::GraphicsContext* gc) = 0;

        //! Must be called once rendering operations are done, and before submitting the swapchain
        virtual void endFrame(osg::GraphicsContext* gc) = 0;

        //! Width of the surface
        uint32_t width() const { return mWidth; }

        //! Height of the surface
        uint32_t height() const { return mHeight; }

        //! Samples of the surface
        uint32_t samples() const { return mSamples; }

        //! Pixel format of the surface. \Note This is 0 until the swapchain has been initialized by the draw thread
        uint32_t format() const { return mFormat; }

        //! Array depth of the surface
        uint32_t arraySize() const { return mArraySize; }

        //! Array depth of the surface
        uint32_t textureTarget() const { return mTextureTarget; }

        //! Get the currently active swapchain image
        virtual SwapchainImage* image() = 0;

        //! Underlying handle
        virtual void* handle() const = 0;

        //! Whether or not the image must be flipped vertically when drawing into the swapchain
        virtual bool mustFlipVertical() const = 0;

    protected:
        uint32_t mWidth;
        uint32_t mHeight;
        uint32_t mSamples;
        uint32_t mFormat;
        uint32_t mArraySize;
        uint32_t mTextureTarget;
        Attachment mAttachment;
        std::string mName;

    private:
    };

    //! Class that handles mapping color swapchain and optional depth swapchain textures to framebuffers.
    //! Using the same Swapchain in multiple instances of this type is undefined behavior.
    //! Once an instance of SwapchainToFrameBufferObjectMapper has been created for a swapchain,
    //! beginFrame() and endFrame() should be called on the SwapchainToFrameBufferObjectMapper rather
    //! than on the swapchain itself.
     class SwapchainToFrameBufferObjectMapper
    {
     public:
         SwapchainToFrameBufferObjectMapper(std::shared_ptr<Swapchain> colorSwapchain, std::shared_ptr<Swapchain>
         depthSwapchain); ~SwapchainToFrameBufferObjectMapper();

        osg::FrameBufferObject* beginFrame(osg::RenderInfo& renderInfo);
        void endFrame(osg::RenderInfo& renderInfo);

     private:
         std::shared_ptr<Swapchain> mColorSwapchain = nullptr;
         std::shared_ptr<Swapchain> mDepthSwapchain = nullptr;

        using ColorDepthTexturePair = std::pair<uint32_t, uint32_t>;
        std::map<ColorDepthTexturePair, osg::ref_ptr< osg::FrameBufferObject >> mFramebuffers;
    };
}

#endif
