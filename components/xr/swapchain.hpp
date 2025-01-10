#ifndef OPENXR_SWAPCHAIN_HPP
#define OPENXR_SWAPCHAIN_HPP

#include <components/vr/swapchain.hpp>
#include <openxr/openxr.h>

#include <vector>
#include <memory>

struct XrSwapchainSubImage;

namespace XR
{
    class OpenXRSwapchainImpl;
    class VRFramebuffer;

    class Swapchain : public VR::Swapchain
    {
    public:
        using VR::Swapchain::Swapchain;
        ~Swapchain() override;

        XrSwapchain xrSwapchain() const { return mXrSwapchain; }

    protected:
        //! Acquire a rendering surface from this swapchain
        void beginFrame(osg::GraphicsContext* gc) override;

        //! Release the rendering surface
        void endFrame(osg::GraphicsContext* gc) override;

        //! Return the xr swapchain
        void* handle() const override { return mXrSwapchain; }

        VR::SwapchainImage* image() override;

        bool mustFlipVertical() const override;

    private:
        void init();

        void acquire();
        void wait();
        void release();
        bool isAcquired();

        uint32_t mAcquiredIndex = 0;
        bool mIsAcquired = false;
        bool mIsReady = false;
        XrSwapchain mXrSwapchain = nullptr;
        std::vector<std::unique_ptr<VR::SwapchainImage>> mImages;
    };
}

#endif
