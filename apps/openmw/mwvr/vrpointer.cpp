#include "vrpointer.hpp"
#include "vrgui.hpp"
#include "vrutil.hpp"

#include <osg/BlendFunc>
#include <osg/Drawable>
#include <osg/Fog>
#include <osg/LightModel>
#include <osg/Material>
#include <osg/MatrixTransform>

#include <components/debug/debuglog.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>

#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/sceneutil/shadow.hpp>

#include <components/vr/trackingmanager.hpp>
#include <components/vr/viewer.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwrender/renderingmanager.hpp"
#include "../mwrender/vismask.hpp"

#include "../mwgui/draganddrop.hpp"
#include "../mwgui/inventorywindow.hpp"
#include "../mwgui/itemmodel.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/security.hpp"

#include "../mwworld/action.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"

namespace MWVR
{
    /**
     * Makes it possible to use ItemModel::moveItem to move an item from an inventory to the world.
     */
    class DropItemAtPointModel : public MWGui::ItemModel
    {
    public:
        DropItemAtPointModel(UserPointer* pointer)
            : mVRPointer(pointer)
        {
        }
        ~DropItemAtPointModel() {}

        MWWorld::Ptr dropItemImpl(const MWGui::ItemStack& item, size_t count, bool copy)
        {
            MWBase::World* world = MWBase::Environment::get().getWorld();

            MWWorld::Ptr dropped;
            if (mVRPointer->canPlaceObject())
                dropped = world->placeObject(item.mBase, mVRPointer->getPointerRay(), count);
            else
                dropped = world->dropObjectOnGround(world->getPlayerPtr(), item.mBase, count);
            dropped.getCellRef().setOwner(ESM::RefId());

            return dropped;
        }

        MWWorld::Ptr addItem(const MWGui::ItemStack& item, size_t count, bool /*allowAutoEquip*/) override
        {
            return dropItemImpl(item, count, false);
        }

        MWWorld::Ptr copyItem(const MWGui::ItemStack& item, size_t count, bool /*allowAutoEquip*/) override
        {
            return dropItemImpl(item, count, true);
        }

        void removeItem(const MWGui::ItemStack& item, size_t count) override
        {
            throw std::runtime_error("removeItem not implemented");
        }
        ModelIndex getIndex(const MWGui::ItemStack& item) override
        {
            throw std::runtime_error("getIndex not implemented");
        }
        void update() override {}
        size_t getItemCount() override { return 0; }
        MWGui::ItemStack getItem(ModelIndex index) override { throw std::runtime_error("getItem not implemented"); }
        bool usesContainer(const MWWorld::Ptr&) override { return false; }

    private:
        // Where to drop the item
        MWRender::RayResult mIntersection;
        UserPointer* mVRPointer;
    };

    UserPointer::UserPointer(osg::Group* root)
        : mRoot(root)
    {
        mSpaceTransform = new VR::SpaceTransform();
        mSpaceTransform->setNodeMask(MWRender::VisMask::Mask_Pointer);
        mSpaceTransform->setName("VR Pointer deformation");
        mSpaceTransform->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        mCrosshair = std::make_unique<Crosshair>(mSpaceTransform, osg::Vec3f(1.f, 0.f, 0.f), 1.f, 0.f, true);
        mCrosshair->show();
        //mRoot->addChild(mSpaceTransform);
    }

    UserPointer::~UserPointer() {}

    void UserPointer::setSource(std::shared_ptr<VR::Space> space)
    {
        mSpace = space;
        mSpaceTransform->setSpace(mSpace);
    }

    void UserPointer::activate()
    {
        if (mPointerRay.mHit)
        {
            MWWorld::Ptr target = mPointerRay.mHitObject;
            auto wm = MWBase::Environment::get().getWindowManager();
            auto& dnd = wm->getDragAndDrop();
            if (dnd.mIsOnDragAndDrop)
            {
                // Intersected with the world while drag and drop is active
                // Drop item into the world
                MWBase::Environment::get().getWorld()->breakInvisibility(MWMechanics::getPlayer());
                DropItemAtPointModel drop(this);
                dnd.drop(&drop, nullptr);
            }
            else if (!target.isEmpty())
            {
                if (wm->isConsoleMode())
                    wm->setConsoleSelectedObject(target);
                // Don't active things during GUI mode.
                else if (wm->isGuiMode())
                {
                    if (wm->getMode() != MWGui::GM_Container && wm->getMode() != MWGui::GM_Inventory)
                        return;
                    wm->getInventoryWindow()->pickUpObject(target);
                }
                else
                {
                    MWWorld::Player& player = MWBase::Environment::get().getWorld()->getPlayer();
                    if (!tryProbePick(target))
                    {
                        auto action = target.getClass().activate(target, player.getPlayer());
                        if(action)
                            action->execute(player.getPlayer());
                    }
                }
            }
        }
    }

    bool UserPointer::tryProbePick(MWWorld::Ptr target)
    {
        std::string_view resultMessage = "";
        std::string_view resultSound = "";

        MWWorld::Player& player = MWBase::Environment::get().getWorld()->getPlayer();
        if (player.getDrawState() == MWMechanics::DrawState::Weapon)
        {
            MWWorld::Ptr playerPtr = player.getPlayer();
            MWWorld::InventoryStore& invStore = playerPtr.getClass().getInventoryStore(playerPtr);
            MWWorld::ContainerStoreIterator rightHandItem
                = invStore.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
            if (rightHandItem == invStore.end())
                return false;
            MWWorld::Ptr tool = *rightHandItem;

            if (!target.isEmpty() && (target.getCellRef().getLockLevel() != 0))
            {
                if (tool.getType() == ESM::Lockpick::sRecordId)
                    MWMechanics::Security(playerPtr).pickLock(target, tool, resultMessage, resultSound);
                else if (tool.getType() == ESM::Probe::sRecordId)
                    MWMechanics::Security(playerPtr).probeTrap(target, tool, resultMessage, resultSound);
            }

            if (!resultMessage.empty())
                MWBase::Environment::get().getWindowManager()->messageBox(resultMessage);
            if (!resultSound.empty())
            {
                MWBase::SoundManager* sndMgr = MWBase::Environment::get().getSoundManager();
                sndMgr->playSound3D(target, ESM::RefId::stringRefId(resultSound), 1.0f, 1.0f);
            }
        }

        return !(resultSound.empty() && resultMessage.empty());
    }

    const MWRender::RayResult& UserPointer::getPointerRay() const
    {
        return mPointerRay;
    }

    bool UserPointer::canPlaceObject() const
    {
        return mCanPlaceObject;
    }

    void UserPointer::update()
    {
        auto pose = Util::getNodePose(mSpaceTransform);
        mDistanceToPointerTarget = Util::getPoseTarget(mPointerRay, pose, true, false);
        // Make a ref-counted copy of the target node to ensure the object's lifetime this frame.
        mPointerTarget = mPointerRay.mHitNode;

        mCanPlaceObject = false;
        if (mPointerRay.mHit && mDistanceToPointerTarget > 0.f)
        {
            // check if the wanted position is on a flat surface, and not e.g. against a vertical wall
            mCanPlaceObject = !(
                std::acos((mPointerRay.mHitNormalWorld / mPointerRay.mHitNormalWorld.length()) * osg::Vec3f(0, 0, 1))
                >= osg::DegreesToRadians(30.f));

            mCrosshair->setStretch(mDistanceToPointerTarget / 3.f);
            mCrosshair->setWidth(0.25f);
            mCrosshair->setOffset(2.f * mDistanceToPointerTarget / 3.f);
            mCrosshair->show();

            MWVR::VRGUIManager::instance().updateFocus(mPointerRay.mHitNode, mPointerRay.mHitPointLocal);
        }
        else
        {
            mCrosshair->setStretch(0.f);
            mCrosshair->hide();

            MWVR::VRGUIManager::instance().updateFocus(nullptr, osg::Vec3(0, 0, 0));
        }
    }

    static osg::ref_ptr<osg::Geometry> createPointerGeometry(
        bool pyramid, osg::Vec3f color, float farAlpha, float nearAlpha)
    {
        osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();

        // Create pointer geometry, which will point from the tip of the player's finger.
        // The geometry will be a Four sided pyramid, with the top at the player's fingers

        static osg::Vec3 vertices[] = {
            { 0, 0, 0 }, // origin
            { -1, 1, -1 }, // A
            { -1, 1, 1 }, //  B
            { 1, 1, 1 }, //   C
            { 1, 1, -1 }, //  D
            { -1, 0, -1 }, // E
            { -1, 0, 1 }, //  F
            { 1, 0, 1 }, //   G
            { 1, 0, -1 }, //  H
        };

        osg::Vec4 colors[] = {
            osg::Vec4(color, farAlpha),
            osg::Vec4(color, nearAlpha),
            osg::Vec4(color, nearAlpha),
            osg::Vec4(color, nearAlpha),
            osg::Vec4(color, nearAlpha),
            osg::Vec4(color, farAlpha),
            osg::Vec4(color, farAlpha),
            osg::Vec4(color, farAlpha),
            osg::Vec4(color, farAlpha),
        };

        const int O = 0;
        const int A = 1;
        const int B = 2;
        const int C = 3;
        const int D = 4;
        const int E = 5;
        const int F = 6;
        const int G = 7;
        const int H = 8;

        static const int pyramidVertices[] = {
            A,
            D,
            B,
            B,
            D,
            C,
            O,
            D,
            A,
            O,
            C,
            D,
            O,
            B,
            C,
            O,
            A,
            B,
        };

        static const int CubeVertices[] = { E, F, B, D, E, A, G, E, H, D, H, E, E, B, A, G, F, E, B, F, G, C, H, D, H,
            C, G, C, D, A, C, A, B, C, B, G };

        int numVertices = 0;
        const int* triangles = nullptr;

        if (pyramid)
        {
            numVertices = sizeof(pyramidVertices) / sizeof(*pyramidVertices);
            triangles = pyramidVertices;
        }
        else
        {
            numVertices = sizeof(CubeVertices) / sizeof(*CubeVertices);
            triangles = CubeVertices;
        }

        osg::ref_ptr<osg::Vec3Array> vertexArray = new osg::Vec3Array(numVertices);
        osg::ref_ptr<osg::Vec4Array> colorArray = new osg::Vec4Array(numVertices);
        for (int i = 0; i < numVertices; i++)
        {
            (*vertexArray)[i] = vertices[triangles[i]];
            (*colorArray)[i] = colors[triangles[i]];
        }

        geometry->setVertexArray(vertexArray);
        geometry->setColorArray(colorArray, osg::Array::BIND_PER_VERTEX);
        geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, numVertices));
        geometry->setSupportsDisplayList(false);
        geometry->setDataVariance(osg::Object::STATIC);
        geometry->setName("VRPointer Geometry");

        auto stateset = geometry->getOrCreateStateSet();
        stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        stateset->setMode(GL_BLEND, osg::StateAttribute::ON);
        stateset->setAttributeAndModes(new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
        osg::ref_ptr<osg::Fog> fog(new osg::Fog);
        fog->setStart(10000000);
        fog->setEnd(10000000);
        stateset->setAttributeAndModes(fog, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);

        osg::ref_ptr<osg::LightModel> lightmodel = new osg::LightModel;
        lightmodel->setAmbientIntensity(osg::Vec4(1.0, 1.0, 1.0, 1.0));
        stateset->setAttributeAndModes(lightmodel, osg::StateAttribute::ON);
        SceneUtil::ShadowManager::instance().disableShadowsForStateSet(*stateset);

        osg::ref_ptr<osg::Material> material = new osg::Material;
        material->setColorMode(osg::Material::ColorMode::AMBIENT_AND_DIFFUSE);
        stateset->setAttributeAndModes(material, osg::StateAttribute::ON);

        // turn of sky blending
        stateset->addUniform(new osg::Uniform("far", 10000000.0f));
        stateset->addUniform(new osg::Uniform("skyBlendingStart", 8000000.0f));
        stateset->addUniform(new osg::Uniform("screenRes", osg::Vec2f{ 1, 1 }));

        MWBase::Environment::get().getResourceSystem()->getSceneManager()->recreateShaders(geometry, "objects", true);

        return geometry;
    }

    Crosshair::Crosshair(
        osg::ref_ptr<osg::Group> parent, osg::Vec3f color, float farAlpha, float nearAlpha, bool pyramid)
        : mParent(parent)
        , mGeometry(createPointerGeometry(pyramid, color, nearAlpha, farAlpha))
        , mTransform(new osg::PositionAttitudeTransform)
        , mStretch(1.f)
        , mWidth(1.f)
        , mOffset(0.f)
        , mVisible(false)
    {
        mTransform->addChild(mGeometry);
        mTransform->setAttitude(osg::Quat(0, 0, 0, 1));
        mTransform->setNodeMask(MWRender::Mask_Pointer);

        updateMatrix();
    }

    Crosshair::~Crosshair()
    {
        hide();
    }

    void Crosshair::hide()
    {
        if (mVisible)
        {
            if (mParent)
                mParent->removeChild(mTransform);
            mVisible = false;
        }
    }

    void Crosshair::show()
    {
        if (!mVisible)
        {
            if (mParent)
                mParent->addChild(mTransform);
            mVisible = true;
        }
    }

    void Crosshair::setStretch(float stretch)
    {
        mStretch = stretch;
        updateMatrix();
    }

    void Crosshair::setWidth(float width)
    {
        mWidth = width;
        updateMatrix();
    }

    void Crosshair::setOffset(float distance)
    {
        mOffset = distance;
        updateMatrix();
    }

    void Crosshair::setParent(osg::ref_ptr<osg::Group> parent)
    {
        auto visible = mVisible;
        hide();

        mParent = parent;

        if (visible)
            show();
    }

    void Crosshair::updateMatrix()
    {
        mTransform->setPosition(osg::Vec3f(0.f, mOffset, 0.f));
        mTransform->setScale(osg::Vec3f(mWidth, mStretch, mWidth));
    }

}
