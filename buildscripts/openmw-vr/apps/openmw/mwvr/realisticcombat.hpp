#ifndef MWVR_REALISTICCOMBAT_H
#define MWVR_REALISTICCOMBAT_H

#include <components/esm/loadweap.hpp>

#include "../mwbase/world.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/class.hpp"

#include "vrenvironment.hpp"
#include "vrsession.hpp"
#include "vrtracking.hpp"

namespace MWVR {
    namespace RealisticCombat {

        /// Enum describing the current state of the MWVR::RealisticCombat::StateMachine
        enum SwingState
        {
            SwingState_Ready,
            SwingState_Launch,
            SwingState_Swing,
            SwingState_Impact,
            SwingState_Cooldown,
        };

        /////////////////////////////////////////////////////////////////////
        /// \brief State machine for "realistic" combat in openmw vr
        ///
        /// \sa SwingState
        ///
        ///  Initial state: Ready.
        ///
        ///  State Ready: Ready to initiate a new attack.
        ///  State Launch: Player has begun swinging his weapon.
        ///  State Swing: Currently swinging weapon.
        ///  State Impact: Contact made, weapon still swinging.
        ///  State Cooldown: Swing completed, wait a minimum period before next.
        ///
        ///  Transition rules:
        ///    Ready     -> Launch:      When the minimum velocity of swing is achieved.
        ///    Launch    -> Ready:       When the minimum velocity of swing is lost before minimum distance was swung.
        ///    Launch    -> Swing:       When minimum distance is swung.
        ///                                  - Play Swish sound
        ///    Swing     -> Impact:      When minimum velocity is lost, or when a hit is detected.
        ///                                  - Register hit based on max velocity observed in swing state
        ///    Impact    -> Cooldown:    When velocity returns below minimum.
        ///    Cooldown  -> Ready:       When the minimum period has passed since entering Cooldown state
        ///
        ///
        struct StateMachine : public VRTrackingListener
        {
        public:
            StateMachine(MWWorld::Ptr ptr, VRPath trackingPath);
            void update(float dt, bool enabled);
            MWWorld::Ptr ptr() { return mPtr; }

        protected:
            void onTrackingUpdated(VRTrackingSource& source, DisplayTime predictedDisplayTime) override;

            bool canSwing();

            void playSwish();
            void reset();

            void transition(SwingState newState);

            void update_cooldownState();
            void transition_cooldownToReady();

            void update_readyState();
            void transition_readyToLaunch();

            void update_launchState();
            void transition_launchToReady();
            void transition_launchToSwing();

            void update_swingState();
            void transition_swingingToImpact();

            void update_impactState();
            void transition_impactToCooldown();

        private:
            MWWorld::Ptr mPtr;
            const float mMinVelocity;
            const float mMaxVelocity;

            float mVelocity = 0.f;
            float mMaxSwingVelocity = 0.f;

            SwingState mState = SwingState_Ready;
            int mSwingType = -1;
            float mStrength = 0.f;

            float mThrustVelocity{ 0.f };
            float mSlashChopVelocity{ 0.f };
            float mSideVelocity{ 0.f };

            float mMinimumPeriod{ .25f };

            float mTimeSinceEnteredState = { 0.f };
            float mMovementSinceEnteredState = { 0.f };

            bool mEnabled = false;

            osg::Vec3       mPreviousPosition{ 0.f,0.f,0.f };
            VRTrackingPose  mTrackingInput = VRTrackingPose();
            VRPath          mTrackingPath = 0;
        };

    }
}

#endif
