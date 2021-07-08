#ifndef MWVR_VRTRACKING_H
#define MWVR_VRTRACKING_H

#include <memory>
#include <map>
#include <set>
#include <vector>
#include <mutex>
#include "vrtypes.hpp"

namespace MWVR
{
    class VRAnimation;

    //! Describes the status of the tracking data. Note that there are multiple success statuses, and predicted poses should be used whenever the status is a non-negative integer.
    enum class TrackingStatus : signed
    {
        Unknown = 0, //!< No data has been written (default value)
        Good = 1,  //!< Accurate, up-to-date tracking data was used.
        Stale = 2, //!< Inaccurate, stale tracking data was used. This code is a status warning, not an error, and the tracking pose should be used.
        NotTracked = -1, //!< No tracking data was returned because the tracking source does not track that
        Lost = -2,  //!< No tracking data was returned because the tracking source could not be read (occluded controller, network connectivity issues, etc.).
        RuntimeFailure = -3 //!< No tracking data was returned because of a runtime failure.
    };

    inline bool operator!(TrackingStatus status)
    {
        return static_cast<signed>(status) < static_cast<signed>(TrackingStatus::Good);
    }

    //! @brief An identifier representing an OpenXR path. 0 represents no path.
    //! A VRPath can be optained from the string representation of a path using VRTrackingManager::getTrackingPath()
    //! \note Support is determined by each VRTrackingSource. ALL strings are convertible to VRPaths but won't be useful unless they match a supported string.
    //! \sa VRTrackingManager::getTrackingPath()
    using VRPath = uint64_t;

    //! A single tracked pose
    struct VRTrackingPose
    {
        TrackingStatus status = TrackingStatus::Unknown; //!< State of the prediction. 
        Pose pose = {}; //!< The predicted pose. 
        DisplayTime time; //!< The time for which the pose was predicted.
    };

    //! Source for tracking data. Converts paths to poses at predicted times.
    //! \par The following paths are compulsory and must be supported by the implementation.
    //! - /user/head/input/pose
    //! - /user/hand/left/input/aim/pose
    //! - /user/hand/right/input/aim/pose
    //! - /user/hand/left/input/grip/pose (Not actually implemented yet)
    //! - /user/hand/right/input/grip/pose (Not actually implemented yet)
    //! \note A path being *supported* does not guarantee tracking data will be available at any given time (or ever).
    //! \note Implementations may expand this list.
    //! \sa OpenXRTracker VRGUITracking OpenXRTrackingToWorldBinding
    class VRTrackingSource
    {
    public:
        VRTrackingSource(const std::string& name);
        virtual ~VRTrackingSource();

        //! @brief Predicted pose of the given path at the predicted time
        //! 
        //! \arg predictedDisplayTime[in] Time to predict. This is normally the predicted display time. If time is 0, the last pose that was predicted is returned.
        //! \arg path[in] path of the pose requested. Should match an available pose path. 
        //! \arg reference[in] path of the pose to use as reference. If 0, pose is referenced to the VR stage.
        //! 
        //! \return A structure describing a pose and the tracking status.
        VRTrackingPose getTrackingPose(DisplayTime predictedDisplayTime, VRPath path, VRPath reference = 0);

        //! List currently supported tracking paths.
        virtual std::vector<VRPath> listSupportedTrackingPosePaths() const = 0;

        //! Returns true if the available poses changed since the last frame, false otherwise.
        bool availablePosesChanged() const;

        void clearAvailablePosesChanged();

        //! Call once per frame, after (or at the end of) OSG update traversals and before cull traversals.
        //! Predict tracked poses for the given display time.
        //! \arg predictedDisplayTime [in] the predicted display time. The pose shall be predicted for this time based on current tracking data.
        virtual void updateTracking(DisplayTime predictedDisplayTime) = 0;

        void clearCache();

    protected:
        virtual VRTrackingPose getTrackingPoseImpl(DisplayTime predictedDisplayTime, VRPath path, VRPath reference = 0) = 0;

        std::map<std::pair<VRPath, VRPath>, VRTrackingPose> mCache;

        void notifyAvailablePosesChanged();

        bool mAvailablePosesChanged = true;
    };

    //! Ties a tracking source to the game world.
    //! A reference pose is selected by passing its path to the constructor. 
    //! All poses are transformed in the horizontal plane by moving the x,y origin to the position of the reference pose, and then reoriented using the current orientation of the player character.
    //! The reference pose is effectively always at the x,y origin, and its movement is accumulated and can be read using the movement() call.
    //! If this movement is ever consumed (such as by moving the character to follow the player) the consumed movement must be reported using consumeMovement().
    class VRTrackingToWorldBinding : public VRTrackingSource
    {
    public:
        VRTrackingToWorldBinding(const std::string& name, VRTrackingSource* source, VRPath reference);

        void setWorldOrientation(float yaw, bool adjust);
        osg::Quat getWorldOrientation() const { return mOrientation; }

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

        //! World origin is the point that ties the stage and the world. (0,0,0 in the world-aligned stage is this node).
        //! If no node is set, the world-aligned stage and the world correspond 1-1.
        void setOriginNode(osg::Node* origin) { mOrigin = origin; }

    protected:
        //! Fetches a pose from the source, and then aligns it with the game world if the reference is 0 (stage). 
        VRTrackingPose getTrackingPoseImpl(DisplayTime predictedDisplayTime, VRPath path, VRPath movementReference = 0) override;

        //! List currently supported tracking paths.
        std::vector<VRPath> listSupportedTrackingPosePaths() const override;

        //! Call once per frame, after (or at the end of) OSG update traversals and before cull traversals.
        //! Predict tracked poses for the given display time.
        //! \arg predictedDisplayTime [in] the predicted display time. The pose shall be predicted for this time based on current tracking data.
        void updateTracking(DisplayTime predictedDisplayTime) override;

    private:
        VRPath mMovementReference;
        VRTrackingSource* mSource;
        osg::Node* mOrigin = nullptr;
        bool mSeatedPlay = false;
        bool mHasTrackingData = false;
        float mEyeLevel = 0;
        Pose mOriginWorldPose = Pose();
        Pose mLastPose = Pose();
        osg::Vec3 mMovement = osg::Vec3(0, 0, 0);
        osg::Quat mOrientation = osg::Quat(0, 0, 0, 1);
    };

    class VRTrackingListener
    {
    public:
        virtual ~VRTrackingListener();

        //! Notify that available tracking poses have changed.
        virtual void onAvailablePosesChanged(VRTrackingSource& source) {};

        //! Notify that a tracking source has been attached
        virtual void onTrackingAttached(VRTrackingSource& source) {};

        //! Notify that a tracking source has been detached.
        virtual void onTrackingDetached() {};

        //! Called every frame after tracking poses have been updated
        virtual void onTrackingUpdated(VRTrackingSource& source, DisplayTime predictedDisplayTime) {};

    private:
    };

    class VRTrackingManager
    {
    public:
        VRTrackingManager();
        ~VRTrackingManager();

        //! Angles to be used for overriding movement direction
        //void movementAngles(float& yaw, float& pitch);

        void updateTracking();

        //! Bind listener to source, listener will receive tracking updates from source until unbound.
        //! \note A single listener can only receive tracking updates from one source.
        void bind(VRTrackingListener* listener, std::string source);

        //! Unbind listener, listener will no longer receive tracking updates.
        void unbind(VRTrackingListener* listener);

        //! Converts a string representation of a path to a VRTrackerPath identifier
        VRPath stringToVRPath(const std::string& path);

        //! Converts a path identifier back to string. Returns an empty string if no such identifier exists.
        std::string VRPathToString(VRPath path);

        //! Get a tracking source by name
        VRTrackingSource* getSource(const std::string& name);

        //! Angles to be used for overriding movement direction
        void movementAngles(float& yaw, float& pitch);

        void processChangedSettings(const std::set< std::pair<std::string, std::string> >& changed);

    private:
        friend class VRTrackingSource;
        void registerTrackingSource(VRTrackingSource* source, const std::string& name);
        void unregisterTrackingSource(VRTrackingSource* source);
        void notifySourceChanged(const std::string& name);
        void updateMovementAngles(DisplayTime predictedDisplayTime);

        std::map<std::string, VRTrackingSource*> mSources;
        std::map<VRTrackingListener*, std::string> mBindings;
        std::map<std::string, VRPath> mPathIdentifiers;

        bool mHandDirectedMovement = 0.f;
        VRPath mHeadPath = 0;
        VRPath mHandPath = 0;
        float mMovementYaw = 0.f;
        float mMovementPitch = 0.f;
    };
}

#endif
