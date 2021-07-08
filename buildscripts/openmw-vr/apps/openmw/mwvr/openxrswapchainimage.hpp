#ifndef OPENXR_SWAPCHAINIMAGE_HPP
#define OPENXR_SWAPCHAINIMAGE_HPP

#include <vector>
#include <memory>

#include <openxr/openxr.h>
#include <osg/GraphicsContext>

#include "vrframebuffer.hpp"

namespace MWVR
{
    class OpenXRSwapchainImage
    {
    public:
        static std::vector< std::unique_ptr<OpenXRSwapchainImage> >
            enumerateSwapchainImages(osg::GraphicsContext* gc, XrSwapchain swapchain, XrSwapchainCreateInfo swapchainCreateInfo);

        OpenXRSwapchainImage();
        virtual ~OpenXRSwapchainImage() {};

        virtual void blit(osg::GraphicsContext* gc, VRFramebuffer& readBuffer, int offset_x, int offset_y) = 0;
    };
}

#endif
