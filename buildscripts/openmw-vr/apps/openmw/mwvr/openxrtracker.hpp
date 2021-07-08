#ifndef OPENXR_TRACKER_HPP
#define OPENXR_TRACKER_HPP

#include <openxr/openxr.h>
#include "vrtracking.hpp"

#include <map>

namespace MWVR
{
    //! Serves as a C++ wrapper of openxr spaces, but also bridges stage coordinates and game coordinates.
    //! Supports the compulsory sets of paths.
    class OpenXRTracker : public VRTrackingSource
    {
    public:
        OpenXRTracker(const std::string& name, XrSpace referenceSpace);
        ~OpenXRTracker();

        void addTrackingSpace(VRPath path, XrSpace space);
        void deleteTrackingSpace(VRPath path);

        //! The base space used to reference everything else.
        void setReferenceSpace(XrSpace referenceSpace);

        std::vector<VRPath> listSupportedTrackingPosePaths() const override;
        void updateTracking(DisplayTime predictedDisplayTime) override;

    protected:
        VRTrackingPose getTrackingPoseImpl(DisplayTime predictedDisplayTime, VRPath path, VRPath reference = 0) override;

    private:
        std::array<View, 2> locateViews(DisplayTime predictedDisplayTime, XrSpace reference);
        void locate(VRTrackingPose& pose, XrSpace space, XrSpace reference, DisplayTime predictedDisplayTime);
        XrSpace getSpace(VRPath);

        XrSpace mReferenceSpace;
        std::map<VRPath, XrSpace> mTrackingSpaces;
    };

    //! Ties a tracked pose to the game world.
    //! A movement tracking pose is selected by passing its path to the constructor. 
    //! All poses are transformed in the horizontal plane by moving the x,y origin to the position of the movement tracking pose, and then reoriented using the current orientation.
    //! The movement tracking pose is effectively always at the x,y origin
    //! The movement of the movement tracking pose is accumulated and can be read using the movement() call.
    //! If this movement is ever consumed (such as by moving the character to follow the player) the consumed movement must be reported using consumeMovement().
    class OpenXRTrackingToWorldBinding
    {
    public:
        OpenXRTrackingToWorldBinding();

        //! Re-orient the stage.
        void setOrientation(float yaw, bool adjust);
        osg::Quat getOrientation() const { return mOrientation; }

        void setEyeLevel(float eyeLevel) { mEyeLevel = eyeLevel; }
        float getEyeLevel() const { return mEyeLevel; }

        void setSeatedPlay(bool seatedPlay) { mSeatedPlay = seatedPlay; }
        bool getSeatedPlay() const { return mSeatedPlay; }

        //! The player's movement within the VR stage. This accumulates until the movement has been consumed by calling consumeMovement()
        osg::Vec3 movement() const;

        //! Consume movement
        void consumeMovement(const osg::Vec3& movement);

        //! Recenter tracking by consuming all movement.
        void recenter(bool resetZ);

        void update(Pose movementTrackingPose);

        //! Transforms a stage-referenced pose to be world-aligned.
        //! \note Unlike VRTrackingSource::getTrackingPose() this does not take a reference path, as re-alignment is only needed when fetching a stage-referenced pose.
        void alignPose(Pose& pose);

    private:
        bool mSeatedPlay = false;
        bool mHasTrackingData = false;
        float mEyeLevel = 0;
        Pose mLastPose = Pose();
        osg::Vec3 mMovement = osg::Vec3(0,0,0);
        osg::Quat mOrientation = osg::Quat(0,0,0,1);
    };
}

#endif
