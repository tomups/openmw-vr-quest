#ifndef MWVR_OPENXRTYPECONVERSIONS_H
#define MWVR_OPENXRTYPECONVERSIONS_H

#include <openxr/openxr.h>
#include "vrtypes.hpp"
#include <osg/Vec3>
#include <osg/Quat>

namespace MWVR
{
    /// Conversion methods between openxr types to osg/mwvr types. Includes managing the differing conventions.
    Pose          fromXR(XrPosef pose);
    FieldOfView   fromXR(XrFovf fov);
    osg::Vec3           fromXR(XrVector3f);
    osg::Quat           fromXR(XrQuaternionf quat);
    XrPosef             toXR(Pose pose);
    XrFovf              toXR(FieldOfView fov);
    XrVector3f          toXR(osg::Vec3 v);
    XrQuaternionf       toXR(osg::Quat quat);

    XrCompositionLayerProjectionView toXR(CompositionLayerProjectionView layer);
    XrSwapchainSubImage toXR(SubImage, bool depthImage);
}

#endif
