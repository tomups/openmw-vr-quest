#include "openxrswapchainimage.hpp"
#include "openxrmanagerimpl.hpp"
#include "vrenvironment.hpp"
#include "vrframebuffer.hpp"

// The OpenXR SDK's platform headers assume we've included platform headers
#ifdef _WIN32
#include <Windows.h>
#include <objbase.h>

#ifdef XR_USE_GRAPHICS_API_D3D11
#include <d3d11.h>
#include <dxgi1_2.h>
#endif

#elif __linux__
#include <X11/Xlib.h>
#include <GL/glx.h>
#undef None

#else
#error Unsupported platform
#endif

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_platform_defines.h>
#include <openxr/openxr_reflection.h>

#include <stdexcept>

#define GLERR if(auto err = glGetError() != GL_NO_ERROR) Log(Debug::Verbose) << __FILE__ << "." << __LINE__ << ": " << glGetError()

namespace MWVR {

    template<typename Image>
    class OpenXRSwapchainImageTemplate;

    template<>
    class OpenXRSwapchainImageTemplate< XrSwapchainImageOpenGLKHR > : public OpenXRSwapchainImage
    {
    public:
        static constexpr XrStructureType XrType = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;

    public:
        OpenXRSwapchainImageTemplate(osg::GraphicsContext* gc, XrSwapchainCreateInfo swapchainCreateInfo, const XrSwapchainImageOpenGLKHR& xrImage)
            : OpenXRSwapchainImage()
            , mXrImage(xrImage)
            , mBufferBits(0)
            , mFramebuffer(nullptr)
        {
            mFramebuffer.reset(new VRFramebuffer(gc->getState(), swapchainCreateInfo.width, swapchainCreateInfo.height, swapchainCreateInfo.sampleCount));
            if (swapchainCreateInfo.usageFlags & XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                mFramebuffer->setDepthBuffer(gc, mXrImage.image, false);
                mBufferBits = GL_DEPTH_BUFFER_BIT;
            }
            else
            {
                mFramebuffer->setColorBuffer(gc, mXrImage.image, false);
                mBufferBits = GL_COLOR_BUFFER_BIT;
            }
        }

        void blit(osg::GraphicsContext* gc, VRFramebuffer& readBuffer, int offset_x, int offset_y) override
        {
            mFramebuffer->bindFramebuffer(gc, GL_FRAMEBUFFER_EXT);
            readBuffer.blit(gc, offset_x, offset_y, offset_x + mFramebuffer->width(), offset_y + mFramebuffer->height(), 0, 0, mFramebuffer->width(), mFramebuffer->height(), mBufferBits, GL_NEAREST);
        }

        XrSwapchainImageOpenGLKHR mXrImage;
        uint32_t mBufferBits;
        std::unique_ptr<VRFramebuffer> mFramebuffer;
    };

#ifdef XR_USE_GRAPHICS_API_D3D11
    template<>
    class OpenXRSwapchainImageTemplate< XrSwapchainImageD3D11KHR > : public OpenXRSwapchainImage
    {
    public:
        static constexpr XrStructureType XrType = XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR;
    public:
        OpenXRSwapchainImageTemplate(osg::GraphicsContext* gc, XrSwapchainCreateInfo swapchainCreateInfo, const XrSwapchainImageD3D11KHR& xrImage)
            : OpenXRSwapchainImage()
            , mXrImage(xrImage)
            , mBufferBits(0)
            , mFramebuffer(nullptr)
        {
            mXrImage.texture->GetDevice(&mDevice);
            mDevice->GetImmediateContext(&mDeviceContext);

            mXrImage.texture->GetDesc(&mDesc);

            glGenTextures(1, &mGlTextureName);

            auto* xr = Environment::get().getManager();
            //mDxResourceShareHandle = xr->impl().platform().DXRegisterObject(mXrImage.texture, mGlTextureName, GL_TEXTURE_2D, true, nullptr);

            if (!mDxResourceShareHandle)
            {
                // Some OpenXR runtimes return textures that cannot be directly shared.
                // So we need to make a redundant texture to use as an intermediary...
                mSharedTextureDesc.Width = mDesc.Width;
                mSharedTextureDesc.Height = mDesc.Height;
                mSharedTextureDesc.MipLevels = mDesc.MipLevels;
                mSharedTextureDesc.ArraySize = mDesc.ArraySize;
                mSharedTextureDesc.Format = static_cast<DXGI_FORMAT>(swapchainCreateInfo.format);
                mSharedTextureDesc.SampleDesc = mDesc.SampleDesc;
                mSharedTextureDesc.Usage = D3D11_USAGE_DEFAULT;
                mSharedTextureDesc.BindFlags = 0;
                mSharedTextureDesc.CPUAccessFlags = 0;
                mSharedTextureDesc.MiscFlags = 0;;

                mDevice->CreateTexture2D(&mSharedTextureDesc, nullptr, &mSharedTexture);
                mXrImage.texture->GetDesc(&mSharedTextureDesc);
                mDxResourceShareHandle = xr->impl().platform().DXRegisterObject(mSharedTexture, mGlTextureName, GL_TEXTURE_2D, true, nullptr);
            }

            // Set up shared texture as blit target
            mFramebuffer.reset(new VRFramebuffer(gc->getState(), swapchainCreateInfo.width, swapchainCreateInfo.height, swapchainCreateInfo.sampleCount));

            if (swapchainCreateInfo.usageFlags & XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                mFramebuffer->setDepthBuffer(gc, mGlTextureName, false);
                mBufferBits = GL_DEPTH_BUFFER_BIT;
            }
            else
            {
                mFramebuffer->setColorBuffer(gc, mGlTextureName, false);
                mBufferBits = GL_COLOR_BUFFER_BIT;
            }
        }

        ~OpenXRSwapchainImageTemplate()
        {
            auto* xr = Environment::get().getManager();
            if (mDxResourceShareHandle)
                xr->impl().platform().DXUnregisterObject(mDxResourceShareHandle);
            glDeleteTextures(1, &mGlTextureName);
        }

        void blit(osg::GraphicsContext* gc, VRFramebuffer& readBuffer, int offset_x, int offset_y) override
        {
            // Blit readBuffer into directx texture, while flipping the Y axis.
            auto* xr = Environment::get().getManager();
            xr->impl().platform().DXLockObject(mDxResourceShareHandle);
            mFramebuffer->bindFramebuffer(gc, GL_FRAMEBUFFER_EXT);
            readBuffer.blit(gc, offset_x, offset_y, offset_x + mFramebuffer->width(), offset_y + mFramebuffer->height(), 0, mFramebuffer->height(), mFramebuffer->width(), 0, mBufferBits, GL_NEAREST);
            xr->impl().platform().DXUnlockObject(mDxResourceShareHandle);
            

            // If the d3d11 texture couldn't be shared directly, blit it again.
            if (mSharedTexture)
            {
                mDeviceContext->CopyResource(mXrImage.texture, mSharedTexture);
            }
        }

        ID3D11Device* mDevice = nullptr;
        ID3D11DeviceContext* mDeviceContext = nullptr;
        D3D11_TEXTURE2D_DESC mDesc;
        D3D11_TEXTURE2D_DESC mSharedTextureDesc;
        ID3D11Texture2D* mSharedTexture = nullptr;
        uint32_t mGlTextureName = 0;
        void* mDxResourceShareHandle = nullptr;

        XrSwapchainImageD3D11KHR mXrImage;
        uint32_t mBufferBits;
        std::unique_ptr<VRFramebuffer> mFramebuffer;
    };
#endif


    template< typename Image > static inline
    std::vector<std::unique_ptr<OpenXRSwapchainImage> > 
        enumerateSwapchainImagesImpl(osg::GraphicsContext* gc, XrSwapchain swapchain, XrSwapchainCreateInfo swapchainCreateInfo)
    {
        using SwapchainImage = OpenXRSwapchainImageTemplate<Image>;

        uint32_t imageCount = 0;
        std::vector< Image > images;
        CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain, 0, &imageCount, nullptr));
        images.resize(imageCount, { SwapchainImage::XrType });
        CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain, imageCount, &imageCount, reinterpret_cast<XrSwapchainImageBaseHeader*>(images.data())));

        std::vector<std::unique_ptr<OpenXRSwapchainImage> > swapchainImages;
        for(auto& image: images)
        {
            swapchainImages.emplace_back(new OpenXRSwapchainImageTemplate<Image>(gc, swapchainCreateInfo, image));
        }

        return swapchainImages;
    }

    std::vector<std::unique_ptr<OpenXRSwapchainImage> > 
        OpenXRSwapchainImage::enumerateSwapchainImages(osg::GraphicsContext* gc, XrSwapchain swapchain, XrSwapchainCreateInfo swapchainCreateInfo)
    {
        auto* xr = Environment::get().getManager();

        if (xr->xrExtensionIsEnabled(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME))
        {
            return enumerateSwapchainImagesImpl<XrSwapchainImageOpenGLKHR>(gc, swapchain, swapchainCreateInfo);
        }
#ifdef XR_USE_GRAPHICS_API_D3D11
        else if (xr->xrExtensionIsEnabled(XR_KHR_D3D11_ENABLE_EXTENSION_NAME))
        {
            return enumerateSwapchainImagesImpl<XrSwapchainImageD3D11KHR>(gc, swapchain, swapchainCreateInfo);
        }
#endif
        else
        {
            throw std::logic_error("Implementation missing for selected graphics API");
        }

        return std::vector<std::unique_ptr<OpenXRSwapchainImage>>();
    }

    OpenXRSwapchainImage::OpenXRSwapchainImage()
    {
    }
}
