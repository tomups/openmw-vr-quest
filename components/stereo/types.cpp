#include "types.hpp"

#include <osg/io_utils>

namespace Stereo
{

    Pose Pose::operator+(const Pose& rhs) const
    {
        Pose pose = *this;
        pose.position += this->orientation * rhs.position;
        pose.orientation = rhs.orientation * this->orientation;
        return pose;
    }

    const Pose& Pose::operator+=(const Pose& rhs)
    {
        *this = *this + rhs;
        return *this;
    }

    Pose Pose::operator*(float scalar)
    {
        Pose pose = *this;
        pose.position *= scalar;
        return pose;
    }

    const Pose& Pose::operator*=(float scalar)
    {
        *this = *this * scalar;
        return *this;
    }

    Pose Pose::operator/(float scalar)
    {
        Pose pose = *this;
        pose.position /= scalar;
        return pose;
    }
    const Pose& Pose::operator/=(float scalar)
    {
        *this = *this / scalar;
        return *this;
    }

    bool Pose::operator==(const Pose& rhs) const
    {
        return position == rhs.position && orientation == rhs.orientation;
    }

    osg::Matrix View::viewMatrix(bool useGLConventions)
    {
//## VR_PATCH BEGIN
        auto position = pose.position.asMWUnits();
//## VR_PATCH END
        auto orientation = pose.orientation;

        if (useGLConventions)
        {
            // When applied as an offset to an existing view matrix,
            // that view matrix will already convert points to a camera space
            // with opengl conventions. So we need to convert offsets to opengl
            // conventions.
            {
                float y = position.y();
                float z = position.z();
                position.y() = z;
                position.z() = -y;
            }
            {
                double y = orientation.y();
                double z = orientation.z();
                orientation.y() = z;
                orientation.z() = -y;
            }

            osg::Matrix viewMatrix;
            viewMatrix.setTrans(-position);
            viewMatrix.postMultRotate(orientation.conj());
            return viewMatrix;
        }
        else
        {
            osg::Vec3d forward = orientation * osg::Vec3d(0, 1, 0);
            osg::Vec3d up = orientation * osg::Vec3d(0, 0, 1);
            osg::Matrix viewMatrix;
            viewMatrix.makeLookAt(position, position + forward, up);

            return viewMatrix;
        }
    }

    osg::Matrix View::perspectiveMatrix(float near, float far, bool reverseZ)
    {
        const float tanLeft = tanf(fov.angleLeft);
        const float tanRight = tanf(fov.angleRight);
        const float tanDown = tanf(fov.angleDown);
        const float tanUp = tanf(fov.angleUp);

        float depth = far - near;
        float matrix[16] = {};

        float left = near * tanLeft;
        float right = near * tanRight;
        float bottom = near * tanDown;
        float top = near * tanUp;
        float width = right - left;
        float height = top - bottom;

        matrix[0] = 2 * near / width;
        matrix[8] = (right + left) / width;
        matrix[5] = 2 * near / height;
        matrix[9] = (top + bottom) / height;
        if (reverseZ)
        {
            matrix[10] = near / depth;
            matrix[14] = near * far / depth;
        }
        else
        {
            matrix[10] = -(far + near) / depth;
            matrix[14] = -(far * (2.f * near)) / depth;
        }
        matrix[11] = -1;

        return osg::Matrix(matrix);
    }

    bool FieldOfView::operator==(const FieldOfView& rhs) const
    {
        return angleDown == rhs.angleDown && angleUp == rhs.angleUp && angleLeft == rhs.angleLeft
            && angleRight == rhs.angleRight;
    }

    bool View::operator==(const View& rhs) const
    {
        return pose == rhs.pose && fov == rhs.fov;
    }

    std::ostream& operator<<(std::ostream& os, const Pose& pose)
    {
//## VR_PATCH BEGIN
        os << "position={ Meters=" << pose.position.asMeters() << ", MWUnits=" << pose.position.asMWUnits()
           << " }, orientation=" << pose.orientation;
//## VR_PATCH END
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const FieldOfView& fov)
    {
        os << "left=" << fov.angleLeft << ", right=" << fov.angleRight << ", down=" << fov.angleDown
           << ", up=" << fov.angleUp;
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const View& view)
    {
        os << "pose=< " << view.pose << " >, fov=< " << view.fov << " >";
        return os;
    }
//## VR_PATCH BEGIN
    // OSG doesn't provide API to extract euler angles from a quat, but i need it.
    // Credits goes to Dennis Bunfield, i just copied his formula https://narkive.com/v0re6547.4
    void getEulerAngles(const osg::Quat& quat, float& yaw, float& pitch, float& roll)
    {
        // Now do the computation
        osg::Matrixd m2(osg::Matrixd::rotate(quat));
        double* mat = (double*)m2.ptr();
        double angleX = 0.0;
        double angleY = 0.0;
        double angleZ = 0.0;
        double c, trX, trY;
        angleY = asin(mat[2]); /* Calculate Y-axis angle */
        c = cos(angleY);
        if (fabs(c) > 0.005) /* Test for Gimball lock? */
        {
            trX = mat[10] / c; /* No, so get X-axis angle */
            trY = -mat[6] / c;
            angleX = atan2(trY, trX);
            trX = mat[0] / c; /* Get Z-axis angle */
            trY = -mat[1] / c;
            angleZ = atan2(trY, trX);
        }
        else /* Gimball lock has occurred */
        {
            angleX = 0; /* Set X-axis angle to zero
                         */
            trX = mat[5]; /* And calculate Z-axis angle
                           */
            trY = mat[4];
            angleZ = atan2(trY, trX);
        }

        yaw = static_cast<float>(angleZ);
        pitch = static_cast<float>(angleX);
        roll = static_cast<float>(angleY);
    }
//## VR_PATCH END
}
