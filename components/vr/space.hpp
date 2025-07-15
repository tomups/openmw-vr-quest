#pragma once

#include "vr.hpp"
#include "session.hpp"

namespace VR
{
    class Space
    {
    public:
        Space();
        virtual ~Space() {}

        Space(const Space&) = delete;
        void operator=(const Space&) = delete;

        virtual TrackingPose locate(Space& reference) = 0;
        TrackingPose locate(ReferenceSpace reference);
        virtual TrackingPose locateInWorld() = 0;
    };

    class DerivedSpace : public Space
    {
    public:
        DerivedSpace(std::shared_ptr<Space> reference, Stereo::Pose pose);
        ~DerivedSpace() override {}

        DerivedSpace(const DerivedSpace&) = delete;
        void operator=(const DerivedSpace&) = delete;

        TrackingPose locate(Space& reference) override;
        TrackingPose locateInWorld() override;

        void setPose(Stereo::Pose pose) { mPose = pose; }
        void setReference(std::shared_ptr<Space> reference) { mReference = reference; }

    private:
        std::shared_ptr<Space> mReference;
        Stereo::Pose mPose;
    };

    //! A simple transform that places transforms to an XrSpace's location in the world
    class SpaceTransform : public osg::Transform, private Session::Listener
    {
    public:
        SpaceTransform();
        SpaceTransform(std::shared_ptr<Space> space);
        SpaceTransform(const SpaceTransform& transform, const osg::CopyOp& op = osg::CopyOp::SHALLOW_COPY);

        META_Node(VR, SpaceTransform)

        void setSpace(std::shared_ptr<VR::Space> space);

        bool computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor*) const override;
        bool computeWorldToLocalMatrix(osg::Matrix& matrix, osg::NodeVisitor*) const override;
        void onSpaceUpdate() override;

    private:
        std::shared_ptr<Space> mSpace;
        //mutable unsigned int mLastUpdate = -1;
        mutable osg::Matrix mMatrix = osg::Matrix::identity();
    };
}
