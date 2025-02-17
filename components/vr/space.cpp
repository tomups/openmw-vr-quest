#include "space.hpp"
#include <components/sceneutil/nodecallback.hpp>

namespace VR
{
    SpaceTransform::SpaceTransform()
    {
        setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    }
    SpaceTransform::SpaceTransform(std::shared_ptr<Space> space)
        : mSpace(space)
    {
        setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    }

    SpaceTransform::SpaceTransform(const SpaceTransform& transform, const osg::CopyOp&)
        : mSpace(transform.mSpace)
    {
        setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    }

    void SpaceTransform::setSpace(std::shared_ptr<VR::Space> space) {
        mSpace = space;
    }

    bool SpaceTransform::computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const
    {
        if (_referenceFrame == RELATIVE_RF)
        {
            // TODO: This use case doesn't make sense
            //matrix.preMult(mMatrix);
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
            // TODO: This use case doesn't make sense
            //matrix.postMult(inverse);
        }
        else // absolute
        {
            matrix = inverse;
        }
        return true;
    }

    void SpaceTransform::onFrameUpdate(VR::Frame&)
    {
        VR::TrackingPose tp = {};
        if (_referenceFrame == RELATIVE_RF)
        {
            // TODO: This use case doesn't make sense
            //tp = mSpace->locate();
        }
        else // absolute
        {
            if (mSpace)
                tp = mSpace->locateInWorld();
        }

        if (!tp.status)
        {
            mMatrix.makeIdentity();
        }
        else
        {
            mMatrix.preMultTranslate(tp.pose.position.asMWUnits());
            mMatrix.preMultRotate(tp.pose.orientation);
        }
    }

}
