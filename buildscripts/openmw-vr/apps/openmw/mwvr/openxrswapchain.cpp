#include "openxrswapchain.hpp"
#include "openxrswapchainimpl.hpp"
#include "openxrmanager.hpp"
#include "openxrmanagerimpl.hpp"
#include "vrenvironment.hpp"

#include <components/debug/debuglog.hpp>

namespace MWVR {
    OpenXRSwapchain::OpenXRSwapchain(osg::ref_ptr<osg::State> state, SwapchainConfig config)
        : mPrivate(new OpenXRSwapchainImpl(state, config))
    {
    }

    OpenXRSwapchain::~OpenXRSwapchain()
    {
    }

    void OpenXRSwapchain::beginFrame(osg::GraphicsContext* gc)
    {
        return impl().beginFrame(gc);
    }

    void OpenXRSwapchain::endFrame(osg::GraphicsContext* gc, VRFramebuffer& readBuffer)
    {
        return impl().endFrame(gc, readBuffer);
    }

    int OpenXRSwapchain::width() const
    {
        return impl().width();
    }

    int OpenXRSwapchain::height() const
    {
        return impl().height();
    }

    int OpenXRSwapchain::samples() const
    {
        return impl().samples();
    }

    bool OpenXRSwapchain::isAcquired() const
    {
        return impl().isAcquired();
    }
}
