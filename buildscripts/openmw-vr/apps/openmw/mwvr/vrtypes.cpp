#include "vrtypes.hpp"

#include <iostream>

namespace MWVR
{
    //Pose Pose::operator+(const Pose& rhs)
    //{
    //    Pose pose = *this;
    //    pose.position += this->orientation * rhs.position;
    //    pose.orientation = rhs.orientation * this->orientation;
    //    return pose;
    //}

    //const Pose& Pose::operator+=(const Pose& rhs)
    //{
    //    *this = *this + rhs;
    //    return *this;
    //}

    //Pose Pose::operator*(float scalar)
    //{
    //    Pose pose = *this;
    //    pose.position *= scalar;
    //    return pose;
    //}

    //const Pose& Pose::operator*=(float scalar)
    //{
    //    *this = *this * scalar;
    //    return *this;
    //}

    //Pose Pose::operator/(float scalar)
    //{
    //    Pose pose = *this;
    //    pose.position /= scalar;
    //    return pose;
    //}
    //const Pose& Pose::operator/=(float scalar)
    //{
    //    *this = *this / scalar;
    //    return *this;
    //}

    //bool Pose::operator==(const Pose& rhs) const
    //{
    //    return position == rhs.position && orientation == rhs.orientation;
    //}

    //bool FieldOfView::operator==(const FieldOfView& rhs) const
    //{
    //    return angleDown == rhs.angleDown
    //        && angleUp == rhs.angleUp
    //        && angleLeft == rhs.angleLeft
    //        && angleRight == rhs.angleRight;
    //}

    //// near and far named with an underscore because of windows' headers galaxy brain defines.
    //osg::Matrix FieldOfView::perspectiveMatrix(float near_, float far_)
    //{
    //    const float tanLeft = tanf(angleLeft);
    //    const float tanRight = tanf(angleRight);
    //    const float tanDown = tanf(angleDown);
    //    const float tanUp = tanf(angleUp);

    //    const float tanWidth = tanRight - tanLeft;
    //    const float tanHeight = tanUp - tanDown;

    //    const float offset = near_;

    //    float matrix[16] = {};

    //    matrix[0] = 2 / tanWidth;
    //    matrix[4] = 0;
    //    matrix[8] = (tanRight + tanLeft) / tanWidth;
    //    matrix[12] = 0;

    //    matrix[1] = 0;
    //    matrix[5] = 2 / tanHeight;
    //    matrix[9] = (tanUp + tanDown) / tanHeight;
    //    matrix[13] = 0;

    //    if (far_ <= near_) {
    //        matrix[2] = 0;
    //        matrix[6] = 0;
    //        matrix[10] = -1;
    //        matrix[14] = -(near_ + offset);
    //    }
    //    else {
    //        matrix[2] = 0;
    //        matrix[6] = 0;
    //        matrix[10] = -(far_ + offset) / (far_ - near_);
    //        matrix[14] = -(far_ * (near_ + offset)) / (far_ - near_);
    //    }

    //    matrix[3] = 0;
    //    matrix[7] = 0;
    //    matrix[11] = -1;
    //    matrix[15] = 0;

    //    return osg::Matrix(matrix);
    //}
    bool PoseSet::operator==(const PoseSet& rhs) const
    {
        return  eye[0] == rhs.eye[0]
            && eye[1] == rhs.eye[1]
            && hands[0] == rhs.hands[0]
            && hands[1] == rhs.hands[1]
            && view[0] == rhs.view[0]
            && view[1] == rhs.view[1]
            && head == rhs.head;

    }
    //bool View::operator==(const View& rhs) const
    //{
    //    return pose == rhs.pose && fov == rhs.fov;
    //}

    std::ostream& operator <<(
        std::ostream& os,
        const MWVR::Pose& pose)
    {
        os << "position=" << pose.position << ", orientation=" << pose.orientation;
        return os;
    }

    std::ostream& operator <<(
        std::ostream& os,
        const MWVR::FieldOfView& fov)
    {
        os << "left=" << fov.angleLeft << ", right=" << fov.angleRight << ", down=" << fov.angleDown << ", up=" << fov.angleUp;
        return os;
    }

    std::ostream& operator <<(
        std::ostream& os,
        const MWVR::View& view)
    {
        os << "pose=< " << view.pose << " >, fov=< " << view.fov << " >";
        return os;
    }

    std::ostream& operator <<(
        std::ostream& os,
        const MWVR::PoseSet& poseSet)
    {
        os << "eye[" << Side::LEFT_SIDE << "]: " << poseSet.eye[(int)Side::LEFT_SIDE] << std::endl;
        os << "eye[" << Side::RIGHT_SIDE << "]: " << poseSet.eye[(int)Side::RIGHT_SIDE] << std::endl;
        os << "hands[" << Side::LEFT_SIDE << "]: " << poseSet.hands[(int)Side::LEFT_SIDE] << std::endl;
        os << "hands[" << Side::RIGHT_SIDE << "]: " << poseSet.hands[(int)Side::RIGHT_SIDE] << std::endl;
        os << "head: " << poseSet.head << std::endl;
        os << "view[" << Side::LEFT_SIDE << "]: " << poseSet.view[(int)Side::LEFT_SIDE] << std::endl;
        os << "view[" << Side::RIGHT_SIDE << "]: " << poseSet.view[(int)Side::RIGHT_SIDE] << std::endl;
        return os;
    }

    std::ostream& operator <<(
        std::ostream& os,
        TrackedLimb limb)
    {
        switch (limb)
        {
        case TrackedLimb::HEAD:
            os << "HEAD"; break;
        case TrackedLimb::LEFT_HAND:
            os << "LEFT_HAND"; break;
        case TrackedLimb::RIGHT_HAND:
            os << "RIGHT_HAND"; break;
        }
        return os;
    }

    std::ostream& operator <<(
        std::ostream& os,
        ReferenceSpace limb)
    {
        switch (limb)
        {
        case ReferenceSpace::STAGE:
            os << "STAGE"; break;
        case ReferenceSpace::VIEW:
            os << "VIEW"; break;
        }
        return os;
    }

    std::ostream& operator <<(
        std::ostream& os,
        Side side)
    {
        switch (side)
        {
        case Side::LEFT_SIDE:
            os << "LEFT_SIDE"; break;
        case Side::RIGHT_SIDE:
            os << "RIGHT_SIDE"; break;
        }
        return os;
    }

}


