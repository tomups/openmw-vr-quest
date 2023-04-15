#ifndef STEREO_TYPES_H
#define STEREO_TYPES_H

#include <osg/Matrix>
#include <osg/Vec3>

#include <components/misc/constants.hpp>

namespace Stereo
{
    enum class Eye
    {
        Left = 0,
        Right = 1,
        Center = 2
    };

//## VR_PATCH BEGIN
    //! Wrapper type for position data, that handles conversion between MW units and metric units.
    //! Use this type instead of repeatedly transforming between units.
    struct Unit
    {
        static Unit fromMeters(float meters) { return Unit(meters * Constants::UnitsPerMeter); }
        static Unit fromMWUnits(float mwUnit) { return Unit(mwUnit); }

        Unit()
            : mMWUnit(0.f)
        {
        }

        Unit operator+(Unit rhs) const { return Unit(mMWUnit + rhs.mMWUnit); }
        const Unit& operator+=(Unit rhs)
        {
            mMWUnit += rhs.mMWUnit;
            return *this;
        }

        Unit operator-(Unit rhs) const { return Unit(mMWUnit - rhs.mMWUnit); }
        const Unit& operator-=(Unit rhs)
        {
            mMWUnit -= rhs.mMWUnit;
            return *this;
        }

        Unit operator*(float rhs) const { return Unit(mMWUnit * rhs); }
        const Unit& operator*=(Unit rhs)
        {
            mMWUnit *= rhs.mMWUnit;
            return *this;
        }

        Unit operator/(float rhs) const { return Unit(mMWUnit / rhs); }
        const Unit& operator/=(Unit rhs)
        {
            mMWUnit /= rhs.mMWUnit;
            return *this;
        }

        Unit operator-() const { return Unit(-mMWUnit); }

        // Any unit dividing itself results in a scalar, so return a scalar
        float operator/(Unit rhs) const { return mMWUnit / rhs.mMWUnit; }

        bool operator==(const Unit& rhs) const { return mMWUnit == rhs.mMWUnit; }
        bool operator!=(const Unit& rhs) const { return mMWUnit != rhs.mMWUnit; }
        bool operator<=(const Unit& rhs) const { return mMWUnit <= rhs.mMWUnit; }
        bool operator>=(const Unit& rhs) const { return mMWUnit >= rhs.mMWUnit; }
        bool operator<(const Unit& rhs) const { return mMWUnit < rhs.mMWUnit; }
        bool operator>(const Unit& rhs) const { return mMWUnit > rhs.mMWUnit; }

        float asMeters() const { return mMWUnit / Constants::UnitsPerMeter; }
        float asMWUnits() const { return mMWUnit; }

    private:
        Unit(float mwUnit)
            : mMWUnit(mwUnit)
        {
        }

        float mMWUnit = 0.f;
    };

    struct Position
    {
        static Position fromMeters(const osg::Vec3f& pos) { return Position(pos * Constants::UnitsPerMeter); }
        static Position fromMeters(float x, float y, float z)
        {
            return Position(osg::Vec3f(x, y, z) * Constants::UnitsPerMeter);
        }
        static Position fromMWUnits(const osg::Vec3f& pos) { return Position(pos); }
        static Position fromMWUnits(float x, float y, float z) { return Position(osg::Vec3f(x, y, z)); }

        Position() {}
        Position(Unit x, Unit y, Unit z)
            : mX(x)
            , mY(y)
            , mZ(z)
        {
        }

        Position operator+(const Position& rhs) const { return Position(mX + rhs.mX, mY + rhs.mY, mZ + rhs.mZ); }
        const Position& operator+=(const Position& rhs)
        {
            *this = *this + rhs;
            return *this;
        }

        Position operator-(const Position& rhs) const { return Position(mX - rhs.mX, mY - rhs.mY, mZ - rhs.mZ); }
        const Position& operator-=(const Position& rhs)
        {
            *this = *this - rhs;
            return *this;
        }

        Position operator*(float rhs) const { return Position(mX * rhs, mY * rhs, mZ * rhs); }
        const Position& operator*=(float rhs)
        {
            *this = *this * rhs;
            return *this;
        }

        Position operator/(float rhs) const { return Position(mX / rhs, mY / rhs, mZ / rhs); }
        const Position& operator/=(float rhs)
        {
            *this = *this / rhs;
            return *this;
        }

        // A unit dividing itself results in a scalar, so return a scalar
        osg::Vec3f operator/(Position rhs) const { return osg::Vec3f(mX / rhs.mX, mY / rhs.mY, mZ / rhs.mZ); }

        const Position& operator*=(const osg::Quat& rhs)
        {
            *this = fromMWUnits(rhs * asMWUnits());
            return *this;
        }

        Position operator-() const { return Position(-mX, -mY, -mZ); }

        bool operator==(const Position& rhs) const { return mX == rhs.mX && mY == rhs.mY && mZ == rhs.mZ; }
        bool operator!=(const Position& rhs) const { return !(*this == rhs); }

        osg::Vec3 asMeters() const { return asMWUnits() / Constants::UnitsPerMeter; }
        osg::Vec3 asMWUnits() const { return osg::Vec3f(mX.asMWUnits(), mY.asMWUnits(), mZ.asMWUnits()); }

        Unit mX;
        Unit mY;
        Unit mZ;

    private:
        Position(const osg::Vec3f& pos)
        {
            mX = Unit::fromMWUnits(pos.x());
            mY = Unit::fromMWUnits(pos.y());
            mZ = Unit::fromMWUnits(pos.z());
        }
    };

    inline Position operator*(const osg::Quat& lhs, const Position& rhs)
    {
        return Position::fromMWUnits(lhs * rhs.asMWUnits());
    }

    struct Pose
    {
        //! Position in space
        Position position{};

        //! Orientation in space.
        osg::Quat orientation{ 0, 0, 0, 1 };

        //! Add one pose to another
        Pose operator+(const Pose& rhs);
        const Pose& operator+=(const Pose& rhs);

        //! Scale a pose (does not affect orientation)
        Pose operator*(float scalar);
        const Pose& operator*=(float scalar);
        Pose operator/(float scalar);
        const Pose& operator/=(float scalar);

        bool operator==(const Pose& rhs) const;
    };

    struct FieldOfView
    {
        float angleLeft{ 0.f };
        float angleRight{ 0.f };
        float angleUp{ 0.f };
        float angleDown{ 0.f };

        bool operator==(const FieldOfView& rhs) const;
    };

    struct View
    {
        Pose pose;
        FieldOfView fov;
        bool operator==(const View& rhs) const;

        osg::Matrix viewMatrix(bool useGLConventions);
        osg::Matrix perspectiveMatrix(float near, float far, bool reverseZ);
    };

    std::ostream& operator<<(std::ostream& os, const Pose& pose);
    std::ostream& operator<<(std::ostream& os, const FieldOfView& fov);
    std::ostream& operator<<(std::ostream& os, const View& view);
//## VR_PATCH BEGIN

    void getEulerAngles(const osg::Quat& quat, float& yaw, float& pitch, float& roll);
//## VR_PATCH END
}

#endif
