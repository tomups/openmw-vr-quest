#pragma once

#include "vr.hpp"
#include "session.hpp"

namespace VR
{
    class Space
    {
    public:
        Space() {}
        virtual ~Space() {}

        Space(const Space&) = delete;
        void operator=(const Space&) = delete;

        virtual TrackingPose locate(const std::shared_ptr<Space>& reference) const = 0;
        virtual TrackingPose locateInWorld() const = 0;
    };

    //! A simple transform that places transforms to an XrSpace's location in the world
    class SpaceTransform : public osg::Transform, private Session::Listener
    {
    public:
        SpaceTransform();
        SpaceTransform(std::shared_ptr<Space> space);
        SpaceTransform(const SpaceTransform& transform, const osg::CopyOp& op = osg::CopyOp::SHALLOW_COPY);

        META_Node(VR, SpaceTransform);

        void setSpace(std::shared_ptr<VR::Space> space);

        bool computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor*) const;
        bool computeWorldToLocalMatrix(osg::Matrix& matrix, osg::NodeVisitor*) const;
        void onFrameUpdate(VR::Frame&) override;

    private:
        std::shared_ptr<Space> mSpace;
        mutable unsigned int mLastUpdate = -1;
        mutable osg::Matrix mMatrix = osg::Matrix::identity();
    };
}
