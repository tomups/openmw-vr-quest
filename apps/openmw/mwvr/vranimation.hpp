#ifndef MWVR_VRANIMATION_H
#define MWVR_VRANIMATION_H

#include "../mwrender/npcanimation.hpp"
#include "../mwrender/renderingmanager.hpp"
#include <components/vr/vr.hpp>
#include <components/vr/session.hpp>
#include <components/vr/space.hpp>

#include <osg/MatrixTransform>

namespace MWVR
{
    class HandController;
    class FingerController;
    class TrackingController;
    class Crosshair;
    class XrSpaceTransform;

    /// Subclassing NpcAnimation to implement VR related behaviour
    class VRAnimation : public MWRender::NpcAnimation, private VR::Session::Listener
    {
    protected:
        virtual void addControllers() override;

    public:
        /**
         * @param ptr
         * @param disableListener  Don't listen for equipment changes and magic effects. InventoryStore only supports
         *                         one listener at a time, so you shouldn't do this if creating several NpcAnimations
         *                         for the same Ptr, eg preview dolls for the player.
         *                         Those need to be manually rendered anyway.
         * @param disableSounds    Same as \a disableListener but for playing items sounds
         * @param xrSession        The XR session that shall be used to track limbs
         */
        VRAnimation(const MWWorld::Ptr& ptr, osg::ref_ptr<osg::Group> parentNode,
            Resource::ResourceSystem* resourceSystem, bool disableSounds, osg::ref_ptr<osg::Group> sceneRoot);
        virtual ~VRAnimation();

        /// Overridden to always be false
        void enableHeadAnimation(bool enable) override;

        /// Overridden to always be false
        void setAccurateAiming(bool enabled) override;

        /// Overriden to always be a variant of VM_VR*
        void setViewMode(ViewMode viewMode) override;

        /// Overriden to include VR modifications
        void updateParts() override;

        /// @return world transform that yields the position and orientation of the current weapon
        osg::Node* getWeaponTransform() { return mWeaponDirectionTransform.get(); }
        const osg::Node* getWeaponTransform() const { return mWeaponDirectionTransform.get(); }

        /// Enable pointers
        void enablePointers(bool left, bool right);

        void setEnableCrosshairs(bool enable);

        void updateLocalSpaceWorldPose();

        virtual void addAnimSource(std::string_view model, const std::string& baseModel) override;

        void updateSpace();

    protected:

        void updateCrosshairs() override;

        void updateCharHeight();

        void recenter();
        void onRecenter() override { recenter(); };
        void onInteractionProfileActiveChanged(XrPath topLevelPath, bool isActive) override;

        void updateTrackingControllers();

        void enableTracking(XrPath path);
        void disableTracking(XrPath path);
        void enablePointer(XrPath topLevelPath, bool enable);

        struct TrackingContext
        {
            bool enabled = false;
            XrPath topLevelPath;
            std::string spaceName;
            std::string forearmBone;
            std::unique_ptr<TrackingController> forearmController;
            std::string handBone;
            osg::ref_ptr<HandController> handController;
            std::string indexFingerBone[2];
            osg::ref_ptr<FingerController> indexFingerControllers[2];
        };

    protected:

        typedef std::map<XrPath, TrackingContext> TrackingContextMap;
        TrackingContextMap mVrControllers;
        osg::ref_ptr<osg::MatrixTransform> mModelOffset;
        osg::ref_ptr<osg::MatrixTransform> mWeaponDirectionTransform;
        osg::ref_ptr<osg::MatrixTransform> mWeaponPointerTransform;

        bool mRecenter = false;
        XrPath mLeftHandPath = XR_NULL_PATH;
        XrPath mRightHandPath = XR_NULL_PATH;
        bool mCrosshairsEnabled;
        float mCharHeight = 120.f;
        Stereo::Pose mHeadPoseInLocalSpace;
        Stereo::Pose mCharLocalSpacePose;
        float mCharacterYaw = 0.f;
        std::unique_ptr<MWVR::Crosshair> mCrosshairAmmo;
        std::unique_ptr<MWVR::Crosshair> mCrosshairThrown;
        std::unique_ptr<MWVR::Crosshair> mCrosshairSpell;
        osg::ref_ptr<osg::Transform> mKBMouseCrosshairTransform;
        osg::ref_ptr<osg::Group> mSceneRoot;
    };

}

#endif
