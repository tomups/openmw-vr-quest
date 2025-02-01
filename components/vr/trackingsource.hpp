//#ifndef VR_TRACKING_SOURCE_H
//#define VR_TRACKING_SOURCE_H
//
//#include <components/vr/vr.hpp>
//
//#include <map>
//
//namespace VR
//{
//    class TrackingManager;
//    class TrackingListener;
//    struct Frame;
//
//    //! Ties a tracking source to the game world.
//    //! A reference pose is selected by passing its path to the constructor.
//    //! All poses are transformed in the horizontal plane by moving the x,y origin to the position of the reference
//    //! pose, and then reoriented using the current orientation of the player character. The reference pose is
//    //! effectively always at the x,y origin, and its movement is accumulated and can be read using the movement() call.
//    //! If this movement is ever consumed (such as by moving the character to follow the player) the consumed movement
//    //! must be reported using consumeMovement().
//    class StageToWorldBinding
//    {
//    public:
//        StageToWorldBinding(std::string_view);
//
//        void setWorldOrientation(float yaw, bool adjust);
//        osg::Quat getWorldOrientation() const { return mOrientation; }
//
//        void setEyeLevel(Stereo::Unit eyeLevel);
//
//        //! The player's movement within the VR stage. This accumulates until the movement has been consumed by calling
//        //! consumeMovement()
//        const Stereo::Position& movement() const { return mMovement; }
//
//        //! Consume movement
//        void consumeMovement(const Stereo::Position& movement);
//
//        //! Recenter tracking by consuming all movement.
//        void recenter();
//
//        //! World origin is the point that ties the stage and the world. (0,0,0 in the world-aligned stage is this
//        //! node). If no node is set, the world-aligned stage and the world correspond 1-1.
//        void setOriginNode(osg::Node* origin) { mOrigin = origin; }
//
//        void bindPaths(VRPath worldPath, VRPath stagePath);
//        void unbindPath(VRPath worldPath);
//
//        void instantTransition();
//
//    protected:
//        //! Fetches a pose from the source, and then aligns it with the game world if the reference is 0 (stage).
//        TrackingPose locate(VRPath path) override;
//
//        //! List currently supported tracking paths.
//        std::vector<VRPath> listSupportedPaths() const override;
//
//        //! Call once per frame, after (or at the end of) OSG update traversals and before cull traversals.
//        //! Predict tracked poses for the given display time.
//        //! \arg predictedDisplayTime [in] the predicted display time. The pose shall be predicted for this time based
//        //! on current tracking data.
//        void updateTracking() override;
//
//    private:
//        std::string mReferenceAction;
//        std::map<VRPath, VRPath> mBindings;
//        osg::Node* mOrigin = nullptr;
//        bool mHasTrackingData = false;
//        bool mInstantTransition = false;
//        Stereo::Unit mEyeLevel = {};
//        Stereo::Pose mOriginWorldPose = Stereo::Pose();
//        TrackingPose mLastPose = VR::TrackingPose();
//        Stereo::Position mMovement = {};
//        osg::Quat mOrientation = osg::Quat(0, 0, 0, 1);
//    };
//}
//
//#endif
