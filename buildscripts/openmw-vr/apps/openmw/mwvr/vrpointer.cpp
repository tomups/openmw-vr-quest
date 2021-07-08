#include "vrpointer.hpp"
#include "vrutil.hpp"
#include "vrenvironment.hpp"

#include <osg/MatrixTransform>
#include <osg/Drawable>
#include <osg/BlendFunc>
#include <osg/Fog>
#include <osg/LightModel>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>

#include <components/sceneutil/shadow.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwrender/renderingmanager.hpp"
#include "../mwrender/vismask.hpp"

namespace MWVR
{
    UserPointer::UserPointer(osg::Group* root)
        : mRoot(root)
    {
        mPointerGeometry = createPointerGeometry();
        mPointerRescale = new osg::MatrixTransform();
        mPointerRescale->addChild(mPointerGeometry);
        mPointerTransform = new osg::MatrixTransform();
        mPointerTransform->addChild(mPointerRescale);
        mPointerTransform->setName("Pointer Transform");
        mPointerTransform->setNodeMask(MWRender::VisMask::Mask_Pointer);

        auto tm = MWVR::Environment::get().getTrackingManager();
        tm->bind(this, "pcworld");
        mHandPath = tm->stringToVRPath("/user/hand/right/input/aim/pose");

        setEnabled(true);
    }

    UserPointer::~UserPointer()
    {
    }

    void UserPointer::setParent(osg::Group* group)
    {
        bool enabled = mEnabled;
        setEnabled(false);
        mParent = group;
        setEnabled(enabled);
    }

    void UserPointer::setEnabled(bool enabled)
    {
        mRoot->removeChild(mPointerTransform);
        if (mParent)
        {
            mParent->removeChild(mPointerTransform);
            if (enabled)
            {
                mParent->addChild(mPointerTransform);
                // Morrowind's hands don't actually point forward, so we have to reorient the pointer.
                mPointerTransform->setMatrix(osg::Matrix::rotate(osg::Quat(-osg::PI_2, osg::Vec3f(0, 0, 1))));
            }
        }
        else if(enabled)
        {
            mRoot->addChild(mPointerTransform);
        }
        mEnabled = enabled;
    }

    void UserPointer::onTrackingUpdated(VRTrackingSource& source, DisplayTime predictedDisplayTime)
    {
        // If no parent is set, then the actor is currently unloaded
        // And we need to point directly from tracking data and the root
        if (!mParent)
        {
            auto tp = source.getTrackingPose(predictedDisplayTime, mHandPath, 0);
            osg::Matrix worldReference = osg::Matrix::identity();
            worldReference.preMultTranslate(tp.pose.position);
            worldReference.preMultRotate(tp.pose.orientation);
            mPointerTransform->setMatrix(worldReference);
            updatePointerTarget();

            MWBase::Environment::get().getResourceSystem()->getSceneManager()->recreateShaders(mPointerGeometry);
        }
    }

    const MWRender::RayResult& UserPointer::getPointerTarget() const
    {
        return mPointerTarget;
    }

    bool UserPointer::canPlaceObject() const
    {
        return mCanPlaceObject;
    }

    void UserPointer::updatePointerTarget()
    {
        auto* world = MWBase::Environment::get().getWorld();
        if (world)
        {
            mPointerRescale->setMatrix(osg::Matrix::scale(1, 1, 1));

            //mDistanceToPointerTarget = world->getTargetObject(mPointerTarget, mPointerTransform);
            //osg::computeLocalToWorld(mPointerTransform->getParentalNodePaths()[0]);
            mDistanceToPointerTarget = Util::getPoseTarget(mPointerTarget, Util::getNodePose(mPointerTransform), true);

            mCanPlaceObject = false;
            if (mPointerTarget.mHit)
            {
                // check if the wanted position is on a flat surface, and not e.g. against a vertical wall
                mCanPlaceObject = !(std::acos((mPointerTarget.mHitNormalWorld / mPointerTarget.mHitNormalWorld.length()) * osg::Vec3f(0, 0, 1)) >= osg::DegreesToRadians(30.f));
            }

            if (mDistanceToPointerTarget > 0.f)
                mPointerRescale->setMatrix(osg::Matrix::scale(0.25f, mDistanceToPointerTarget, 0.25f));
            else
                mPointerRescale->setMatrix(osg::Matrix::scale(0.25f, 10000.f, 0.25f));
        }
    }

    osg::ref_ptr<osg::Geometry> UserPointer::createPointerGeometry()
    {
        osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();

        // Create pointer geometry, which will point from the tip of the player's finger.
        // The geometry will be a Four sided pyramid, with the top at the player's fingers

        osg::Vec3 vertices[]{
            {0, 0, 0}, // origin
            {-1, 1, -1}, // A
            {-1, 1, 1}, //  B
            {1, 1, 1}, //   C
            {1, 1, -1}, //  D
        };

        osg::Vec4 colors[]{
            osg::Vec4(1.0f, 0.0f, 0.0f, 0.0f),
            osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f),
            osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f),
            osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f),
            osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f),
        };

        const int O = 0;
        const int A = 1;
        const int B = 2;
        const int C = 3;
        const int D = 4;

        const int triangles[] =
        {
            A,D,B,
            B,D,C,
            O,D,A,
            O,C,D,
            O,B,C,
            O,A,B,
        };
        int numVertices = sizeof(triangles) / sizeof(*triangles);
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
        SceneUtil::ShadowManager::disableShadowsForStateSet(stateset);

        osg::ref_ptr<osg::Material> material = new osg::Material;
        material->setColorMode(osg::Material::ColorMode::AMBIENT_AND_DIFFUSE);
        stateset->setAttributeAndModes(material, osg::StateAttribute::ON);

        MWBase::Environment::get().getResourceSystem()->getSceneManager()->recreateShaders(geometry);

        return geometry;
    }

}
