#include "directx.hpp"
#ifdef _WIN32

#ifdef NOGDI
#undef NOGDI
#endif

#include <Windows.h>
#include <objbase.h>

#include <d3d11.h>
#include <dxgi1_2.h>

#include <gl/GL.h>

#include <stdexcept>

#include <osg/GraphicsContext>

#include <components/debug/debuglog.hpp>
#include <components/sceneutil/color.hpp>
#include <components/sceneutil/depth.hpp>

#ifndef GL_DEPTH32F_STENCIL8
#define GL_DEPTH32F_STENCIL8 0x8CAD
#endif

#ifndef GL_DEPTH32F_STENCIL8_NV
#define GL_DEPTH32F_STENCIL8_NV 0x8DAC
#endif

#ifndef GL_DEPTH24_STENCIL8
#define GL_DEPTH24_STENCIL8 0x88F0
#endif

namespace VR
{

    struct DirectXWGLInteropPrivate
    {
        DirectXWGLInteropPrivate();
        ~DirectXWGLInteropPrivate();

        typedef BOOL(WINAPI* P_wglDXSetResourceShareHandleNV)(void* dxObject, HANDLE shareHandle);
        typedef HANDLE(WINAPI* P_wglDXOpenDeviceNV)(void* dxDevice);
        typedef BOOL(WINAPI* P_wglDXCloseDeviceNV)(HANDLE hDevice);
        typedef HANDLE(WINAPI* P_wglDXRegisterObjectNV)(
            HANDLE hDevice, void* dxObject, GLuint name, GLenum type, GLenum access);
        typedef BOOL(WINAPI* P_wglDXUnregisterObjectNV)(HANDLE hDevice, HANDLE hObject);
        typedef BOOL(WINAPI* P_wglDXObjectAccessNV)(HANDLE hObject, GLenum access);
        typedef BOOL(WINAPI* P_wglDXLockObjectsNV)(HANDLE hDevice, GLint count, HANDLE* hObjects);
        typedef BOOL(WINAPI* P_wglDXUnlockObjectsNV)(HANDLE hDevice, GLint count, HANDLE* hObjects);

        bool mWGL_NV_DX_interop2 = false;
        ID3D11Device* mD3D11Device = nullptr;
        ID3D11DeviceContext* mD3D11ImmediateContext = nullptr;
        HMODULE mD3D11Dll = nullptr;
        PFN_D3D11_CREATE_DEVICE pD3D11CreateDevice = nullptr;

        P_wglDXSetResourceShareHandleNV wglDXSetResourceShareHandleNV = nullptr;
        P_wglDXOpenDeviceNV wglDXOpenDeviceNV = nullptr;
        P_wglDXCloseDeviceNV wglDXCloseDeviceNV = nullptr;
        P_wglDXRegisterObjectNV wglDXRegisterObjectNV = nullptr;
        P_wglDXUnregisterObjectNV wglDXUnregisterObjectNV = nullptr;
        P_wglDXObjectAccessNV wglDXObjectAccessNV = nullptr;
        P_wglDXLockObjectsNV wglDXLockObjectsNV = nullptr;
        P_wglDXUnlockObjectsNV wglDXUnlockObjectsNV = nullptr;

        HANDLE wglDXDevice = nullptr;
    };

    class DirectXSharedImage
    {
    public:
        DirectXSharedImage(uint64_t image, uint32_t glTextureTarget, DXGI_FORMAT format,
            std::shared_ptr<DirectXWGLInterop> wglInterop);
        ~DirectXSharedImage();

        void beginFrame(osg::GraphicsContext* gc);
        void endFrame(osg::GraphicsContext* gc);
        uint32_t glImage() const { return mGlTextureName; }

    protected:
        std::shared_ptr<DirectXWGLInterop> mWglInterop;
        ID3D11Device* mDevice = nullptr;
        ID3D11DeviceContext* mDeviceContext = nullptr;
        ID3D11Texture2D* mSharedTexture = nullptr;
        uint32_t mGlTextureName = 0;
        void* mDxResourceShareHandle = nullptr;

        ID3D11Texture2D* mDxImage;
    };

    DirectXSharedImage::DirectXSharedImage(
        uint64_t image, uint32_t glTextureTarget, DXGI_FORMAT format, std::shared_ptr<DirectXWGLInterop> wglInterop)
        : mWglInterop(wglInterop)
        , mDxImage(reinterpret_cast<ID3D11Texture2D*>(image))
    {
        mDxImage->GetDevice(&mDevice);
        mDevice->GetImmediateContext(&mDeviceContext);

        glGenTextures(1, &mGlTextureName);

        D3D11_TEXTURE2D_DESC desc{};
        mDxImage->GetDesc(&desc);
        // mDxResourceShareHandle = mWglInterop->DXRegisterObject(mDxImage, mGlTextureName, glTextureTarget, true,
        // nullptr);

        if (!mDxResourceShareHandle)
        {
            // Some OpenXR runtimes return textures that cannot be directly shared.
            // So we need to make a second texture to use as an intermediary.
            D3D11_TEXTURE2D_DESC sharedTextureDesc{};
            sharedTextureDesc.Width = desc.Width;
            sharedTextureDesc.Height = desc.Height;
            sharedTextureDesc.MipLevels = desc.MipLevels;
            sharedTextureDesc.ArraySize = desc.ArraySize;
            sharedTextureDesc.Format = static_cast<DXGI_FORMAT>(format);
            sharedTextureDesc.SampleDesc = desc.SampleDesc;
            sharedTextureDesc.Usage = D3D11_USAGE_DEFAULT;
            sharedTextureDesc.BindFlags = 0;
            sharedTextureDesc.CPUAccessFlags = 0;
            sharedTextureDesc.MiscFlags = 0;
            ;

            // mDevice->CreateTexture2D(&sharedTextureDesc, nullptr, &mSharedTexture);
            mDevice->CreateTexture2D(&sharedTextureDesc, nullptr, &mSharedTexture);

            mDxResourceShareHandle
                = mWglInterop->DXRegisterObject(mSharedTexture, mGlTextureName, glTextureTarget, true, nullptr);
        }
    }

    DirectXSharedImage::~DirectXSharedImage()
    {
        mWglInterop->DXUnregisterObject(mDxResourceShareHandle);
    }

    void DirectXSharedImage::beginFrame(osg::GraphicsContext* gc)
    {
        if (!mWglInterop->DXLockObject(mDxResourceShareHandle))
            Log(Debug::Verbose) << "DXLockObject failed";
    }

    void DirectXSharedImage::endFrame(osg::GraphicsContext* gc)
    {
        if (!mWglInterop->DXUnlockObject(mDxResourceShareHandle))
            Log(Debug::Verbose) << "DXUnlockObject failed";
        if (mSharedTexture)
        {
            mDeviceContext->CopyResource(mDxImage, mSharedTexture);
        }
    }

    DirectXWGLInteropPrivate::DirectXWGLInteropPrivate()
    {
        mD3D11Dll = LoadLibrary("D3D11.dll");

        if (!mD3D11Dll)
            throw std::runtime_error("Current OpenXR runtime requires DirectX >= 11.0 but D3D11.dll was not found.");

        pD3D11CreateDevice = reinterpret_cast<PFN_D3D11_CREATE_DEVICE>(GetProcAddress(mD3D11Dll, "D3D11CreateDevice"));

        if (!pD3D11CreateDevice)
            throw std::runtime_error("Symbol 'D3D11CreateDevice' not found in D3D11.dll");

        // Create the device and device context objects
        pD3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &mD3D11Device,
            nullptr, &mD3D11ImmediateContext);

#define LOAD_WGL(a)                                                                                                    \
    a = reinterpret_cast<decltype(a)>(wglGetProcAddress(#a));                                                          \
    if (!a)                                                                                                            \
    throw std::runtime_error(                                                                                          \
        "Extension WGL_NV_DX_interop2 required to run OpenMW VR via DirectX missing expected symbol '" #a "'.")
        LOAD_WGL(wglDXSetResourceShareHandleNV);
        LOAD_WGL(wglDXOpenDeviceNV);
        LOAD_WGL(wglDXCloseDeviceNV);
        LOAD_WGL(wglDXRegisterObjectNV);
        LOAD_WGL(wglDXUnregisterObjectNV);
        LOAD_WGL(wglDXObjectAccessNV);
        LOAD_WGL(wglDXLockObjectsNV);
        LOAD_WGL(wglDXUnlockObjectsNV);
#undef LOAD_WGL

        wglDXDevice = wglDXOpenDeviceNV(mD3D11Device);
    }

    DirectXWGLInteropPrivate::~DirectXWGLInteropPrivate() {}

    DirectXWGLInterop::DirectXWGLInterop()
        : mPrivate(std::make_unique<DirectXWGLInteropPrivate>())
    {
    }

    DirectXWGLInterop::~DirectXWGLInterop() {}

    void* DirectXWGLInterop::DXRegisterObject(
        void* dxResource, uint32_t glName, uint32_t glType, bool discard, void* ntShareHandle)
    {
        if (ntShareHandle)
        {
            mPrivate->wglDXSetResourceShareHandleNV(dxResource, ntShareHandle);
        }
        return mPrivate->wglDXRegisterObjectNV(mPrivate->wglDXDevice, dxResource, glName, glType, 1);
    }

    void DirectXWGLInterop::DXUnregisterObject(void* dxResourceShareHandle)
    {
        mPrivate->wglDXUnregisterObjectNV(mPrivate->wglDXDevice, dxResourceShareHandle);
    }

    bool DirectXWGLInterop::DXLockObject(void* dxResourceShareHandle)
    {
        return !!mPrivate->wglDXLockObjectsNV(mPrivate->wglDXDevice, 1, &dxResourceShareHandle);
    }

    bool DirectXWGLInterop::DXUnlockObject(void* dxResourceShareHandle)
    {
        return !!mPrivate->wglDXUnlockObjectsNV(mPrivate->wglDXDevice, 1, &dxResourceShareHandle);
    }

    void* DirectXWGLInterop::d3d11DeviceHandle()
    {
        return mPrivate->mD3D11Device;
    }

    int64_t GLFormatToDXGIFormat(int64_t format)
    {
        switch (format)
        {
            case GL_RGBA32F: //
                return DXGI_FORMAT_R32G32B32A32_FLOAT;
            case GL_RGBA32UI: //
                return DXGI_FORMAT_R32G32B32A32_UINT;
            case GL_RGBA32I: //
                return DXGI_FORMAT_R32G32B32A32_SINT;

            case GL_RGB32F: //
                return DXGI_FORMAT_R32G32B32_FLOAT;
            case GL_RGB32UI: //
                return DXGI_FORMAT_R32G32B32_UINT;
            case GL_RGB32I: //
                return DXGI_FORMAT_R32G32B32_SINT;

            case GL_RGBA16F: //
                return DXGI_FORMAT_R16G16B16A16_FLOAT;
            case GL_RGBA16: //
                return DXGI_FORMAT_R16G16B16A16_UNORM;
            case GL_RGBA16UI: //
                return DXGI_FORMAT_R16G16B16A16_UINT;
            case GL_RGBA16_SNORM: //
                return DXGI_FORMAT_R16G16B16A16_SNORM;
            case GL_RGBA16I: //
                return DXGI_FORMAT_R16G16B16A16_SINT;

            case GL_RGBA8: //
                return DXGI_FORMAT_R8G8B8A8_UNORM;
            case GL_SRGB8_ALPHA8: //
                return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            case GL_RGBA8UI: //
                return DXGI_FORMAT_R8G8B8A8_UINT;
            case GL_RGBA8_SNORM: //
                return DXGI_FORMAT_R8G8B8A8_SNORM;
            case GL_RGBA8I: //
                return DXGI_FORMAT_R8G8B8A8_SINT;

            case GL_RGB10_A2: //
                return DXGI_FORMAT_R10G10B10A2_UNORM;
            case GL_RGB10_A2UI: //
                return DXGI_FORMAT_R10G10B10A2_UINT;

            case GL_R11F_G11F_B10F: //
                return DXGI_FORMAT_R11G11B10_FLOAT;

            case GL_DEPTH32F_STENCIL8: //
                return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
            case GL_DEPTH_COMPONENT32F: //
                return DXGI_FORMAT_D32_FLOAT;
            case GL_DEPTH24_STENCIL8: //
                return DXGI_FORMAT_D24_UNORM_S8_UINT;
            case GL_DEPTH_COMPONENT16: //
                return DXGI_FORMAT_D16_UNORM;
            default:
                return 0;
        }
    }

    int64_t DXGIFormatToGLFormat(int64_t format)
    {
        switch (format)
        {
            case DXGI_FORMAT_R32G32B32A32_FLOAT:
                return GL_RGBA32F; //
            case DXGI_FORMAT_R32G32B32A32_UINT:
                return GL_RGBA32UI; //
            case DXGI_FORMAT_R32G32B32A32_SINT:
                return GL_RGBA32I; //

            case DXGI_FORMAT_R32G32B32_FLOAT:
                return GL_RGB32F; //
            case DXGI_FORMAT_R32G32B32_UINT:
                return GL_RGB32UI; //
            case DXGI_FORMAT_R32G32B32_SINT:
                return GL_RGB32I; //

            case DXGI_FORMAT_R16G16B16A16_FLOAT:
                return GL_RGBA16F; //
            case DXGI_FORMAT_R16G16B16A16_UNORM:
                return GL_RGBA16; //
            case DXGI_FORMAT_R16G16B16A16_UINT:
                return GL_RGBA16UI; //
            case DXGI_FORMAT_R16G16B16A16_SNORM:
                return GL_RGBA16_SNORM; //
            case DXGI_FORMAT_R16G16B16A16_SINT:
                return GL_RGBA16I; //

            case DXGI_FORMAT_R8G8B8A8_UNORM:
                return GL_RGBA8; //
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
                return GL_SRGB8_ALPHA8; //
            case DXGI_FORMAT_R8G8B8A8_UINT:
                return GL_RGBA8UI; //
            case DXGI_FORMAT_R8G8B8A8_SNORM:
                return GL_RGBA8_SNORM; //
            case DXGI_FORMAT_R8G8B8A8_SINT:
                return GL_RGBA8I; //

            case DXGI_FORMAT_R10G10B10A2_UNORM:
                return GL_RGB10_A2; //
            case DXGI_FORMAT_R10G10B10A2_UINT:
                return GL_RGB10_A2UI; //

            case DXGI_FORMAT_R11G11B10_FLOAT:
                return GL_R11F_G11F_B10F; //

            case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
                return GL_DEPTH32F_STENCIL8; //
            case DXGI_FORMAT_D32_FLOAT:
                return GL_DEPTH_COMPONENT32F; //
            case DXGI_FORMAT_D24_UNORM_S8_UINT:
                return GL_DEPTH24_STENCIL8; //
            case DXGI_FORMAT_D16_UNORM:
                return GL_DEPTH_COMPONENT16; //
            default:
                return 0;
        }
    }

    SwapchainImageDX::SwapchainImageDX(
        uint64_t image, uint32_t glTextureTarget, uint64_t dxFormat, std::shared_ptr<DirectXWGLInterop> wglInterop)
    {
        mSharedImage = std::make_unique<DirectXSharedImage>(
            image, glTextureTarget, static_cast<DXGI_FORMAT>(dxFormat), wglInterop);
    }

    SwapchainImageDX::~SwapchainImageDX() {}

    void SwapchainImageDX::beginFrame(osg::GraphicsContext* gc)
    {
        mSharedImage->beginFrame(gc);
    }

    void SwapchainImageDX::endFrame(osg::GraphicsContext* gc)
    {
        mSharedImage->endFrame(gc);
    }

    uint32_t SwapchainImageDX::glImage() const
    {
        return mSharedImage->glImage();
    }
}
#endif
