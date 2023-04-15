#ifndef VR_DIRECTX_H
#define VR_DIRECTX_H

#ifdef _WIN32
// This header contains classes and methods for sharing directx surfaces with OpenGL
// for use on platforms that offer directx surfaces but not opengl surfaces

#include <cstdint>
#include <map>
#include <memory>

#include "swapchain.hpp"

namespace osg
{
    class GraphicsContext;
}

namespace VR
{
    class DirectXSharedImage;
    struct DirectXWGLInteropPrivate;

    //! Translates the given OpenGL format to the equivalent DXGI format. Returns 0 if there is no equivalent.
    int64_t GLFormatToDXGIFormat(int64_t format);

    //! Translates the given DXGI format to the equivalent OpenGL format. Returns 0 if there is no equivalent.
    int64_t DXGIFormatToGLFormat(int64_t format);

    class DirectXWGLInterop
    {
    public:
        DirectXWGLInterop();
        ~DirectXWGLInterop();

        /// Registers an object for sharing as if calling wglDXRegisterObjectNV requesting write access.
        /// If ntShareHandle is not null, wglDXSetResourceShareHandleNV is called first to register the share handle
        void* DXRegisterObject(void* dxResource, uint32_t glName, uint32_t glType, bool discard, void* ntShareHandle);
        /// Unregisters an object from sharing as if calling wglDXUnregisterObjectNV
        void DXUnregisterObject(void* dxResourceShareHandle);
        /// Locks a DX object for use by OpenGL as if calling wglDXLockObjectsNV
        bool DXLockObject(void* dxResourceShareHandle);
        /// Unlocks a DX object for use by DirectX as if calling wglDXUnlockObjectsNV
        bool DXUnlockObject(void* dxResourceShareHandle);

        void* d3d11DeviceHandle();

        std::unique_ptr<DirectXWGLInteropPrivate> mPrivate;
    };

    class SwapchainImageDX : public VR::SwapchainImage
    {
    public:
        SwapchainImageDX(
            uint64_t image, uint32_t glTextureTarget, uint64_t dxFormat, std::shared_ptr<DirectXWGLInterop> wglInterop);
        ~SwapchainImageDX();
        virtual void beginFrame(osg::GraphicsContext* gc);
        virtual void endFrame(osg::GraphicsContext* gc);
        uint32_t glImage() const override;

        std::unique_ptr<DirectXSharedImage> mSharedImage;
    };
}

#else
namespace VR
{
    class DirectXWGLInterop
    {
    };
}
#endif

#endif
