#ifndef OPENXR_SWAPCHAIN_HPP
#define OPENXR_SWAPCHAIN_HPP

#include "openxrmanager.hpp"

struct XrSwapchainSubImage;

namespace MWVR
{
    class OpenXRSwapchainImpl;
    class VRFramebuffer;

    /// \brief Creation and management of openxr swapchains
    class OpenXRSwapchain
    {
    public:
        OpenXRSwapchain(osg::ref_ptr<osg::State> state, SwapchainConfig config);
        ~OpenXRSwapchain();

    public:
        //! Prepare for render (set FBO)
        void beginFrame(osg::GraphicsContext* gc);

        //! Finalize render
        void endFrame(osg::GraphicsContext* gc, VRFramebuffer& readBuffer);

        //! Whether subchain is currently acquired (true) or released (false)
        bool isAcquired() const;

        //! Width of the view surface
        int width() const;

        //! Height of the view surface
        int height() const;

        //! Samples of the view surface
        int samples() const;

        //! Get the private implementation
        OpenXRSwapchainImpl& impl() { return *mPrivate; }

        //! Get the private implementation
        const OpenXRSwapchainImpl& impl() const { return *mPrivate; }

    protected:
        OpenXRSwapchain(const OpenXRSwapchain&) = delete;
        void operator=(const OpenXRSwapchain&) = delete;
    private:
        std::unique_ptr<OpenXRSwapchainImpl> mPrivate;
    };
}

#endif
