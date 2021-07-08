#ifndef OPENXR_SWAPCHAINIMPL_HPP
#define OPENXR_SWAPCHAINIMPL_HPP

#include "openxrswapchainimage.hpp"
#include "openxrmanagerimpl.hpp"

struct XrSwapchainSubImage;

namespace MWVR
{
    class VRFramebuffer;

    /// \brief Implementation of OpenXRSwapchain
    class OpenXRSwapchainImpl
    {
    private:
        struct SwapchainPrivate
        {
            enum class Use {
                COLOR = 0,
                DEPTH = 1
            };

            SwapchainPrivate(osg::ref_ptr<osg::State> state, SwapchainConfig config, Use use);
            ~SwapchainPrivate();

            uint32_t count() const;
            bool isAcquired() const;
            uint32_t acuiredIndex() const { return mAcquiredIndex; };
            XrSwapchain xrSwapchain(void) const { return mSwapchain; };
            XrSwapchainSubImage xrSubImage(void) const { return mSubImage; };
            int width() const { return mWidth; };
            int height() const { return mHeight; };
            int samples() const { return mSamples; };

            void acquire(osg::GraphicsContext* gc);
            void blitAndRelease(osg::GraphicsContext* gc, VRFramebuffer& readBuffer);
            void checkAcquired() const;

        protected:

        private:
            SwapchainConfig mConfig;
            XrSwapchain mSwapchain = XR_NULL_HANDLE;
            XrSwapchainSubImage mSubImage{};
            std::vector< std::unique_ptr<OpenXRSwapchainImage> > mImages;
            int32_t mWidth = -1;
            int32_t mHeight = -1;
            int32_t mSamples = -1;
            int64_t mFormat = 0;
            uint32_t mAcquiredIndex{ 0 };
            Use mUsage;
            bool mIsIndexAcquired{ false };
            bool mIsReady{ false };
        };

    public:
        OpenXRSwapchainImpl(osg::ref_ptr<osg::State> state, SwapchainConfig config);
        ~OpenXRSwapchainImpl();

        void beginFrame(osg::GraphicsContext* gc);
        void endFrame(osg::GraphicsContext* gc, VRFramebuffer& readBuffer);

        bool isAcquired() const;
        XrSwapchain xrSwapchain(void) const;
        XrSwapchain xrSwapchainDepth(void) const;
        XrSwapchainSubImage xrSubImage(void) const;
        XrSwapchainSubImage xrSubImageDepth(void) const;
        int width() const { return mConfig.selectedWidth; };
        int height() const { return mConfig.selectedHeight; };
        int samples() const { return mConfig.selectedSamples; };

    protected:
        OpenXRSwapchainImpl(const OpenXRSwapchainImpl&) = delete;
        void operator=(const OpenXRSwapchainImpl&) = delete;

        void acquire(osg::GraphicsContext* gc);
        void release(osg::GraphicsContext* gc, VRFramebuffer& readBuffer);
        void checkAcquired() const;

    private:
        SwapchainConfig mConfig;
        std::unique_ptr<SwapchainPrivate> mSwapchain{ nullptr };
        std::unique_ptr<SwapchainPrivate> mSwapchainDepth{ nullptr };
        bool mFormallyAcquired{ false };
        bool mShouldRelease{ false };
    };
}

#endif
