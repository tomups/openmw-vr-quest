#ifndef MWVR_POINTER_H
#define MWVR_POINTER_H

#include "../mwrender/npcanimation.hpp"
#include "../mwrender/renderingmanager.hpp"
#include "openxrmanager.hpp"
#include "vrsession.hpp"
#include "vrtracking.hpp"

namespace MWVR
{
    //! Controls the beam used to target/select objects.
    class UserPointer : public VRTrackingListener
    {
    public:
        UserPointer(osg::Group* root);
        ~UserPointer();

        void updatePointerTarget();
        const MWRender::RayResult& getPointerTarget() const;
        bool canPlaceObject() const;
        void setParent(osg::Group* group);
        void setEnabled(bool enabled);
    protected:
        void onTrackingUpdated(VRTrackingSource& source, DisplayTime predictedDisplayTime) override;

    private:
        osg::ref_ptr<osg::Geometry> createPointerGeometry();

        osg::ref_ptr<osg::Geometry> mPointerGeometry{ nullptr };
        osg::ref_ptr<osg::MatrixTransform> mPointerRescale{ nullptr };
        osg::ref_ptr<osg::MatrixTransform> mPointerTransform{ nullptr };

        osg::ref_ptr<osg::Group> mParent{ nullptr };
        osg::ref_ptr<osg::Group> mRoot{ nullptr };
        VRPath mHandPath;

        bool mEnabled;
        MWRender::RayResult mPointerTarget{};
        float mDistanceToPointerTarget{ -1.f };
        bool mCanPlaceObject{ false };
    };
}

#endif
