#ifndef XR_PLATFORM_HPP
#define XR_PLATFORM_HPP

#include <memory>
#include <string>
#include <vector>

#include <openxr/openxr.h>

#include <components/vr/constants.hpp>
#include <components/vr/directx.hpp>
#include <components/vr/swapchain.hpp>

#include <osg/GraphicsContext>

namespace XR
{
    struct PlatformPrivate;

    class Platform
    {
    public:
        Platform(osg::GraphicsContext* gc);
        ~Platform();

        void selectGraphicsAPIExtension();
        XrSession createXrSession(XrInstance instance, XrSystemId systemId, const std::string& systemName);

        const std::vector<GLenum>& supportedColorFormats() const { return mMWColorFormatsGL; }
        const std::vector<GLenum>& supportedDepthFormats() const { return mMWDepthFormatsGL; }

        void selectSwapchainFormat(VR::Swapchain::Attachment attachment, int64_t& swapchainFormat, GLenum glFormat);
        std::vector<std::unique_ptr<VR::SwapchainImage>> enumerateSwapchainImages(
            XrSwapchain swapchain, uint32_t textureTarget, uint64_t swapchainFormat);

    private:
        bool selectDirectX();
        bool selectOpenGL();

        std::unique_ptr<PlatformPrivate> mPrivate;
        std::shared_ptr<VR::DirectXWGLInterop> mDxInterop = nullptr;
        std::vector<int64_t> mRuntimeFormatsDX;
        std::vector<GLenum> mRuntimeFormatsGL;
        std::vector<GLenum> mMWColorFormatsGL;
        std::vector<GLenum> mMWDepthFormatsGL;
    };
}

#endif
