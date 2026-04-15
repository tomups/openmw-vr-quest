#include "swapchain.hpp"
#include "debug.hpp"
#include "extensions.hpp"
#include "session.hpp"
#include "instance.hpp"

#include <components/debug/debuglog.hpp>

// The OpenXR SDK's platform headers assume we've included platform headers
#ifdef _WIN32
#ifdef NOGDI
#undef NOGDI
#endif
#include <Windows.h>
#include <objbase.h>

#ifdef XR_USE_GRAPHICS_API_D3D11
#include <d3d11_1.h>
#endif

#elif __linux__
#include <EGL/egl.h>
#include <GL/glx.h>
#undef None

#else
#error Unsupported platform
#endif
#include <openxr/openxr_platform.h>
#include <openxr/openxr_platform_defines.h>

namespace XR
{

    Swapchain::~Swapchain()
    {
        if (mXrSwapchain)
            xrDestroySwapchain(mXrSwapchain);
    }

    void Swapchain::beginFrame(osg::GraphicsContext* gc)
    {
        if (mImages.empty())
        {
            init();
        }

        if (!mIsAcquired)
        {
            acquire();
        }
        if (mIsAcquired && !mIsReady)
        {
            wait();
        }

        if (mIsReady)
        {
            image()->beginFrame(gc);
        }
    }

    void Swapchain::endFrame(osg::GraphicsContext* gc)
    {
        if (mIsReady)
        {
            image()->endFrame(gc);
            release();
        }

        mAcquiredIndex = mImages.size();
    }

    VR::SwapchainImage* Swapchain::image()
    {
        if (mAcquiredIndex < mImages.size())
            return mImages[mAcquiredIndex].get();
        return nullptr;
    }

    bool Swapchain::mustFlipVertical() const
    {
#ifdef XR_USE_GRAPHICS_API_D3D11
        return XR::Extensions::instance().extensionEnabled(XR_KHR_D3D11_ENABLE_EXTENSION_NAME);
#else
        return false;
#endif
    }

    void Swapchain::init()
    {
        std::string typeString = mAttachment == Attachment::Color ? "color" : "depth";

        XrSwapchainCreateInfo swapchainCreateInfo{};
        swapchainCreateInfo.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
        swapchainCreateInfo.arraySize = mArraySize;
        swapchainCreateInfo.width = mWidth;
        swapchainCreateInfo.height = mHeight;
        swapchainCreateInfo.mipCount = 1;
        swapchainCreateInfo.faceCount = 1;
        swapchainCreateInfo.format = 0;
        swapchainCreateInfo.usageFlags = 0;
        if (mAttachment == Attachment::Color)
            swapchainCreateInfo.usageFlags |= XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        else
            swapchainCreateInfo.usageFlags |= XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        auto [swapchainFormat, glFormat] = Instance::instance().platform().selectSwapchainFormat(mAttachment);
        swapchainCreateInfo.format = swapchainFormat;
        mFormat = glFormat;

        while (mSamples > 0 && mXrSwapchain == XR_NULL_HANDLE)
        {
            // Now create the swapchain
            Log(Debug::Verbose) << "Creating swapchain with dimensions Width=" << mWidth << " Heigh=" << mHeight
                                << " SampleCount=" << mSamples;
            swapchainCreateInfo.sampleCount = mSamples;
            auto res = xrCreateSwapchain(Session::instance().xrSession(), &swapchainCreateInfo, &mXrSwapchain);

            // Check errors and try again if possible
            if (res == XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED)
            {
                // User specified a swapchian format not supported by the runtime.
                throw std::runtime_error(
                    std::string("Swapchain ") + typeString + " format not supported by openxr runtime.");
            }
            else if (res == XR_ERROR_FEATURE_UNSUPPORTED)
            {
                // Swapchain using some unsupported feature.
                // This means either array textures or depth attachment was used but not supported.
                // Application will have to do without said feature.
                Log(Debug::Warning) << "xrCreateSwapchain returned XR_ERROR_FEATURE_UNSUPPORTED.";
                return;
            }
            else if (!XR_SUCCEEDED(res))
            {
                // If XR runtime doesn't support the number of samples, there is no return code for this, so we try
                // again until we've tried all possibilities down to 1.
                Log(Debug::Verbose) << "Failed to create swapchain with SampleCount=" << mSamples << ": "
                                    << XrResultString(res);
                mSamples /= 2;
                if (mSamples == 0)
                {
                    // Use CHECK_XRRESULT to pass the failure through the same error checking / logging mechanism as
                    // other calls.
                    CHECK_XRRESULT(res, "xrCreateSwapchain");
                    throw std::runtime_error(XrResultString(res));
                }
                continue;
            }

            // Pass the return code through CHECK_XRRESULT in case the user is logging all calls.
            CHECK_XRRESULT(res, "xrCreateSwapchain");

            if (mAttachment == Attachment::Color)
                Debugging::setName(mXrSwapchain, "OpenMW XR Color Swapchain " + mName);
            else
                Debugging::setName(mXrSwapchain, "OpenMW XR Depth Swapchain " + mName);

            mImages = Instance::instance().platform().enumerateSwapchainImages(
                mXrSwapchain, mTextureTarget, swapchainCreateInfo.format);
        }
    }

    void Swapchain::acquire()
    {
        XrSwapchainImageAcquireInfo acquireInfo{};
        acquireInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO;
        mIsAcquired = XR_SUCCEEDED(CHECK_XRCMD(xrAcquireSwapchainImage(mXrSwapchain, &acquireInfo, &mAcquiredIndex)));
    }

    void Swapchain::wait()
    {
        XrSwapchainImageWaitInfo waitInfo{};
        waitInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;
        waitInfo.timeout = XR_INFINITE_DURATION;
        mIsReady = XR_SUCCEEDED(CHECK_XRCMD(xrWaitSwapchainImage(mXrSwapchain, &waitInfo)));
    }

    void Swapchain::release()
    {
        XrSwapchainImageReleaseInfo releaseInfo{};
        releaseInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO;

        mIsReady = !XR_SUCCEEDED(CHECK_XRCMD(xrReleaseSwapchainImage(mXrSwapchain, &releaseInfo)));
        if (!mIsReady)
        {
            mIsAcquired = false;
        }
    }

    bool Swapchain::isAcquired()
    {
        return mIsAcquired;
    }
}
