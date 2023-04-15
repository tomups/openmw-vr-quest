#ifndef MWVR_POINTER_H
#define MWVR_POINTER_H

#include "../mwrender/npcanimation.hpp"
#include "../mwrender/renderingmanager.hpp"

#include <components/vr/trackinglistener.hpp>

#include <osg/Node>
#include <osg/ref_ptr>

#include <memory>

namespace SceneUtil
{
    class PositionAttitudeTransform;
}

namespace osg
{
    class Geometry;
    class Group;
    class Transform;
    class MatrixTransform;
}

namespace MWVR
{
    class Crosshair
    {
    public:
        Crosshair(osg::ref_ptr<osg::Group> parent, osg::Vec3f color, float farAlpha, float nearAlpha, bool pyramid);
        ~Crosshair();

        void hide();
        void show();
        void setStretch(float stretch);
        void setWidth(float width);
        void setOffset(float distance);
        void setParent(osg::ref_ptr<osg::Group> parent);
        void updateMatrix();

        osg::ref_ptr<osg::Group> mParent;
        osg::ref_ptr<osg::Geometry> mGeometry;
        osg::ref_ptr<osg::PositionAttitudeTransform> mTransform;
        float mStretch;
        float mWidth;
        float mOffset;
        bool mVisible;
    };

    //! Controls the beam used to target/select objects.
    class UserPointer : public VR::TrackingListener
    {

    public:
        UserPointer(osg::Group* root);
        ~UserPointer();

        // void updatePointerTarget();
        const MWRender::RayResult& getPointerRay() const;
        bool canPlaceObject() const;
        void setSource(VR::VRPath source);
        // bool enabled() const { return !!mSource; };
        float distanceToPointerTarget() const { return mDistanceToPointerTarget; }
        void activate();

    private:
        void onTrackingUpdated(VR::TrackingManager& manager) override;

        bool tryProbePick(MWWorld::Ptr target);

        // osg::ref_ptr<SceneUtil::PositionAttitudeTransform> mPointerSource;
        osg::ref_ptr<SceneUtil::PositionAttitudeTransform> mPointerTransform;

        VR::VRPath mSourcePath = 0;
        osg::ref_ptr<osg::Group> mRoot;

        MWRender::RayResult mPointerRay = {};
        osg::ref_ptr<osg::Node> mPointerTarget;
        float mDistanceToPointerTarget = -1.f;
        bool mCanPlaceObject = false;
        bool mLeftHandEnabled = true;
        bool mRightHandEnabled = true;
        std::unique_ptr<Crosshair> mCrosshair;
    };
}

#endif
