#include "openxrswapchainimpl.hpp"
#include "openxrdebug.hpp"
#include "vrenvironment.hpp"
#include "vrframebuffer.hpp"

#include <components/debug/debuglog.hpp>

namespace MWVR {
    OpenXRSwapchainImpl::OpenXRSwapchainImpl(osg::ref_ptr<osg::State> state, SwapchainConfig config)
        : mConfig(config)
    {
        if (mConfig.selectedWidth <= 0)
            throw std::invalid_argument("Width must be a positive integer");
        if (mConfig.selectedHeight <= 0)
            throw std::invalid_argument("Height must be a positive integer");
        if (mConfig.selectedSamples <= 0)
            throw std::invalid_argument("Samples must be a positive integer");

        mSwapchain.reset(new SwapchainPrivate(state, mConfig, SwapchainPrivate::Use::COLOR));
        mConfig.selectedSamples = mSwapchain->samples();

        auto* xr = Environment::get().getManager();

        if (xr->xrExtensionIsEnabled(XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME))
        {
            try
            {
                mSwapchainDepth.reset(new SwapchainPrivate(state, mConfig, SwapchainPrivate::Use::DEPTH));
            }
            catch (std::exception& e)
            {
                Log(Debug::Warning) << "XR_KHR_composition_layer_depth was enabled but creating depth swapchain failed: " << e.what();
                mSwapchainDepth = nullptr;
            }
        }
    }

    OpenXRSwapchainImpl::~OpenXRSwapchainImpl()
    {
    }

    bool OpenXRSwapchainImpl::isAcquired() const
    {
        return mFormallyAcquired;
    }

    void OpenXRSwapchainImpl::beginFrame(osg::GraphicsContext* gc)
    {
        acquire(gc);
    }

    int swapCount = 0;

    void OpenXRSwapchainImpl::endFrame(osg::GraphicsContext* gc, VRFramebuffer& readBuffer)
    {
        checkAcquired();
        release(gc, readBuffer);
    }

    void OpenXRSwapchainImpl::acquire(osg::GraphicsContext* gc)
    {
        if (isAcquired())
            throw std::logic_error("Trying to acquire already acquired swapchain");

        // The openxr runtime may fail to acquire/release.
        // Do not re-acquire a swapchain before having successfully released it.
        // Lest the swapchain fall out of sync.
        if (!mShouldRelease)
        {
            mSwapchain->acquire(gc);
            mShouldRelease = mSwapchain->isAcquired();
            if (mSwapchainDepth && mSwapchain->isAcquired())
            {
                mSwapchainDepth->acquire(gc);
                mShouldRelease = mSwapchainDepth->isAcquired();
            }
        }

        mFormallyAcquired = true;
    }

    void OpenXRSwapchainImpl::release(osg::GraphicsContext* gc, VRFramebuffer& readBuffer)
    {
        // The openxr runtime may fail to acquire/release.
        // Do not release a swapchain before having successfully acquire it.
        if (mShouldRelease)
        {
            mSwapchain->blitAndRelease(gc, readBuffer);
            mShouldRelease = mSwapchain->isAcquired();
            if (mSwapchainDepth)
            {
                mSwapchainDepth->blitAndRelease(gc, readBuffer);
                mShouldRelease = mSwapchainDepth->isAcquired();
            }
        }

        mFormallyAcquired = false;
    }

    void OpenXRSwapchainImpl::checkAcquired() const
    {
        if (!isAcquired())
            throw std::logic_error("Swapchain must be acquired before use. Call between OpenXRSwapchain::beginFrame() and OpenXRSwapchain::endFrame()");
    }

    OpenXRSwapchainImpl::SwapchainPrivate::SwapchainPrivate(osg::ref_ptr<osg::State> state, SwapchainConfig config, Use use)
        : mConfig(config)
        , mImages()
        , mWidth(config.selectedWidth)
        , mHeight(config.selectedHeight)
        , mSamples(1)
        , mUsage(use)
    {
        auto* xr = Environment::get().getManager();

        XrSwapchainCreateInfo swapchainCreateInfo{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
        swapchainCreateInfo.arraySize = 1;
        swapchainCreateInfo.width = mWidth;
        swapchainCreateInfo.height = mHeight;
        swapchainCreateInfo.mipCount = 1;
        swapchainCreateInfo.faceCount = 1;

        while (mSamples > 0 && mSwapchain == XR_NULL_HANDLE && mFormat == 0)
        {
            // Select a swapchain format.
            if (use == Use::COLOR)
                mFormat = xr->selectColorFormat();
            else
                mFormat = xr->selectDepthFormat();
            std::string typeString = use == Use::COLOR ? "color" : "depth";
            if (mFormat == 0) {
                throw std::runtime_error(std::string("Swapchain ") + typeString + " format not supported");
            }
            Log(Debug::Verbose) << "Selected " << typeString << " format: " << std::dec << mFormat << " (" << std::hex << mFormat << ")" << std::dec;

            // Now create the swapchain
            Log(Debug::Verbose) << "Creating swapchain with dimensions Width=" << mWidth << " Heigh=" << mHeight << " SampleCount=" << mSamples;
            swapchainCreateInfo.format = mFormat;
            swapchainCreateInfo.sampleCount = mSamples;
            if(mUsage == Use::COLOR)
                swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
            else
                swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            auto res = xrCreateSwapchain(xr->impl().xrSession(), &swapchainCreateInfo, &mSwapchain);

            // Check errors and try again if possible
            if (res == XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED)
            {
                // We only try swapchain formats enumerated by the runtime itself.
                // This does not guarantee that that swapchain format is going to be supported for this specific usage.
                Log(Debug::Verbose) << "Failed to create swapchain with Format=" << mFormat<< ": " << XrResultString(res);
                xr->eraseFormat(mFormat);
                mFormat = 0;
                continue;
            }
            else if (!XR_SUCCEEDED(res))
            {
                Log(Debug::Verbose) << "Failed to create swapchain with SampleCount=" << mSamples << ": " << XrResultString(res);
                mSamples /= 2;
                if (mSamples == 0)
                {
                    CHECK_XRRESULT(res, "xrCreateSwapchain");
                    throw std::runtime_error(XrResultString(res));
                }
                continue;
            }

            CHECK_XRRESULT(res, "xrCreateSwapchain");
            VrDebug::setName(mSwapchain, "OpenMW XR Color Swapchain " + config.name);
        }

        // TODO: here
        mImages = OpenXRSwapchainImage::enumerateSwapchainImages(state->getGraphicsContext(), mSwapchain, swapchainCreateInfo);
        mSubImage.swapchain = mSwapchain;
        mSubImage.imageRect.offset = { 0, 0 };
        mSubImage.imageRect.extent = { mWidth, mHeight };
    }

    OpenXRSwapchainImpl::SwapchainPrivate::~SwapchainPrivate()
    {
        if (mSwapchain)
            CHECK_XRCMD(xrDestroySwapchain(mSwapchain));
    }

    uint32_t OpenXRSwapchainImpl::SwapchainPrivate::count() const
    {
        return mImages.size();
    }

    bool OpenXRSwapchainImpl::SwapchainPrivate::isAcquired() const
    {
        return mIsReady;
    }

    void OpenXRSwapchainImpl::SwapchainPrivate::acquire(osg::GraphicsContext* gc)
    {
        auto xr = Environment::get().getManager();
        XrSwapchainImageAcquireInfo acquireInfo{ XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
        XrSwapchainImageWaitInfo waitInfo{ XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
        waitInfo.timeout = XR_INFINITE_DURATION;
        if (!mIsIndexAcquired)
        {
            mIsIndexAcquired = XR_SUCCEEDED(CHECK_XRCMD(xrAcquireSwapchainImage(mSwapchain, &acquireInfo, &mAcquiredIndex)));
            if (mIsIndexAcquired)
                xr->xrResourceAcquired();
        }
        if (mIsIndexAcquired && !mIsReady)
        {
            mIsReady = XR_SUCCEEDED(CHECK_XRCMD(xrWaitSwapchainImage(mSwapchain, &waitInfo)));
        }
    }
    void OpenXRSwapchainImpl::SwapchainPrivate::blitAndRelease(osg::GraphicsContext* gc, VRFramebuffer& readBuffer)
    {
        auto xr = Environment::get().getManager();

        XrSwapchainImageReleaseInfo releaseInfo{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
        if (mIsReady)
        {
            mImages[mAcquiredIndex]->blit(gc, readBuffer, mConfig.offsetWidth, mConfig.offsetHeight);

            mIsReady = !XR_SUCCEEDED(CHECK_XRCMD(xrReleaseSwapchainImage(mSwapchain, &releaseInfo)));
            if (!mIsReady)
            {
                mIsIndexAcquired = false;
                xr->xrResourceReleased();
            }
        }
    }

    void OpenXRSwapchainImpl::SwapchainPrivate::checkAcquired() const
    {
        if (!isAcquired())
            throw std::logic_error("Swapchain must be acquired before use. Call between OpenXRSwapchain::beginFrame() and OpenXRSwapchain::endFrame()");
    }

    XrSwapchain OpenXRSwapchainImpl::xrSwapchain(void) const 
    { 
        if(mSwapchain)
            return mSwapchain->xrSwapchain(); 
        return XR_NULL_HANDLE;
    }

    XrSwapchain OpenXRSwapchainImpl::xrSwapchainDepth(void) const
    {
        if (mSwapchainDepth)
            return mSwapchainDepth->xrSwapchain();
        return XR_NULL_HANDLE;
    }

    XrSwapchainSubImage OpenXRSwapchainImpl::xrSubImage(void) const
    {
        if (mSwapchain)
            return mSwapchain->xrSubImage();
        return XrSwapchainSubImage{ XR_NULL_HANDLE };
    }

    XrSwapchainSubImage OpenXRSwapchainImpl::xrSubImageDepth(void) const
    {
        if (mSwapchainDepth)
            return mSwapchainDepth->xrSubImage();
        return XrSwapchainSubImage{ XR_NULL_HANDLE };
    }
}
