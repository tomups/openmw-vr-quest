#ifndef MISC_MATHUTIL_H
#define MISC_MATHUTIL_H

#include <osg/Math>
#include <osg/Matrixf>
#include <osg/Matrixd>
#include <osg/Quat>
#include <osg/Vec2f>
#include <osg/Vec3f>

#include <cmath>
#include <type_traits>

namespace Misc
{
    /// Normalizes given angle to the range [-PI, PI]. E.g. PI*3/2 -> -PI/2.
    inline double normalizeAngle(double angle)
    {
        double fullTurns = angle / (2 * osg::PI) + 0.5;
        return (fullTurns - floor(fullTurns) - 0.5) * (2 * osg::PI);
    }

    /// Rotates given 2d vector counterclockwise. Angle is in radians.
    inline osg::Vec2f rotateVec2f(osg::Vec2f vec, float angle)
    {
        float s = std::sin(angle);
        float c = std::cos(angle);
        return osg::Vec2f(vec.x() * c + vec.y() * -s, vec.x() * s + vec.y() * c);
    }

    inline osg::Vec3f toEulerAnglesXZ(osg::Vec3f forward)
    {
        float x = -std::asin(forward.z());
        float z = std::atan2(forward.x(), forward.y());
        return osg::Vec3f(x, 0, z);
    }

    inline osg::Vec3f toEulerAnglesXZ(const osg::Quat& quat)
    {
        osg::Vec3f forward = quat * osg::Vec3f(0, 1, 0);
        forward.normalize();
        return toEulerAnglesXZ(forward);
    }

    inline osg::Vec3f toEulerAnglesXZ(const osg::Matrixf& m)
    {
        osg::Vec3f forward(m(1, 0), m(1, 1), m(1, 2));
        forward.normalize();
        return toEulerAnglesXZ(forward);
    }

    inline osg::Vec3f toEulerAnglesZYX(osg::Vec3f forward, osg::Vec3f up)
    {
        float y = -std::asin(up.x());
        float x = std::atan2(up.y(), up.z());
        osg::Vec3f forwardZ = (osg::Quat(x, osg::Vec3f(1, 0, 0)) * osg::Quat(y, osg::Vec3f(0, 1, 0))) * forward;
        float z = std::atan2(forwardZ.x(), forwardZ.y());
        return osg::Vec3f(x, y, z);
    }

    inline osg::Vec3f toEulerAnglesZYX(const osg::Quat& quat)
    {
        osg::Vec3f forward = quat * osg::Vec3f(0, 1, 0);
        forward.normalize();
        osg::Vec3f up = quat * osg::Vec3f(0, 0, 1);
        up.normalize();
        return toEulerAnglesZYX(forward, up);
    }

    inline osg::Vec3f toEulerAnglesZYX(const osg::Matrixf& m)
    {
        osg::Vec3f forward(m(1, 0), m(1, 1), m(1, 2));
        osg::Vec3f up(m(2, 0), m(2, 1), m(2, 2));
        forward.normalize();
        up.normalize();
        return toEulerAnglesZYX(forward, up);
    }

    inline void getEulerAngles(const osg::Quat& quat, float& yaw, float& pitch, float& roll)
    {
        // Now do the computation
        osg::Matrixd m2(osg::Matrixd::rotate(quat));
        double* mat = (double*)m2.ptr();
        double angle_x = 0.0;
        double angle_y = 0.0;
        double angle_z = 0.0;
        double C, tr_x, tr_y;
        angle_y = asin(mat[2]); /* Calculate Y-axis angle */
        C = cos(angle_y);
        if (fabs(C) > 0.005) /* Test for Gimball lock? */
        {
            tr_x = mat[10] / C; /* No, so get X-axis angle */
            tr_y = -mat[6] / C;
            angle_x = atan2(tr_y, tr_x);
            tr_x = mat[0] / C; /* Get Z-axis angle */
            tr_y = -mat[1] / C;
            angle_z = atan2(tr_y, tr_x);
        }
        else /* Gimball lock has occurred */
        {
            angle_x = 0; /* Set X-axis angle to zero
                          */
            tr_x = mat[5]; /* And calculate Z-axis angle
                            */
            tr_y = mat[4];
            angle_z = atan2(tr_y, tr_x);
        }

        yaw = angle_z;
        pitch = angle_x;
        roll = angle_y;
    }

    template <class T>
    bool isPowerOfTwo(T x)
    {
        static_assert(std::is_integral_v<T>);
        return ((x > 0) && ((x & (x - 1)) == 0));
    }

    inline int nextPowerOfTwo(int v)
    {
        if (isPowerOfTwo(v))
            return v;
        int depth = 0;
        while (v)
        {
            v >>= 1;
            depth++;
        }
        return 1 << depth;
    }
}

#endif
