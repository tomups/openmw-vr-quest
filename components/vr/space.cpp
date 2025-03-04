#include "space.hpp"
#include <components/sceneutil/nodecallback.hpp>

namespace VR
{
    SpaceTransform::SpaceTransform()
    {
    }
    SpaceTransform::SpaceTransform(std::shared_ptr<Space> space)
        : mSpace(space)
    {
    }

    SpaceTransform::SpaceTransform(const SpaceTransform& transform, const osg::CopyOp&)
        : mSpace(transform.mSpace)
    {
    }

    void SpaceTransform::setSpace(std::shared_ptr<VR::Space> space)
    {
        mSpace = space;
    }

    bool SpaceTransform::computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const
    {
        if (_referenceFrame == RELATIVE_RF)
        {
             matrix.preMult(mMatrix);
        }
        else // absolute
        {
            matrix = mMatrix;
        }
        return true;
    }

    bool SpaceTransform::computeWorldToLocalMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const
    {
        const osg::Matrix& inverse = osg::Matrix::inverse(mMatrix);

        if (_referenceFrame == RELATIVE_RF)
        {
            matrix.postMult(inverse);
        }
        else // absolute
        {
            matrix = inverse;
        }
        return true;
    }

    void SpaceTransform::onSpaceUpdate()
    {
        VR::TrackingPose tp = mSpace->locateInWorld();

        mMatrix.makeIdentity();
        if (!!tp.status)
        {
            mMatrix.makeRotate(tp.pose.orientation);
            mMatrix.postMultTranslate(tp.pose.position.asMWUnits());
        }
    }

    Space::Space() {}

    DerivedSpace::DerivedSpace(std::shared_ptr<Space> reference, Stereo::Pose pose)
        : mReference(reference)
        , mPose(pose)
    {
    }

    TrackingPose VR::DerivedSpace::locate(const std::shared_ptr<Space>& reference) const
    {
        auto tp = mReference->locate(reference);
        if (!!tp.status)
            tp.pose += mPose;
        return tp;
    }
    TrackingPose DerivedSpace::locateInWorld() const
    {
        auto tp = mReference->locateInWorld();
        if (!!tp.status)
            tp.pose += mPose;
        return tp;
    }
}
