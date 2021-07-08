#include "vranimation.hpp"
#include "vrenvironment.hpp"
#include "vrviewer.hpp"
#include "vrinputmanager.hpp"
#include "vrcamera.hpp"
#include "vrutil.hpp"
#include "vrpointer.hpp"

#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include <osg/Depth>
#include <osg/Drawable>
#include <osg/Object>
#include <osg/BlendFunc>
#include <osg/Fog>
#include <osg/LightModel>

#include <components/debug/debuglog.hpp>

#include <components/sceneutil/actorutil.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/sceneutil/shadow.hpp>
#include <components/sceneutil/skeleton.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>

#include <components/settings/settings.hpp>

#include <components/misc/constants.hpp>

#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/weapontype.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwrender/camera.hpp"
#include "../mwrender/renderingmanager.hpp"
#include "../mwrender/vismask.hpp"

namespace MWVR
{
    // Some weapon types, such as spellcast, are classified as melee even though they are not. At least not in the way i want.
    // All the false melee types have negative enum values, but also so does hand to hand.
    // I think this covers all cases
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

    /// Implements control of a finger by overriding rotation
    class FingerController : public osg::NodeCallback
    {
    public:
        FingerController() {};
        void setEnabled(bool enabled) { mEnabled = enabled; };
        void operator()(osg::Node* node, osg::NodeVisitor* nv);

    private:
        bool mEnabled = true;
    };

    void FingerController::operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        if (!mEnabled)
        {
            traverse(node, nv);
            return;
        }

        // Since morrowinds assets do not do a particularly good job of imitating natural hands and poses,
        // the best i can do for pointing is to just make it point straight forward. While not
        // totally natural, it works.
        osg::Quat rotate{ 0,0,0,1 };
        auto matrixTransform = node->asTransform()->asMatrixTransform();
        auto matrix = matrixTransform->getMatrix();
        matrix.setRotate(rotate);
        matrixTransform->setMatrix(matrix);


        // Omit nested callbacks to override animations of this node
        osg::ref_ptr<osg::Callback> ncb = getNestedCallback();
        setNestedCallback(nullptr);
        traverse(node, nv);
        setNestedCallback(ncb);
    }

    /// Implements control of a finger by overriding rotation
    class HandController : public osg::NodeCallback
    {
    public:
        HandController() = default;
        void setEnabled(bool enabled) { mEnabled = enabled; };
        void operator()(osg::Node* node, osg::NodeVisitor* nv);

    private:
        bool mEnabled = true;
    };

    void HandController::operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        if (!mEnabled)
        {
            traverse(node, nv);
            return;
        }
        float PI_2 = osg::PI_2;
        if (node->getName() == "Bip01 L Hand")
            PI_2 = -PI_2;
        float PI_4 = PI_2 / 2.f;

        osg::Quat rotate{ 0,0,0,1 };
        auto* world = MWBase::Environment::get().getWorld();
        auto windowManager = MWBase::Environment::get().getWindowManager();
        auto animation = MWVR::Environment::get().getPlayerAnimation();
        auto weaponType = world->getActiveWeaponType();
        // Morrowind models do not hold most weapons at a natural angle, so i rotate the hand
        // to more natural angles on weapons to allow more comfortable combat.
        if (!windowManager->isGuiMode() && !animation->fingerPointingMode())
        {

            switch (weaponType)
            {
            case ESM::Weapon::None:
            case ESM::Weapon::HandToHand:
            case ESM::Weapon::MarksmanThrown:
            case ESM::Weapon::Spell:
            case ESM::Weapon::Arrow:
            case ESM::Weapon::Bolt:
                // No adjustment
                break;
            case ESM::Weapon::MarksmanCrossbow:
                // Crossbow points upwards. Assumedly because i am overriding hand animations.
                rotate = osg::Quat(PI_4 / 1.05, osg::Vec3{ 0,1,0 }) * osg::Quat(0.06, osg::Vec3{ 0,0,1 });
                break;
            case ESM::Weapon::MarksmanBow:
                // Bow points down by default, rotate it back up a little
                rotate = osg::Quat(-PI_2 * .10f, osg::Vec3{ 0,1,0 });
                break;
            default:
                // Melee weapons Need adjustment
                rotate = osg::Quat(PI_4, osg::Vec3{ 0,1,0 });
                break;
            }
        }

        auto matrixTransform = node->asTransform()->asMatrixTransform();
        auto matrix = matrixTransform->getMatrix();
        matrix.setRotate(rotate);
        matrixTransform->setMatrix(matrix);


        // Omit nested callbacks to override animations of this node
        osg::ref_ptr<osg::Callback> ncb = getNestedCallback();
        setNestedCallback(nullptr);
        traverse(node, nv);
        setNestedCallback(ncb);
    }

    /// Implements control of weapon direction
    class WeaponDirectionController : public osg::NodeCallback
    {
    public:
        WeaponDirectionController() = default;
        void setEnabled(bool enabled) { mEnabled = enabled; };
        void operator()(osg::Node* node, osg::NodeVisitor* nv);

    private:
        bool mEnabled = true;
    };

    void WeaponDirectionController::operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        if (!mEnabled)
        {
            traverse(node, nv);
            return;
        }

        // Arriving here implies a parent, no need to check
        auto parent = static_cast<osg::MatrixTransform*>(node->getParent(0));



        osg::Quat rotate{ 0,0,0,1 };
        auto* world = MWBase::Environment::get().getWorld();
        auto weaponType = world->getActiveWeaponType();
        switch (weaponType)
        {
        case ESM::Weapon::MarksmanThrown:
        case ESM::Weapon::Spell:
        case ESM::Weapon::Arrow:
        case ESM::Weapon::Bolt:
        case ESM::Weapon::HandToHand:
        case ESM::Weapon::MarksmanBow:
        case ESM::Weapon::MarksmanCrossbow:
            // Rotate to point straight forward, reverting any rotation of the hand to keep aim consistent.
            rotate = parent->getInverseMatrix().getRotate();
            rotate = osg::Quat(-osg::PI_2, osg::Vec3{ 0,0,1 }) * rotate;
            break;
        default:
            // Melee weapons point straight up from the hand
            rotate = osg::Quat(osg::PI_2, osg::Vec3{ 1,0,0 });
            break;
        }

        auto matrixTransform = node->asTransform()->asMatrixTransform();
        auto matrix = matrixTransform->getMatrix();
        matrix.setRotate(rotate);
        matrixTransform->setMatrix(matrix);

        traverse(node, nv);
    }

    /// Implements control of the weapon pointer
    class WeaponPointerController : public osg::NodeCallback
    {
    public:
        WeaponPointerController() = default;
        void setEnabled(bool enabled) { mEnabled = enabled; };
        void operator()(osg::Node* node, osg::NodeVisitor* nv);

    private:
        bool mEnabled = true;
    };

    void WeaponPointerController::operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        if (!mEnabled)
        {
            traverse(node, nv);
            return;
        }

        auto matrixTransform = node->asTransform()->asMatrixTransform();
        auto world = MWBase::Environment::get().getWorld();
        auto weaponType = world->getActiveWeaponType();
        auto windowManager = MWBase::Environment::get().getWindowManager();

        if (!isMeleeWeapon(weaponType) && !windowManager->isGuiMode())
        {
            // Ranged weapons should show a pointer to where they are targeting
            matrixTransform->setMatrix(
                osg::Matrix::scale(1.f, 64.f, 1.f)
            );
        }
        else
        {
            matrixTransform->setMatrix(
                osg::Matrix::scale(1.f, 64.f, 1.f)
            );
        }

        // First, update the base of the finger to the overriding orientation

        traverse(node, nv);
    }

    class TrackingController : public VRTrackingListener
    {
    public:
        TrackingController(VRPath trackingPath, osg::Vec3 baseOffset, osg::Quat baseOrientation)
            : mTrackingPath(trackingPath)
            , mTransform(nullptr)
            , mBaseOffset(baseOffset)
            , mBaseOrientation(baseOrientation)
        {

        }

        void onTrackingUpdated(VRTrackingSource& source, DisplayTime predictedDisplayTime) override
        {
            if (!mTransform)
                return;

            auto tp = source.getTrackingPose(predictedDisplayTime, mTrackingPath, 0);
            if (!tp.status)
                return;

            auto orientation = mBaseOrientation * tp.pose.orientation;

            // Undo the wrist translate
            // TODO: I'm sure this could bee a lot less hacky
            // But i'll defer that to whenever we get inverse cinematics so i can track the hand directly.
            auto* hand = mTransform->getChild(0);
            auto handMatrix = hand->asTransform()->asMatrixTransform()->getMatrix();
            auto position = tp.pose.position - (orientation * handMatrix.getTrans());

            // Center hand mesh on tracking
            // This is just an estimate from trial and error, any suggestion for improving this is welcome
            position -= orientation * mBaseOffset;

            // Get current world transform of limb
            osg::Matrix worldToLimb = osg::computeWorldToLocal(mTransform->getParentalNodePaths()[0]);
            // Get current world of the reference node
            osg::Matrix worldReference = osg::Matrix::identity();
            // New transform based on tracking.
            worldReference.preMultTranslate(position);
            worldReference.preMultRotate(orientation);

            // Finally, set transform
            mTransform->setMatrix(worldReference * worldToLimb * mTransform->getMatrix());
        }

        void setTransform(osg::MatrixTransform* transform)
        {
            mTransform = transform;
        }

        VRPath mTrackingPath;
        osg::ref_ptr<osg::MatrixTransform> mTransform;
        osg::Vec3 mBaseOffset;
        osg::Quat mBaseOrientation;
    };

    VRAnimation::VRAnimation(
        const MWWorld::Ptr& ptr, osg::ref_ptr<osg::Group> parentNode, Resource::ResourceSystem* resourceSystem,
        bool disableSounds, std::shared_ptr<UserPointer> userPointer)
        // Note that i let it construct as 3rd person and then later update it to VM_VRFirstPerson
        // when the character controller updates
        : MWRender::NpcAnimation(ptr, parentNode, resourceSystem, disableSounds, VM_Normal, 55.f)
        , mIndexFingerControllers{ nullptr, nullptr }
        // The player model needs to be pushed back a little to make sure the player's view point is naturally protruding 
        // Pushing the camera forward instead would produce an unnatural extra movement when rotating the player model.
        , mModelOffset(new osg::MatrixTransform(osg::Matrix::translate(osg::Vec3(0, -15, 0))))
        , mUserPointer(userPointer)
    {
        for (int i = 0; i < 2; i++)
        {
            mIndexFingerControllers[i] = new FingerController;
            mHandControllers[i] = new HandController;
        }

        mWeaponDirectionTransform = new osg::MatrixTransform();
        mWeaponDirectionTransform->setName("Weapon Direction");
        mWeaponDirectionTransform->setUpdateCallback(new WeaponDirectionController);

        mModelOffset->setName("ModelOffset");

        mWeaponPointerTransform = new osg::MatrixTransform();
        mWeaponPointerTransform->setMatrix(
            osg::Matrix::scale(0.f, 0.f, 0.f)
        );
        mWeaponPointerTransform->setName("Weapon Pointer");
        mWeaponPointerTransform->setUpdateCallback(new WeaponPointerController);
        //mWeaponDirectionTransform->addChild(mWeaponPointerTransform);

        auto vrTrackingManager = MWVR::Environment::get().getTrackingManager();
        vrTrackingManager->bind(this, "pcworld");
        auto* source = static_cast<VRTrackingToWorldBinding*>(vrTrackingManager->getSource("pcworld"));
        source->setOriginNode(mObjectRoot->getParent(0));

        // Morrowind's meshes do not point forward by default and need re-positioning and orientation.
        float VRbias = osg::DegreesToRadians(-90.f);
        osg::Quat yaw(-VRbias, osg::Vec3f(0, 0, 1));
        osg::Quat roll(2 * VRbias, osg::Vec3f(1, 0, 0));
        osg::Vec3 offset{ 15,0,0 };
        auto* tm = Environment::get().getTrackingManager();

        // Note that these controllers could be bound directly to source in the tracking manager.
        // Instead we store them and update them manually to ensure order of operations.
        {
            auto path = tm->stringToVRPath("/user/hand/right/input/aim/pose");
            auto orientation = yaw;
            mVrControllers.emplace("bip01 r forearm", std::make_unique<TrackingController>(path, offset, orientation));
        }

        {
            auto path = tm->stringToVRPath("/user/hand/left/input/aim/pose");
            auto orientation = roll * yaw;
            mVrControllers.emplace("bip01 l forearm", std::make_unique<TrackingController>(path, offset, orientation));
        }
    }

    VRAnimation::~VRAnimation() {};

    void VRAnimation::setViewMode(NpcAnimation::ViewMode viewMode)
    {
        if (viewMode != VM_VRFirstPerson && viewMode != VM_VRNormal)
        {
            Log(Debug::Warning) << "Attempted to set view mode of VRAnimation to non-vr mode. Defaulted to VM_VRFirstPerson.";
            viewMode = VM_VRFirstPerson;
        }
        NpcAnimation::setViewMode(viewMode);
        return;
    }

    void VRAnimation::updateParts()
    {
        NpcAnimation::updateParts();

        if (mViewMode == VM_VRFirstPerson)
        {
            // Hide everything other than hands
            removeIndividualPart(ESM::PartReferenceType::PRT_Hair);
            removeIndividualPart(ESM::PartReferenceType::PRT_Head);
            removeIndividualPart(ESM::PartReferenceType::PRT_LForearm);
            removeIndividualPart(ESM::PartReferenceType::PRT_LUpperarm);
            removeIndividualPart(ESM::PartReferenceType::PRT_LWrist);
            removeIndividualPart(ESM::PartReferenceType::PRT_RForearm);
            removeIndividualPart(ESM::PartReferenceType::PRT_RUpperarm);
            removeIndividualPart(ESM::PartReferenceType::PRT_RWrist);
            removeIndividualPart(ESM::PartReferenceType::PRT_Cuirass);
            removeIndividualPart(ESM::PartReferenceType::PRT_Groin);
            removeIndividualPart(ESM::PartReferenceType::PRT_Neck);
            removeIndividualPart(ESM::PartReferenceType::PRT_Skirt);
            removeIndividualPart(ESM::PartReferenceType::PRT_Tail);
            removeIndividualPart(ESM::PartReferenceType::PRT_LLeg);
            removeIndividualPart(ESM::PartReferenceType::PRT_RLeg);
            removeIndividualPart(ESM::PartReferenceType::PRT_LAnkle);
            removeIndividualPart(ESM::PartReferenceType::PRT_RAnkle);
            removeIndividualPart(ESM::PartReferenceType::PRT_LKnee);
            removeIndividualPart(ESM::PartReferenceType::PRT_RKnee);
            removeIndividualPart(ESM::PartReferenceType::PRT_LFoot);
            removeIndividualPart(ESM::PartReferenceType::PRT_RFoot);
            removeIndividualPart(ESM::PartReferenceType::PRT_LPauldron);
            removeIndividualPart(ESM::PartReferenceType::PRT_RPauldron);
        }
        else
        {
            removeIndividualPart(ESM::PartReferenceType::PRT_LForearm);
            removeIndividualPart(ESM::PartReferenceType::PRT_LWrist);
            removeIndividualPart(ESM::PartReferenceType::PRT_RForearm);
            removeIndividualPart(ESM::PartReferenceType::PRT_RWrist);
        }


        auto playerPtr = MWMechanics::getPlayer();
        const MWWorld::LiveCellRef<ESM::NPC>* ref = playerPtr.get<ESM::NPC>();
        const ESM::Race* race =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(ref->mBase->mRace);
        bool isMale = ref->mBase->isMale();
        float charHeightFactor = isMale ? race->mData.mHeight.mMale : race->mData.mHeight.mFemale;
        float charHeightBase = 1.8288f; // Is this ~ the right value?
        float charHeight = charHeightBase * charHeightFactor;
        float realHeight = Settings::Manager::getFloat("real height", "VR");
        float sizeFactor = charHeight / realHeight;
        Environment::get().getSession()->setPlayerScale(sizeFactor);
        Environment::get().getSession()->setEyeLevel(charHeightBase - 0.15f); // approximation
    }

    void VRAnimation::setFingerPointingMode(bool enabled)
    {
        if (enabled == mFingerPointingMode)
            return;

        auto finger = mNodeMap.find("bip01 r finger1");
        if (finger != mNodeMap.end())
        {
            auto base_joint = finger->second;
            auto second_joint = base_joint->getChild(0)->asTransform()->asMatrixTransform();
            assert(second_joint);

            base_joint->removeUpdateCallback(mIndexFingerControllers[0]);
            second_joint->removeUpdateCallback(mIndexFingerControllers[1]);
            if (enabled)
            {
                base_joint->addUpdateCallback(mIndexFingerControllers[0]);
                second_joint->addUpdateCallback(mIndexFingerControllers[1]);

            }
        }

        mUserPointer->setEnabled(enabled);

        mFingerPointingMode = enabled;
    }

    float VRAnimation::getVelocity(const std::string& groupname) const
    {
        return 0.0f;
    }

    void VRAnimation::onTrackingUpdated(VRTrackingSource& source, DisplayTime predictedDisplayTime)
    {
        for (auto& controller : mVrControllers)
            controller.second->onTrackingUpdated(source, predictedDisplayTime);

        if (mSkeleton)
            mSkeleton->markBoneMatriceDirty();

        mUserPointer->updatePointerTarget();
    }

    osg::Vec3f VRAnimation::runAnimation(float timepassed)
    {
        return NpcAnimation::runAnimation(timepassed);
    }

    void VRAnimation::addControllers()
    {
        NpcAnimation::addControllers();

        for (int i = 0; i < 2; ++i)
        {
            auto forearm = mNodeMap.find(i == 0 ? "bip01 l forearm" : "bip01 r forearm");
            if (forearm != mNodeMap.end())
            {
                auto controller = mVrControllers.find(forearm->first);
                if (controller != mVrControllers.end())
                {
                    controller->second->setTransform(forearm->second);
                }
            }

            auto hand = mNodeMap.find(i == 0 ? "bip01 l hand" : "bip01 r hand");
            if (hand != mNodeMap.end())
            {
                auto node = hand->second;
                node->removeUpdateCallback(mHandControllers[i]);
                node->addUpdateCallback(mHandControllers[i]);
            }
        }

        auto hand = mNodeMap.find("bip01 r hand");
        if (hand != mNodeMap.end())
        {
            hand->second->removeChild(mWeaponDirectionTransform);
            hand->second->addChild(mWeaponDirectionTransform);
        }
        auto finger = mNodeMap.find("bip01 r finger11");
        if (finger != mNodeMap.end())
        {
            mUserPointer->setParent(finger->second);
        }
        mSkeleton->setIsTracked(true);
    }
    void VRAnimation::enableHeadAnimation(bool)
    {
        NpcAnimation::enableHeadAnimation(false);
    }
    void VRAnimation::setAccurateAiming(bool)
    {
        NpcAnimation::setAccurateAiming(false);
    }

    osg::Matrix VRAnimation::getWeaponTransformMatrix() const
    {
        return osg::computeLocalToWorld(mWeaponDirectionTransform->getParentalNodePaths()[0]);
    }
}
