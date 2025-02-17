#include "openxrinput.hpp"
#include "vranimation.hpp"
#include "vrgui.hpp"
#include "vrpointer.hpp"

#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/Drawable>
#include <osg/Fog>
#include <osg/LightModel>
#include <osg/MatrixTransform>
#include <osg/Object>
#include <osg/PositionAttitudeTransform>

#include <components/debug/debuglog.hpp>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/sceneutil/shadow.hpp>
#include <components/sceneutil/skeleton.hpp>

#include <components/settings/settings.hpp>

#include <components/esm3/loadrace.hpp>
#include <components/esm3/loadench.hpp>

#include <components/misc/constants.hpp>

#include <components/vr/session.hpp>
#include <components/vr/trackingmanager.hpp>
#include <components/vr/trackingtransform.hpp>
#include <components/vr/vr.hpp>
#include <components/xr/session.hpp>

#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/weapontype.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwrender/camera.hpp"
#include "../mwrender/renderingmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"

#include "vrpointer.hpp"
#include "vrtracking.hpp"

namespace MWVR
{
    // Some weapon types, such as spellcast, are classified as melee even though they are not. At least not in the way i
    // want. All the false melee types have negative enum values, but also so does hand to hand. I think this covers all
    // cases
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
        FingerController(){}
        void setEnabled(bool enabled) { mEnabled = enabled; }
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
        osg::Quat rotate{ 0, 0, 0, 1 };
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
        void setEnabled(bool enabled) { mEnabled = enabled; }
        void setFingerPointingMode(bool fingerPointingMode) { mFingerPointingMode = fingerPointingMode; }
        void operator()(osg::Node* node, osg::NodeVisitor* nv);

    private:
        bool mEnabled = true;
        bool mFingerPointingMode = false;
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

        osg::Quat rotate{ 0, 0, 0, 1 };
        auto windowManager = MWBase::Environment::get().getWindowManager();
        auto weaponType = MWBase::Environment::get().getWorld()->getActiveWeaponType();
        // Morrowind models do not hold most weapons at a natural angle, so i rotate the hand
        // to more natural angles on weapons to allow more comfortable combat.
        if (!windowManager->isGuiMode() && !mFingerPointingMode)
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
                    rotate = osg::Quat(PI_4 / 1.05, osg::Vec3{ 0, 1, 0 }) * osg::Quat(0.06, osg::Vec3{ 0, 0, 1 });
                    break;
                case ESM::Weapon::MarksmanBow:
                    // Bow points down by default, rotate it back up a little
                    rotate = osg::Quat(-PI_2 * .10f, osg::Vec3{ 0, 1, 0 });
                    break;
                default:
                    // Melee weapons Need adjustment
                    rotate = osg::Quat(PI_4, osg::Vec3{ 0, 1, 0 });
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
        void setEnabled(bool enabled) { mEnabled = enabled; }
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

        osg::Quat rotate{ 0, 0, 0, 1 };
        auto weaponType = MWBase::Environment::get().getWorld()->getActiveWeaponType();
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
                rotate = osg::Quat(-osg::PI_2, osg::Vec3{ 0, 0, 1 }) * rotate;
                break;
            default:
                // Melee weapons point straight up from the hand
                rotate = osg::Quat(osg::PI_2, osg::Vec3{ 1, 0, 0 });
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
        void setEnabled(bool enabled) { mEnabled = enabled; }
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
            matrixTransform->setMatrix(osg::Matrix::scale(1.f, 64.f, 1.f));
        }
        else
        {
            matrixTransform->setMatrix(osg::Matrix::scale(1.f, 64.f, 1.f));
        }

        // First, update the base of the finger to the overriding orientation

        traverse(node, nv);
    }

    // class TrackingController
    //{
    // public:
    //     TrackingController(VR::VRPath trackingPath, VR::VRPath topLevelPath, osg::Vec3 baseOffset, bool left)
    //         : mTrackingPath(trackingPath)
    //         , mTopLevelPath(topLevelPath)
    //         , mTransform(nullptr)
    //         , mBaseOffset(baseOffset)
    //         , mBaseOrientation(osg::PI_2, osg::Vec3f(0, 0, 1))
    //         , mLeft(left)
    //     {
    //         if (left)
    //             mBaseOrientation = osg::Quat(osg::PI, osg::Vec3f(1, 0, 0)) * mBaseOrientation;
    //     }

    //    void onTrackingUpdated(VR::TrackingManager& manager)
    //    {
    //        if (!mTransform)
    //            return;

    //        auto tp = manager.locate(mTrackingPath);
    //        if (!tp.status)
    //            return;

    //        if (!VR::Session::instance().getInteractionProfileActive(mTopLevelPath))
    //            return;

    //        auto orientation = (mBaseOrientation)*tp.pose.orientation;

    //        // Undo the wrist translate
    //        // TODO: I'm sure this could bee a lot less hacky
    //        // But i'll defer that to whenever we get inverse cinematics so i can track the hand directly.
    //        auto* hand = mTransform->getChild(0);
    //        auto handMatrix = hand->asTransform()->asMatrixTransform()->getMatrix();
    //        auto position = tp.pose.position.asMWUnits() - (orientation * handMatrix.getTrans());

    //        // Center hand mesh on tracking
    //        // This is just an estimate from trial and error, any suggestion for improving this is welcome
    //        position -= orientation * mBaseOffset;
    //        auto offset = VR::Session::instance().getHandsOffset();
    //        if (mLeft)
    //            offset.x() = -offset.x();
    //        position += tp.pose.orientation * offset;

    //        // Get current world transform of limb
    //        osg::Matrix worldToLimb = osg::computeWorldToLocal(mTransform->getParentalNodePaths()[0]);
    //        // Get current world of the reference node
    //        osg::Matrix worldReference = osg::Matrix::identity();
    //        // New transform based on tracking.
    //        worldReference.preMultTranslate(position);
    //        worldReference.preMultRotate(orientation);

    //        // Finally, set transform
    //        mTransform->setMatrix(worldReference * worldToLimb * mTransform->getMatrix());
    //    }

    //    void setTransform(osg::MatrixTransform* transform) { mTransform = transform; }

    //    VR::VRPath mTrackingPath;
    //    VR::VRPath mTopLevelPath;
    //    osg::ref_ptr<osg::MatrixTransform> mTransform;
    //    osg::Vec3 mBaseOffset;
    //    osg::Quat mBaseOrientation;
    //    bool mLeft;
    //};

    class TrackingController
    {
    public:
        TrackingController(std::shared_ptr<VR::Space> space, Stereo::Pose pose)
            : mSpace(space)
            , mPose(pose)
        {
        }

        void update(osg::MatrixTransform& transform)
        {
            auto tp = mSpace->locateInWorld();
            if (!tp.status)
                return;
            auto pose = tp.pose + mPose;

            // Undo the wrist translate
            // TODO: I'm sure this could bee a lot less hacky
            // But i'll defer that to whenever we get inverse cinematics so i can track the hand directly.
            //auto* hand = mTransform->getChild(0);
            //auto handMatrix = hand->asTransform()->asMatrixTransform()->getMatrix();
            //auto position = tp.pose.position.asMWUnits() - (orientation * handMatrix.getTrans());
            //auto position = pose.position.asMWUnits();

            // Center hand mesh on tracking
            // This is just an estimate from trial and error, any suggestion for improving this is welcome
            //position -= orientation * mBaseOffset;
            //auto offset = VR::Session::instance().getHandsOffset();
            //if (mLeft)
            //    offset.x() = -offset.x();
            //position += tp.pose.orientation * offset;

            // Transform back to local
            osg::Matrix worldToLocal = osg::computeWorldToLocal(transform.getParentalNodePaths()[0]);
            // Get current world of the reference node
            osg::Matrix spaceToWorld = osg::Matrix::identity();
            // New transform based on tracking.
            spaceToWorld.preMultTranslate(pose.position.asMWUnits());
            spaceToWorld.preMultRotate(pose.orientation);

            // Finally, set transform
            transform.setMatrix(spaceToWorld * worldToLocal * transform.getMatrix());
            // TODO: Wouldn't this work just as well? If so that could help with fp accuracy
            // Might need to disable culling for this though
            //transform.setReferenceFrame(osg::Transform::ABSOLUTE_RF);
            //transform.setmatrix(spaceToWorld)
        }

        std::shared_ptr<VR::Space> mSpace;
        Stereo::Pose mPose;
    };

    VRAnimation::VRAnimation(const MWWorld::Ptr& ptr, osg::ref_ptr<osg::Group> parentNode,
        Resource::ResourceSystem* resourceSystem, bool disableSounds, osg::ref_ptr<osg::Group> sceneRoot)
        // Note that i let it construct as 3rd person and then later update it to VM_VRFirstPerson
        // when the character controller updates
        : MWRender::NpcAnimation(ptr, parentNode, resourceSystem, disableSounds, VM_Normal, 55.f)
        , mLeftIndexFingerControllers{ nullptr, nullptr }
        , mRightIndexFingerControllers{ nullptr, nullptr }
        // The player model needs to be pushed back a little to make sure the player's view point is naturally
        // protruding Pushing the camera forward instead would produce an unnatural extra movement when rotating the
        // player model.
        , mModelOffset(new osg::MatrixTransform(osg::Matrix::translate(osg::Vec3(0, -15, 0))))
        , mCrosshairsEnabled(false)
        , mSceneRoot(sceneRoot)
    {
        //mReferenceSpace = XR::Session::instance().getReferenceSpace(XR_REFERENCE_SPACE_TYPE_LOCAL);

        for (int i = 0; i < 2; i++)
        {
            mHandControllers[i] = new HandController;
            mLeftIndexFingerControllers[i] = new FingerController;
            mRightIndexFingerControllers[i] = new FingerController;
        }

        mWeaponDirectionTransform = new osg::MatrixTransform();
        mWeaponDirectionTransform->setName("Weapon Direction");
        mWeaponDirectionTransform->setUpdateCallback(new WeaponDirectionController);

        mModelOffset->setName("ModelOffset");

        mWeaponPointerTransform = new osg::MatrixTransform();
        mWeaponPointerTransform->setMatrix(osg::Matrix::scale(0.f, 0.f, 0.f));
        mWeaponPointerTransform->setName("Weapon Pointer");
        mWeaponPointerTransform->setUpdateCallback(new WeaponPointerController);
        // mWeaponDirectionTransform->addChild(mWeaponPointerTransform);
        
        //auto worldPath = VR::stringToVRPath("/world/user");
        //auto* stageToWorldBinding
        //    = static_cast<VR::StageToWorldBinding*>(VR::TrackingManager::instance().getTrackingSource(worldPath));
        //stageToWorldBinding->setOriginNode(mObjectRoot->getParent(0));

        // Morrowind's meshes do not point forward by default and need re-positioning and orientation.
        //osg::Vec3 offset{ 15, 0, 0 };

        //{
        //    auto topLevelPath = VR::stringToVRPath("/user/hand/right");
        //    auto path = VR::stringToVRPath("/world/user/hand/right/input/aim/pose");
        //    mVrControllers.emplace(
        //        "bip01 r forearm", std::make_unique<TrackingController>(path, topLevelPath, offset, false));
        //}

        //{
        //    auto topLevelPath = VR::stringToVRPath("/user/hand/left");
        //    auto path = VR::stringToVRPath("/world/user/hand/left/input/aim/pose");
        //    mVrControllers.emplace(
        //        "bip01 l forearm", std::make_unique<TrackingController>(path, topLevelPath, offset, true));
        //}
    }

    VRAnimation::~VRAnimation() {}

    void VRAnimation::setViewMode(NpcAnimation::ViewMode viewMode)
    {
        if (viewMode != VM_VRFirstPerson && viewMode != VM_VRNormal)
        {
            Log(Debug::Warning)
                << "Attempted to set view mode of VRAnimation to non-vr mode. Defaulted to VM_VRFirstPerson.";
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

        updateCharHeight();
    }

    //void VRAnimation::onTrackingUpdated(VR::TrackingManager& manager)
    //{
    //    if (mSkeleton)
    //        mSkeleton->markBoneMatriceDirty();

    //    auto tp = manager.locate(mWorldHeadPath);

    //    if (!!tp.status)
    //    {
    //        auto world = MWBase::Environment::get().getWorld();

    //        auto& player = world->getPlayer();
    //        auto playerPtr = player.getPlayer();

    //        float headYaw = 0.f;
    //        float headPitch = 0.f;
    //        float headRoll = 0.f;
    //        Stereo::getEulerAngles(tp.pose.orientation, headYaw, headPitch, headRoll);

    //        world->rotateObject(playerPtr, osg::Vec3f(headPitch, 0.f, headYaw), MWBase::RotationFlag_none);

    //        osg::Vec3 rotation = {
    //            0.,
    //            0.,
    //            0.,
    //        };

    //        if (VR::Session::instance().handDirectedMovement())
    //        {
    //            tp = manager.locate(VR::stringToVRPath("/world/user/hand/left/input/aim/pose"));
    //            float handYaw = 0.f;
    //            float handPitch = 0.f;
    //            float handRoll = 0.f;

    //            Stereo::getEulerAngles(tp.pose.orientation, handYaw, handPitch, handRoll);

    //            rotation = {
    //                (handPitch - headPitch),
    //                0.,
    //                (handYaw - headYaw),
    //            };
    //        }

    //        VR::Session::instance().setMovementAngleOffset(rotation);
    //    }

    //    for (auto& controller : mVrControllers)
    //        controller.second->onTrackingUpdated(manager);
    //}

    void VRAnimation::updateCrosshairs()
    {
        if (!mCrosshairsEnabled)
            return;

        mCrosshairAmmo->hide();
        mCrosshairSpell->hide();
        mCrosshairThrown->hide();

        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
            return;

        if (VR::getKBMouseModeActive())
        {
        }

        if (isArrowAttached())
        {
            if (VR::getKBMouseModeActive())
            {
                mCrosshairAmmo->setParent(mKBMouseCrosshairTransform);
                mCrosshairAmmo->setOffset(100.f);
                mCrosshairAmmo->setStretch(200.f);
            }
            else
            {
                mCrosshairAmmo->setParent(getArrowBone());
                mCrosshairAmmo->setStretch(100.f);
                mCrosshairAmmo->setOffset(15.f);
            }
            mCrosshairAmmo->show();
        }
        else
        {
            mCrosshairAmmo->hide();
            mCrosshairAmmo->setParent(nullptr);
        }

        const MWWorld::Class& cls = mPtr.getClass();
        MWWorld::InventoryStore& inv = cls.getInventoryStore(mPtr);
        MWMechanics::CreatureStats& stats = cls.getCreatureStats(mPtr);

        mCrosshairSpell->hide();
        mCrosshairSpell->setParent(nullptr);
        mCrosshairThrown->hide();
        mCrosshairThrown->setParent(nullptr);

        if (stats.getDrawState() == MWMechanics::DrawState::Spell)
        {
            auto selectedSpell = MWBase::Environment::get().getWindowManager()->getSelectedSpell();
            ;
            auto world = MWBase::Environment::get().getWorld();
            bool isMagicItem = false;

            if (selectedSpell.empty())
            {
                if (inv.getSelectedEnchantItem() != inv.end())
                {
                    const MWWorld::Ptr& enchantItem = *inv.getSelectedEnchantItem();
                    selectedSpell = enchantItem.getClass().getEnchantment(enchantItem);
                    isMagicItem = true;
                }
            }

            const MWWorld::ESMStore& store = world->getStore();
            const ESM::EffectList* effectList = nullptr;
            if (isMagicItem)
            {
                const ESM::Enchantment* enchantment = store.get<ESM::Enchantment>().find(selectedSpell);
                if (enchantment)
                    effectList = &enchantment->mEffects;
            }
            else
            {
                const ESM::Spell* spell = store.get<ESM::Spell>().find(selectedSpell);
                if (spell)
                    effectList = &spell->mEffects;
            }

            int rangeType = ESM::RT_Self;
            if (effectList)
            {
                for (auto& effect : effectList->mList)
                {
                    rangeType = std::max(rangeType, effect.mData.mRange);
                }
            }

            if (rangeType > 0)
            {
                if (VR::getKBMouseModeActive())
                {
                    mCrosshairSpell->setParent(mKBMouseCrosshairTransform);
                    mCrosshairSpell->setOffset(100.f);
                    mCrosshairSpell->setStretch(200.f);
                }
                else
                {
                    mCrosshairSpell->setParent(mWeaponDirectionTransform);
                    if (rangeType == 1)
                        mCrosshairSpell->setStretch(25.f);
                    else if (rangeType == 2)
                        mCrosshairSpell->setStretch(100.f);
                    mCrosshairSpell->setOffset(15.f);
                }
                mCrosshairSpell->show();
            }
        }
        else if (stats.getDrawState() == MWMechanics::DrawState::Weapon)
        {
            // TODO: Should probably create an accessor for Slot_CarriedRight's WeaponType so this verbose code
            // doens't have to be repeated everywhere.
            MWWorld::ConstContainerStoreIterator weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
            if (weapon != inv.end() && weapon->getType() == ESM::Weapon::sRecordId)
            {
                int type = weapon->get<ESM::Weapon>()->mBase->mData.mType;
                ESM::WeaponType::Class weapclass = MWMechanics::getWeaponType(type)->mWeaponClass;
                if (weapclass == ESM::WeaponType::Thrown)
                {
                    if (VR::getKBMouseModeActive())
                    {
                        mCrosshairThrown->setParent(mKBMouseCrosshairTransform);
                        mCrosshairThrown->setOffset(100.f);
                        mCrosshairThrown->setStretch(200.f);
                    }
                    else
                    {
                        mCrosshairThrown->setParent(mWeaponDirectionTransform);
                        mCrosshairThrown->setStretch(100.f);
                        mCrosshairThrown->setOffset(15.f);
                    }
                    mCrosshairThrown->show();
                }
            }
        }
    }

    void VRAnimation::updateCharHeight() 
    {
        // Compute an approximate character height (eye level)
        //auto playerPtr = MWMechanics::getPlayer();
        //const MWWorld::LiveCellRef<ESM::NPC>* ref = playerPtr.get<ESM::NPC>();
        //const ESM::Race* race
        //    = MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(ref->mBase->mRace);
        //bool isMale = ref->mBase->isMale();
        //float charHeightFactor = isMale ? race->mData.mMaleHeight : race->mData.mFemaleHeight;
        //auto charHeightBase
        //    = Stereo::Unit::fromMeters(Settings::Manager::getFloat("character base height", "VR Debug"));
        //auto charHeight = charHeightBase * charHeightFactor;
        //VR::Session::instance().setCharHeight(charHeight);
        //Log(Debug::Verbose) << "Calculated character height: " << charHeight.asMeters();

        // Use the actual animation to directly get the character's height
        auto root = mObjectRoot;
        auto head = getNode("Bip01 Head");
        if (root && head)
        {
            // The head geometry needs to exist to find a rough estimate of the eye line.
            if (mPartPriorities[ESM::PRT_Head] < 1 && !mHeadModel.empty())
                addOrReplaceIndividualPart(ESM::PRT_Head, -1, 1, mHeadModel);
            if (head->computeBound().radius() > 1.f)
            {
                auto path = head->getParentalNodePaths(root)[0];
                auto m = osg::computeLocalToWorld(path);
                mCharHeight = m.getTrans().z() + head->computeBound().radius();
            }
            removeIndividualPart(ESM::PRT_Head);
        }

        recenter();
    }

    void VRAnimation::onFrameUpdate(VR::Frame&) {
        updateLocalSpaceWorldPose();
        for (auto& it : mVrControllers)
        {
            if (auto* bone = getBoneByName(it.first))
            {
                it.second->update(*bone->asTransform()->asMatrixTransform());
            }
        }
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
            //auto forearm = mNodeMap.find(i == 0 ? "Bip01 L Forearm" : "Bip01 R Forearm");
            //if (forearm != mNodeMap.end())
            //{
            //    auto controller = mVrControllers.find(forearm->first);
            //    if (controller != mVrControllers.end())
            //    {
            //        controller->second->setTransform(forearm->second);
            //    }
            //}

            auto hand = mNodeMap.find(i == 0 ? "Bip01 L Hand" : "Bip01 R Hand");
            if (hand != mNodeMap.end())
            {
                auto node = hand->second;
                node->removeUpdateCallback(mHandControllers[i]);
                node->addUpdateCallback(mHandControllers[i]);
            }

            auto finger1 = mNodeMap.find(i == 0 ? "bip01 l finger1" : "bip01 r finger1");
            auto* indexFingerControllers = i == 0 ? mLeftIndexFingerControllers : mRightIndexFingerControllers;

            if (finger1 != mNodeMap.end())
            {
                auto& base_joint = finger1->second;
                auto second_joint = base_joint->getChild(0)->asTransform()->asMatrixTransform();
                assert(second_joint);

                base_joint->removeUpdateCallback(indexFingerControllers[0]);
                base_joint->addUpdateCallback(indexFingerControllers[0]);
                second_joint->removeUpdateCallback(indexFingerControllers[1]);
                second_joint->addUpdateCallback(indexFingerControllers[1]);
            }
        }

        // TODO: Allow left-hand weapons?
        auto hand = mNodeMap.find("Bip01 R Hand");
        if (hand != mNodeMap.end())
        {
            hand->second->removeChild(mWeaponDirectionTransform);
            hand->second->addChild(mWeaponDirectionTransform);
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

    void VRAnimation::enablePointers(bool left, bool right)
    {
        mHandControllers[0]->setFingerPointingMode(left);
        mHandControllers[1]->setFingerPointingMode(right);
        mLeftIndexFingerControllers[0]->setEnabled(left);
        mLeftIndexFingerControllers[1]->setEnabled(left);
        mRightIndexFingerControllers[0]->setEnabled(right);
        mRightIndexFingerControllers[1]->setEnabled(right);
    }

    void VRAnimation::setEnableCrosshairs(bool enable)
    {
        if (enable == mCrosshairsEnabled)
            return;

        mCrosshairsEnabled = enable;

        if (enable)
        {
            mCrosshairAmmo = std::make_unique<Crosshair>(nullptr, osg::Vec3f(0.66f, 1.f, 0.66f), 0.1f, 0.40f, false);
            mCrosshairAmmo->setStretch(100.f);
            mCrosshairAmmo->setWidth(0.1f);
            mCrosshairAmmo->setOffset(15.f);
            mCrosshairAmmo->show();
            mCrosshairThrown = std::make_unique<Crosshair>(nullptr, osg::Vec3f(0.66f, 0.66f, 1.f), 0.1f, 0.40f, false);
            mCrosshairThrown->setStretch(100.f);
            mCrosshairThrown->setWidth(0.1f);
            mCrosshairThrown->setOffset(15.f);
            mCrosshairThrown->show();
            mCrosshairSpell = std::make_unique<Crosshair>(nullptr, osg::Vec3f(1.f, 1.f, 1.f), 0.1f, 0.40f, false);
            mCrosshairSpell->setStretch(100.f);
            mCrosshairSpell->setWidth(0.1f);
            mCrosshairSpell->setOffset(15.f);
            mCrosshairSpell->show();

            mKBMouseCrosshairTransform = new VR::SpaceTransform(OpenXRInput::instance().getSpace(OpenXRInput::DefaultReferenceSpaceView));
            mKBMouseCrosshairTransform->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
            mSceneRoot->addChild(mKBMouseCrosshairTransform);
        }
        else
        {
            mCrosshairAmmo = nullptr;
            mCrosshairThrown = nullptr;
            mCrosshairSpell = nullptr;

            mSceneRoot->removeChild(mKBMouseCrosshairTransform);
            mKBMouseCrosshairTransform = nullptr;
        }
    }

    void VRAnimation::updateLocalSpaceWorldPose()
    {
        auto origin = mObjectRoot->getParent(0);
        auto worldMatrix = osg::computeLocalToWorld(origin->getParentalNodePaths()[0]);
        auto trans = worldMatrix.getTrans();
        trans.z() += mCharHeight;
        auto localRef = OpenXRInput::instance().getSpace(OpenXRInput::DefaultReferenceSpaceLocal);
        Stereo::Pose originPose = { Stereo::Position::fromMWUnits(trans), worldMatrix.getRotate() };
        XR::Session::instance().setReferenceWorldPose(originPose + mCharLocalSpacePose, localRef);
    }

    void VRAnimation::recenter()
    {
        //Recompute mCharLocalSpacePose so that view->locateInWorld() = mObjectRoot's pose + mCharHeight
        auto localRef = OpenXRInput::instance().getSpace(OpenXRInput::DefaultReferenceSpaceLocal);
        auto viewRef = OpenXRInput::instance().getSpace(OpenXRInput::DefaultReferenceSpaceView);
        auto pose = localRef->locate(viewRef).pose;
        float yaw = 0.f;
        float pitch = 0.f;
        float roll = 0.f;
        Stereo::getEulerAngles(pose.orientation, yaw, pitch, roll);
        pose.orientation = osg::Quat(yaw, osg::Vec3d(0, 0, 1));
        mCharLocalSpacePose = pose;
    }

    void VRAnimation::attachBoneToSpace(const std::string& bone, std::shared_ptr<VR::Space> space, Stereo::Pose pose)
    {
        mVrControllers[bone] = std::make_unique<TrackingController>(space, pose);
    }

    void VRAnimation::addAnimSource(std::string_view model, const std::string& baseModel) 
    {
        Animation::addAnimSource(model, baseModel);
        updateCharHeight();
    }

    //void VRAnimation::attachReferenceSpaceToBone(XrSpace space, const std::string& bone) 
    //{
    //}
}
