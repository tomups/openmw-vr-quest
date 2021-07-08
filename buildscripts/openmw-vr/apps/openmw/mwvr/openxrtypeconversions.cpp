#include "openxrtypeconversions.hpp"
#include "openxrswapchain.hpp"
#include "openxrswapchainimpl.hpp"
#include <iostream>

namespace MWVR
{
    osg::Vec3 fromXR(XrVector3f v)
    {
        return osg::Vec3{ v.x, -v.z, v.y };
    }

    osg::Quat fromXR(XrQuaternionf quat)
    {
        return osg::Quat{ quat.x, -quat.z, quat.y, quat.w };
    }

    XrVector3f toXR(osg::Vec3 v)
    {
        return XrVector3f{ v.x(), v.z(), -v.y() };
    }

    XrQuaternionf toXR(osg::Quat quat)
    {
        return XrQuaternionf{ static_cast<float>(quat.x()), static_cast<float>(quat.z()), static_cast<float>(-quat.y()), static_cast<float>(quat.w()) };
    }

    MWVR::Pose fromXR(XrPosef pose)
    {
        return MWVR::Pose{ fromXR(pose.position), fromXR(pose.orientation) };
    }

    XrPosef toXR(MWVR::Pose pose)
    {
        return XrPosef{ toXR(pose.orientation), toXR(pose.position) };
    }

    MWVR::FieldOfView fromXR(XrFovf fov)
    {
        return MWVR::FieldOfView{ fov.angleLeft, fov.angleRight, fov.angleUp, fov.angleDown };
    }

    XrFovf toXR(MWVR::FieldOfView fov)
    {
        return XrFovf{ fov.angleLeft, fov.angleRight, fov.angleUp, fov.angleDown };
    }

    XrCompositionLayerProjectionView toXR(MWVR::CompositionLayerProjectionView layer)
    {
        XrCompositionLayerProjectionView xrLayer;
        xrLayer.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        xrLayer.subImage = toXR(layer.subImage, false);
        xrLayer.pose = toXR(layer.pose);
        xrLayer.fov = toXR(layer.fov);
        xrLayer.next = nullptr;

        return xrLayer;
    }

    XrSwapchainSubImage toXR(MWVR::SubImage subImage, bool depthImage)
    {
        XrSwapchainSubImage xrSubImage{};
        if (depthImage)
            xrSubImage.swapchain = subImage.swapchain->impl().xrSwapchainDepth();
        else
            xrSubImage.swapchain = subImage.swapchain->impl().xrSwapchain();
        xrSubImage.imageRect.extent.width = subImage.width;
        xrSubImage.imageRect.extent.height = subImage.height;
        xrSubImage.imageRect.offset.x = subImage.x;
        xrSubImage.imageRect.offset.y = subImage.y;
        xrSubImage.imageArrayIndex = 0;
        return xrSubImage;
    }
}


