#include "realisticcombat.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwmechanics/weapontype.hpp"

#include <components/debug/debuglog.hpp>

#include <iomanip>

namespace MWVR {
    namespace RealisticCombat {

        static const char* stateToString(SwingState florida)
        {
            switch (florida)
            {
            case SwingState_Cooldown:
                return "Cooldown";
            case SwingState_Impact:
                return "Impact";
            case SwingState_Ready:
                return "Ready";
            case SwingState_Swing:
                return "Swing";
            case SwingState_Launch:
                return "Launch";
            }
            return "Error, invalid enum";
        }
        static const char* swingTypeToString(int type)
        {
            switch (type)
            {
            case ESM::Weapon::AT_Chop:
                return "Chop";
            case ESM::Weapon::AT_Slash:
                return "Slash";
            case ESM::Weapon::AT_Thrust:
                return "Thrust";
            case -1:
                return "Fail";
            default:
                return "Invalid";
            }
        }

        StateMachine::StateMachine(MWWorld::Ptr ptr, VRPath trackingPath)
            : mPtr(ptr) 
            , mMinVelocity(Settings::Manager::getFloat("realistic combat minimum swing velocity", "VR"))
            , mMaxVelocity(Settings::Manager::getFloat("realistic combat maximum swing velocity", "VR"))
            , mTrackingPath(trackingPath)
        {
            Log(Debug::Verbose) << "realistic combat minimum swing velocity: " << mMinVelocity;
            Log(Debug::Verbose) << "realistic combat maximum swing velocity: " << mMaxVelocity;
            Environment::get().getTrackingManager()->bind(this, "pcstage");
        }

        void StateMachine::onTrackingUpdated(VRTrackingSource& source, DisplayTime predictedDisplayTime)
        {
            mTrackingInput = source.getTrackingPose(predictedDisplayTime, mTrackingPath);
        }

        bool StateMachine::canSwing()
        {
            if (mSwingType >= 0)
                if (mVelocity >= mMinVelocity)
                    if (mSwingType != ESM::Weapon::AT_Thrust || mThrustVelocity >= 0.f)
                        return true;
            return false;
        }

        // Actions common to all transitions
        void StateMachine::transition(
            SwingState newState)
        {
            Log(Debug::Verbose) << "Transition:" << stateToString(mState) << "->" << stateToString(newState);
            mMaxSwingVelocity = 0.f;
            mTimeSinceEnteredState = 0.f;
            mMovementSinceEnteredState = 0.f;
            mState = newState;
        }

        void StateMachine::reset()
        {
            mMaxSwingVelocity = 0.f;
            mTimeSinceEnteredState = 0.f;
            mVelocity = 0.f;
            mPreviousPosition = osg::Vec3(0.f, 0.f, 0.f);
            mState = SwingState_Ready;
        }

        static bool isMeleeWeapon(int type)
        {
            if (MWMechanics::getWeaponType(type)->mWeaponClass != ESM::WeaponType::Melee)
                return false;
            if (type == ESM::Weapon::HandToHand)
                return true;
            if (type >= 0)
                return true;

            return false;
        }

        static bool isSideSwingValidForWeapon(int type)
        {
            switch (type)
            {
            case ESM::Weapon::HandToHand:
            case ESM::Weapon::BluntOneHand:
            case ESM::Weapon::BluntTwoClose:
            case ESM::Weapon::BluntTwoWide:
            case ESM::Weapon::SpearTwoWide:
                return true;
            case ESM::Weapon::ShortBladeOneHand:
            case ESM::Weapon::LongBladeOneHand:
            case ESM::Weapon::LongBladeTwoHand:
            case ESM::Weapon::AxeOneHand:
            case ESM::Weapon::AxeTwoHand:
            default:
                return false;
            }
        }

        void StateMachine::update(float dt, bool enabled)
        {
            auto* world = MWBase::Environment::get().getWorld();
            auto& handPose = mTrackingInput.pose;
            auto weaponType = world->getActiveWeaponType();

            enabled = enabled && isMeleeWeapon(weaponType);
            enabled = enabled && !!mTrackingInput.status;

            if (mEnabled != enabled)
            {
                reset();
                mEnabled = enabled;
            }
            if (!enabled)
                return;

            mTimeSinceEnteredState += dt;


            // First determine direction of different swing types

            // Discover orientation of weapon
            osg::Quat weaponDir = handPose.orientation;

            // Morrowind models do not hold weapons at a natural angle, so i rotate the hand forward 
            // to get a more natural angle on the weapon to allow more comfortable combat.
            if (weaponType != ESM::Weapon::HandToHand)
                weaponDir = osg::Quat(osg::PI_4, osg::Vec3{ 1,0,0 }) * weaponDir;

            // Thrust means stabbing in the direction of the weapon
            osg::Vec3 thrustDirection = weaponDir * osg::Vec3{ 0,1,0 };

            // Slash and Chop are vertical, relative to the orientation of the weapon (direction of the sharp edge / hammer)
            osg::Vec3 slashChopDirection = weaponDir * osg::Vec3{ 0,0,1 };

            // Side direction of the weapon (i.e. The blunt side of the sword)
            osg::Vec3 sideDirection = weaponDir * osg::Vec3{ 1,0,0 };


            // Next determine current hand movement

            // If tracking is lost, openxr will return a position of 0
            // So i reset position when tracking is re-acquired to avoid a superspeed strike.
            // Theoretically, the player's hand really could be at 0,0,0
            // but that's a super rare case so whatever.
            if (mPreviousPosition == osg::Vec3(0.f, 0.f, 0.f))
                mPreviousPosition = handPose.position;

            osg::Vec3 movement = handPose.position - mPreviousPosition;
            mMovementSinceEnteredState += movement.length();
            mPreviousPosition = handPose.position;
            osg::Vec3 swingVector = movement / dt;
            osg::Vec3 swingDirection = swingVector;
            swingDirection.normalize();

            // Compute swing velocities

            // Thrust follows the orientation of the weapon. Negative thrust = no attack.
            mThrustVelocity = swingVector * thrustDirection;
            mVelocity = swingVector.length();


            if (isSideSwingValidForWeapon(weaponType))
            {
                // Compute velocity in the plane normal to the thrust direction.
                float thrustComponent = std::abs(mThrustVelocity / mVelocity);
                float planeComponent = std::sqrt(1 - thrustComponent * thrustComponent);
                mSlashChopVelocity = mVelocity * planeComponent;
                mSideVelocity = -1000.f;
            }
            else
            {
                // If side swing is not valid for the weapon, count slash/chop only along in
                // the direction of the weapon's edge.
                mSlashChopVelocity = std::abs(swingVector * slashChopDirection);
                mSideVelocity = std::abs(swingVector * sideDirection);
            }


            float orientationVerticality = std::abs(thrustDirection * osg::Vec3{ 0,0,1 });
            float swingVerticality = std::abs(swingDirection * osg::Vec3{ 0,0,1 });

            // Pick swing type based on greatest current velocity
            // Note i use abs() of thrust velocity to prevent accidentally triggering
            // chop/slash when player is withdrawing the weapon.
            if (mSideVelocity > std::abs(mThrustVelocity) && mSideVelocity > mSlashChopVelocity)
            {
                // Player is swinging with the "blunt" side of a weapon that
                // cannot be used that way.
                mSwingType = -1;
            }
            else if (std::abs(mThrustVelocity) > mSlashChopVelocity)
            {
                mSwingType = ESM::Weapon::AT_Thrust;
            }
            else
            {
                // First check if the weapon is pointing upwards. In which case slash is not 
                // applicable, and the attack must be a chop.
                if (orientationVerticality > 0.707)
                    mSwingType = ESM::Weapon::AT_Chop;
                else
                {
                    // Next check if the swing is more horizontal or vertical. A slash
                    // would be more horizontal.
                    if (swingVerticality > 0.707)
                        mSwingType = ESM::Weapon::AT_Chop;
                    else
                        mSwingType = ESM::Weapon::AT_Slash;
                }
            }

            switch (mState)
            {
            case SwingState_Cooldown:
                return update_cooldownState();
            case SwingState_Ready:
                return update_readyState();
            case SwingState_Swing:
                return update_swingState();
            case SwingState_Impact:
                return update_impactState();
            case SwingState_Launch:
                return update_launchState();
            default:
                throw std::logic_error(std::string("You forgot to implement state ") + stateToString(mState) + " ya dingus");
            }
        }

        void StateMachine::update_cooldownState()
        {
            if (mTimeSinceEnteredState >= mMinimumPeriod)
                transition_cooldownToReady();
        }

        void StateMachine::transition_cooldownToReady()
        {
            transition(SwingState_Ready);
        }

        void StateMachine::update_readyState()
        {
            if (canSwing())
                return transition_readyToLaunch();
        }

        void StateMachine::transition_readyToLaunch()
        {
            transition(SwingState_Launch);
        }

        void StateMachine::playSwish()
        {
            MWBase::SoundManager* sndMgr = MWBase::Environment::get().getSoundManager();

            std::string sound = "Weapon Swish";
            if (mStrength < 0.5f)
                sndMgr->playSound3D(mPtr, sound, 1.0f, 0.8f); //Weak attack
            if (mStrength < 1.0f)
                sndMgr->playSound3D(mPtr, sound, 1.0f, 1.0f); //Medium attack
            else
                sndMgr->playSound3D(mPtr, sound, 1.0f, 1.2f); //Strong attack

            Log(Debug::Verbose) << "Swing: " << swingTypeToString(mSwingType);
        }

        void StateMachine::update_launchState()
        {
            if (mMovementSinceEnteredState > mMinimumPeriod)
                transition_launchToSwing();
            if (!canSwing())
                return transition_launchToReady();
        }

        void StateMachine::transition_launchToReady()
        {
            transition(SwingState_Ready);
        }

        void StateMachine::transition_launchToSwing()
        {
            playSwish();
            transition(SwingState_Swing);

            // As a special case, update the new state immediately to allow
            // same-frame impacts.
            update_swingState();
        }

        void StateMachine::update_swingState()
        {
            mMaxSwingVelocity = std::max(mVelocity, mMaxSwingVelocity);
            mStrength = std::min(1.f, (mMaxSwingVelocity - mMinVelocity) / mMaxVelocity);

            // When velocity falls below minimum, transition to register the miss
            if (!canSwing())
                return transition_swingingToImpact();
            // Call hit with simulated=true to check for hit without actually causing an impact
            if (mPtr.getClass().hit(mPtr, mStrength, mSwingType, true))
                return transition_swingingToImpact();
        }

        void StateMachine::transition_swingingToImpact()
        {
            mPtr.getClass().hit(mPtr, mStrength, mSwingType, false);
            transition(SwingState_Impact);
        }

        void StateMachine::update_impactState()
        {
            if (mVelocity < mMinVelocity)
                return transition_impactToCooldown();
        }

        void StateMachine::transition_impactToCooldown()
        {
            transition(SwingState_Cooldown);
        }

    }
}
