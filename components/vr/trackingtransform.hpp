#ifndef VR_TRACKING_TRANFORM_H
#define VR_TRACKING_TRANFORM_H

#include "trackinglistener.hpp"
#include <osg/MatrixTransform>

namespace VR
{
    //! Acts as a osg::MatrixTransform using a matrix build from tracking data from the provided path
    class TrackingTransform : public osg::MatrixTransform, public TrackingListener
    {
    public:
        TrackingTransform(VRPath path);

        virtual void onTrackingUpdated(TrackingManager& manager);

        VRPath path() const { return mPath; }

    protected:
        VRPath mPath;
    };
}

#endif
