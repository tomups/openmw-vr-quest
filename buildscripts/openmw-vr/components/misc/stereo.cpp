#include "stereo.hpp"
#include "stringops.hpp"
#include "callbackmanager.hpp"

#include <osg/io_utils>
#include <osg/ViewportIndexed>

#include <osgUtil/CullVisitor>

#include <osgViewer/Renderer>
#include <osgViewer/Viewer>

#include <iostream>
#include <map>
#include <string>

#include <components/debug/debuglog.hpp>

#include <components/sceneutil/statesetupdater.hpp>
#include <components/sceneutil/visitor.hpp>

#include <components/settings/settings.hpp>

namespace Misc
{
    Pose Pose::operator+(const Pose& rhs)
    {
        Pose pose = *this;
        pose.position += this->orientation * rhs.position;
        pose.orientation = rhs.orientation * this->orientation;
        return pose;
    }

    const Pose& Pose::operator+=(const Pose& rhs)
    {
        *this = *this + rhs;
        return *this;
    }

    Pose Pose::operator*(float scalar)
    {
        Pose pose = *this;
        pose.position *= scalar;
        return pose;
    }

    const Pose& Pose::operator*=(float scalar)
    {
        *this = *this * scalar;
        return *this;
    }

    Pose Pose::operator/(float scalar)
    {
        Pose pose = *this;
        pose.position /= scalar;
        return pose;
    }
    const Pose& Pose::operator/=(float scalar)
    {
        *this = *this / scalar;
        return *this;
    }

    bool Pose::operator==(const Pose& rhs) const
    {
        return position == rhs.position && orientation == rhs.orientation;
    }

    osg::Matrix Pose::viewMatrix(bool useGLConventions)
    {
        if (useGLConventions)
        {
            // When applied as an offset to an existing view matrix,
            // that view matrix will already convert points to a camera space
            // with opengl conventions. So we need to convert offsets to opengl 
            // conventions.
            float y = position.y();
            float z = position.z();
            position.y() = z;
            position.z() = -y;

            y = orientation.y();
            z = orientation.z();
            orientation.y() = z;
            orientation.z() = -y;

            osg::Matrix viewMatrix;
            viewMatrix.setTrans(-position);
            viewMatrix.postMultRotate(orientation.conj());
            return viewMatrix;
        }
        else
        {
            osg::Vec3d forward = orientation * osg::Vec3d(0, 1, 0);
            osg::Vec3d up = orientation * osg::Vec3d(0, 0, 1);
            osg::Matrix viewMatrix;
            viewMatrix.makeLookAt(position, position + forward, up);

            return viewMatrix;
        }
    }

    bool FieldOfView::operator==(const FieldOfView& rhs) const
    {
        return angleDown == rhs.angleDown
            && angleUp == rhs.angleUp
            && angleLeft == rhs.angleLeft
            && angleRight == rhs.angleRight;
    }

    // near and far named with an underscore because of windows' headers galaxy brain defines.
    osg::Matrix FieldOfView::perspectiveMatrix(float near_, float far_) const
    {
        const float tanLeft = tanf(angleLeft);
        const float tanRight = tanf(angleRight);
        const float tanDown = tanf(angleDown);
        const float tanUp = tanf(angleUp);

        const float tanWidth = tanRight - tanLeft;
        const float tanHeight = tanUp - tanDown;

        const float offset = near_;

        float matrix[16] = {};

        matrix[0] = 2 / tanWidth;
        matrix[4] = 0;
        matrix[8] = (tanRight + tanLeft) / tanWidth;
        matrix[12] = 0;

        matrix[1] = 0;
        matrix[5] = 2 / tanHeight;
        matrix[9] = (tanUp + tanDown) / tanHeight;
        matrix[13] = 0;

        if (far_ <= near_) {
            matrix[2] = 0;
            matrix[6] = 0;
            matrix[10] = -1;
            matrix[14] = -(near_ + offset);
        }
        else {
            matrix[2] = 0;
            matrix[6] = 0;
            matrix[10] = -(far_ + offset) / (far_ - near_);
            matrix[14] = -(far_ * (near_ + offset)) / (far_ - near_);
        }

        matrix[3] = 0;
        matrix[7] = 0;
        matrix[11] = -1;
        matrix[15] = 0;

        return osg::Matrix(matrix);
    }

    bool View::operator==(const View& rhs) const
    {
        return pose == rhs.pose && fov == rhs.fov;
    }

    std::ostream& operator <<(
        std::ostream& os,
        const Pose& pose)
    {
        os << "position=" << pose.position << ", orientation=" << pose.orientation;
        return os;
    }

    std::ostream& operator <<(
        std::ostream& os,
        const FieldOfView& fov)
    {
        os << "left=" << fov.angleLeft << ", right=" << fov.angleRight << ", down=" << fov.angleDown << ", up=" << fov.angleUp;
        return os;
    }

    std::ostream& operator <<(
        std::ostream& os,
        const View& view)
    {
        os << "pose=< " << view.pose << " >, fov=< " << view.fov << " >";
        return os;
    }

    // Update stereo view/projection during update
    class StereoUpdateCallback : public osg::Callback
    {
    public:
        StereoUpdateCallback(StereoView* stereoView) : stereoView(stereoView) {}

        bool run(osg::Object* object, osg::Object* data) override
        {
            auto b = traverse(object, data);
            stereoView->update();
            return b;
        }

        StereoView* stereoView;
    };

    // Update states during cull
    class StereoStatesetUpdateCallback : public SceneUtil::StateSetUpdater
    {
    public:
        StereoStatesetUpdateCallback(StereoView* view)
            : stereoView(view)
        {
        }

    protected:
        virtual void setDefaults(osg::StateSet* stateset)
        {
            auto stereoViewMatrixUniform = new osg::Uniform(osg::Uniform::FLOAT_MAT4, "stereoViewMatrices", 2);
            stateset->addUniform(stereoViewMatrixUniform, osg::StateAttribute::OVERRIDE);
            auto stereoViewProjectionsUniform = new osg::Uniform(osg::Uniform::FLOAT_MAT4, "stereoViewProjections", 2);
            stateset->addUniform(stereoViewProjectionsUniform);
            auto geometryPassthroughUniform = new osg::Uniform("geometryPassthrough", false);
            stateset->addUniform(geometryPassthroughUniform);
        }

        virtual void apply(osg::StateSet* stateset, osg::NodeVisitor* /*nv*/)
        {
            stereoView->updateStateset(stateset);
        }

    private:
        StereoView* stereoView;
    };

    static StereoView* sInstance = nullptr;

    StereoView& StereoView::instance()
    {
        return *sInstance;
    }

    static osg::Camera*
        createCamera(std::string name, GLbitfield clearMask)
    {
        auto* camera = new osg::Camera;

        camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        camera->setProjectionResizePolicy(osg::Camera::FIXED);
        camera->setProjectionMatrix(osg::Matrix::identity());
        camera->setViewMatrix(osg::Matrix::identity());
        camera->setName(name);
        camera->setDataVariance(osg::Object::STATIC);
        camera->setRenderOrder(osg::Camera::NESTED_RENDER);
        camera->setClearMask(clearMask);
        camera->setUpdateCallback(new SceneUtil::StateSetUpdater());

        return camera;
    }

    StereoView::StereoView(osg::Node::NodeMask noShaderMask, osg::Node::NodeMask sceneMask)
        : mViewer(nullptr)
        , mMainCamera(nullptr)
        , mRoot(nullptr)
        , mStereoRoot(new osg::Group)
        , mUpdateCallback(new StereoUpdateCallback(this))
        , mTechnique(Technique::None)
        , mNoShaderMask(noShaderMask)
        , mSceneMask(sceneMask)
        , mCullMask(0)
        , mMasterConfig(new SharedShadowMapConfig)
        , mSlaveConfig(new SharedShadowMapConfig)
        , mSharedShadowMaps(Settings::Manager::getBool("shared shadow maps", "Stereo"))
        , mUpdateViewCallback(new DefaultUpdateViewCallback)
    {
        mMasterConfig->_id = "STEREO";
        mMasterConfig->_master = true;
        mSlaveConfig->_id = "STEREO";
        mSlaveConfig->_master = false;

        mStereoRoot->setName("Stereo Root");
        mStereoRoot->setDataVariance(osg::Object::STATIC);
        mStereoRoot->addChild(mStereoGeometryShaderRoot);
        mStereoRoot->addChild(mStereoBruteForceRoot);
        mStereoRoot->addCullCallback(new StereoStatesetUpdateCallback(this));

        if (sInstance)
            throw std::logic_error("Double instance og StereoView");
        sInstance = this;

        auto* ds = osg::DisplaySettings::instance().get();
        ds->setStereo(true);
        ds->setStereoMode(osg::DisplaySettings::StereoMode::HORIZONTAL_SPLIT);
        ds->setUseSceneViewForStereoHint(true);
    }

    void StereoView::initializeStereo(osgViewer::Viewer* viewer, Technique technique)
    {
        mViewer = viewer;
        mRoot = viewer->getSceneData()->asGroup();
        mMainCamera = viewer->getCamera();
        mCullMask = mMainCamera->getCullMask();

        setStereoTechnique(technique);
    }

    void StereoView::initializeScene()
    {
        SceneUtil::FindByNameVisitor findScene("Scene Root");
        mRoot->accept(findScene);
        mScene = findScene.mFoundNode;
        if (!mScene)
            throw std::logic_error("Couldn't find scene root");

        if (mTechnique == Technique::GeometryShader_IndexedViewports)
        {
            mLeftCamera->addChild(mScene); // Use scene directly to avoid redundant shadow computation.
            mRightCamera->addChild(mScene);
        }
    }

    void StereoView::setupBruteForceTechnique()
    {
        auto* ds = osg::DisplaySettings::instance().get();
        ds->setStereo(true);
        ds->setStereoMode(osg::DisplaySettings::StereoMode::HORIZONTAL_SPLIT);
        ds->setUseSceneViewForStereoHint(true);

        struct ComputeStereoMatricesCallback : public osgUtil::SceneView::ComputeStereoMatricesCallback
        {
            ComputeStereoMatricesCallback(StereoView* sv)
                : mStereoView(sv)
            {

            }

            osg::Matrixd computeLeftEyeProjection(const osg::Matrixd& projection) const override
            {
                return mStereoView->computeLeftEyeProjection(projection);
            }

            osg::Matrixd computeLeftEyeView(const osg::Matrixd& view) const override
            {
                return mStereoView->computeLeftEyeView(view);
            }

            osg::Matrixd computeRightEyeProjection(const osg::Matrixd& projection) const override
            {
                return mStereoView->computeRightEyeProjection(projection);
            }

            osg::Matrixd computeRightEyeView(const osg::Matrixd& view) const override
            {
                return mStereoView->computeRightEyeView(view);
            }

            StereoView* mStereoView;
        };

        auto* renderer = static_cast<osgViewer::Renderer*>(mMainCamera->getRenderer());

        // osgViewer::Renderer always has two scene views
        for (auto* sceneView : { renderer->getSceneView(0), renderer->getSceneView(1) })
        {
            sceneView->setComputeStereoMatricesCallback(new ComputeStereoMatricesCallback(this));

            if (mSharedShadowMaps)
            {
                sceneView->getCullVisitorLeft()->setUserData(mMasterConfig);
                sceneView->getCullVisitorRight()->setUserData(mSlaveConfig);
            }
        }
    }

    void StereoView::setupGeometryShaderIndexedViewportTechnique()
    {
        mLeftCamera = createCamera("Stereo Left", GL_NONE);
        mRightCamera = createCamera("Stereo Right", GL_NONE);
        mStereoBruteForceRoot->addChild(mLeftCamera);
        mStereoBruteForceRoot->addChild(mRightCamera);

        // Inject self as the root of the scene graph
        mStereoGeometryShaderRoot->addChild(mRoot);
        mViewer->setSceneData(mStereoRoot);
    }

    static void removeSlave(osgViewer::Viewer* viewer, osg::Camera* camera)
    {
        for (unsigned int i = 0; i < viewer->getNumSlaves(); i++)
        {
            auto& slave = viewer->getSlave(i);
            if (slave._camera == camera)
            {
                viewer->removeSlave(i);
                return;
            }
        }
    }

    void StereoView::removeBruteForceTechnique()
    {
        auto* ds = osg::DisplaySettings::instance().get();
        ds->setStereo(false);
        if(mMainCamera->getUserData() == mMasterConfig)
            mMainCamera->setUserData(nullptr);
    }

    void StereoView::removeGeometryShaderIndexedViewportTechnique()
    {
        mStereoGeometryShaderRoot->removeChild(mRoot);
        mViewer->setSceneData(mRoot);
        mStereoBruteForceRoot->removeChild(mLeftCamera);
        mStereoBruteForceRoot->removeChild(mRightCamera);
        mLeftCamera = nullptr;
        mRightCamera = nullptr;
    }

    void StereoView::disableStereo()
    {
        if (mTechnique == Technique::None)
            return;

        mMainCamera->removeUpdateCallback(mUpdateCallback);

        switch (mTechnique)
        {
        case Technique::GeometryShader_IndexedViewports:
            removeGeometryShaderIndexedViewportTechnique(); break;
        case Technique::BruteForce:
            removeBruteForceTechnique(); break;
        default: break;
        }

        mMainCamera->setCullMask(mCullMask);
    }

    void StereoView::enableStereo()
    {
        if (mTechnique == Technique::None)
            return;

        // Update stereo statesets/matrices, but after the main camera updates.
        auto mainCameraCB = mMainCamera->getUpdateCallback();
        mMainCamera->removeUpdateCallback(mainCameraCB);
        mMainCamera->addUpdateCallback(mUpdateCallback);
        mMainCamera->addUpdateCallback(mainCameraCB);

        switch (mTechnique)
        {
        case Technique::GeometryShader_IndexedViewports:
            setupGeometryShaderIndexedViewportTechnique(); break;
        case Technique::BruteForce:
            setupBruteForceTechnique(); break;
        default: break;
        }

        setCullMask(mCullMask);
    }

    void StereoView::setStereoTechnique(Technique technique)
    {
        if (technique == mTechnique)
            return;

        disableStereo();
        mTechnique = technique;
        enableStereo();
    }

    void StereoView::update()
    {
        View left{};
        View right{};
        double near_ = 1.f;
        double far_ = 10000.f;
        if (!mUpdateViewCallback)
        {
            Log(Debug::Error) << "StereoView: No update view callback. Stereo rendering will not work.";
            return;
        }
        mUpdateViewCallback->updateView(left, right);
        auto viewMatrix = mViewer->getCamera()->getViewMatrix();
        auto projectionMatrix = mViewer->getCamera()->getProjectionMatrix();
        near_ = Settings::Manager::getFloat("near clip", "Camera");
        far_ = Settings::Manager::getFloat("viewing distance", "Camera");

        osg::Vec3d leftEye = left.pose.position;
        osg::Vec3d rightEye = right.pose.position;

        osg::Matrix leftViewOffset = left.pose.viewMatrix(true);
        osg::Matrix rightViewOffset = right.pose.viewMatrix(true);

        osg::Matrix leftViewMatrix = viewMatrix * leftViewOffset;
        osg::Matrix rightViewMatrix = viewMatrix * rightViewOffset;

        osg::Matrix leftProjectionMatrix = left.fov.perspectiveMatrix(near_, far_);
        osg::Matrix rightProjectionMatrix = right.fov.perspectiveMatrix(near_, far_);

        mRightCamera->setViewMatrix(rightViewMatrix);
        mLeftCamera->setViewMatrix(leftViewMatrix);
        mRightCamera->setProjectionMatrix(rightProjectionMatrix);
        mLeftCamera->setProjectionMatrix(leftProjectionMatrix);

        auto width = mMainCamera->getViewport()->width();
        auto height = mMainCamera->getViewport()->height();

        // To correctly cull when drawing stereo using the geometry shader, the main camera must
        // draw a fake view+perspective that includes the full frustums of both the left and right eyes.
        // This frustum will be computed as a perspective frustum from a position P slightly behind the eyes L and R
        // where it creates the minimum frustum encompassing both eyes' frustums.
        // NOTE: I make an assumption that the eyes lie in a horizontal plane relative to the base view,
        // and lie mirrored around the Y axis (straight ahead).
        // Re-think this if that turns out to be a bad assumption.
        View frustumView;

        // Compute Frustum angles. A simple min/max.
        /* Example values for reference:
            Left:
            angleLeft   -0.767549932    float
            angleRight   0.620896876    float
            angleDown   -0.837898076    float
            angleUp	     0.726982594    float

            Right:
            angleLeft   -0.620896876    float
            angleRight   0.767549932    float
            angleDown   -0.837898076    float
            angleUp	     0.726982594    float
        */
        frustumView.fov.angleLeft = std::min(left.fov.angleLeft, right.fov.angleLeft);
        frustumView.fov.angleRight = std::max(left.fov.angleRight, right.fov.angleRight);
        frustumView.fov.angleDown = std::min(left.fov.angleDown, right.fov.angleDown);
        frustumView.fov.angleUp = std::max(left.fov.angleUp, right.fov.angleUp);

        // Check that the case works for this approach
        auto maxAngle = std::max(frustumView.fov.angleRight - frustumView.fov.angleLeft, frustumView.fov.angleUp - frustumView.fov.angleDown);
        if (maxAngle > osg::PI)
        {
            Log(Debug::Error) << "Total FOV exceeds 180 degrees. Case cannot be culled in single-pass VR. Disabling culling to cope. Consider switching to dual-pass VR.";
            mMainCamera->setCullingActive(false);
            return;
            // TODO: An explicit frustum projection could cope, so implement that later. Guarantee you there will be VR headsets with total horizontal fov > 180 in the future. Maybe already.
        }

        // Use the law of sines on the triangle spanning PLR to determine P
        double angleLeft = std::abs(frustumView.fov.angleLeft);
        double angleRight = std::abs(frustumView.fov.angleRight);
        double lengthRL = (rightEye - leftEye).length();
        double ratioRL = lengthRL / std::sin(osg::PI - angleLeft - angleRight);
        double lengthLP = ratioRL * std::sin(angleRight);

        osg::Vec3d directionLP = osg::Vec3(std::cos(-angleLeft), std::sin(-angleLeft), 0);
        osg::Vec3d LP = directionLP * lengthLP;
        frustumView.pose.position = leftEye + LP;
        //frustumView.pose.position.x() += 1000;

        // Base view position is 0.0, by definition.
        // The length of the vector P is therefore the required offset to near/far.
        auto nearFarOffset = frustumView.pose.position.length();

        // Generate the frustum matrices
        auto frustumViewMatrix = viewMatrix * frustumView.pose.viewMatrix(true);
        auto frustumProjectionMatrix = frustumView.fov.perspectiveMatrix(near_ + nearFarOffset, far_ + nearFarOffset);

        if (mTechnique == Technique::GeometryShader_IndexedViewports)
        {

            // Update camera with frustum matrices
            mMainCamera->setViewMatrix(frustumViewMatrix);
            mMainCamera->setProjectionMatrix(frustumProjectionMatrix);
            mLeftCamera->getOrCreateStateSet()->setAttribute(new osg::ViewportIndexed(0, 0, 0, width / 2, height), osg::StateAttribute::OVERRIDE);
            mRightCamera->getOrCreateStateSet()->setAttribute(new osg::ViewportIndexed(0, width / 2, 0, width / 2, height), osg::StateAttribute::OVERRIDE);
        }
        else
        {
            mLeftCamera->setClearColor(mMainCamera->getClearColor());
            mRightCamera->setClearColor(mMainCamera->getClearColor());

            mLeftCamera->setViewport(0, 0, width / 2, height);
            mRightCamera->setViewport(width / 2, 0, width / 2, height);

            if (mMasterConfig->_projection == nullptr)
                mMasterConfig->_projection = new osg::RefMatrix;
            if (mMasterConfig->_modelView == nullptr)
                mMasterConfig->_modelView = new osg::RefMatrix;

            if (mSharedShadowMaps)
            {
                mMasterConfig->_referenceFrame = mMainCamera->getReferenceFrame();
                mMasterConfig->_modelView->set(frustumViewMatrix);
                mMasterConfig->_projection->set(projectionMatrix);
            }
        }
    }

    void StereoView::updateStateset(osg::StateSet* stateset)
    {
        // Manage viewports in update to automatically catch window/resolution changes.
        auto width = mMainCamera->getViewport()->width();
        auto height = mMainCamera->getViewport()->height();
        stateset->setAttribute(new osg::ViewportIndexed(0, 0, 0, width / 2, height));
        stateset->setAttribute(new osg::ViewportIndexed(1, width / 2, 0, width / 2, height));

        // Update stereo uniforms
        auto frustumViewMatrixInverse = osg::Matrix::inverse(mMainCamera->getViewMatrix());
        //auto frustumViewProjectionMatrixInverse = osg::Matrix::inverse(mMainCamera->getProjectionMatrix()) * osg::Matrix::inverse(mMainCamera->getViewMatrix());
        auto* stereoViewMatrixUniform = stateset->getUniform("stereoViewMatrices");
        auto* stereoViewProjectionsUniform = stateset->getUniform("stereoViewProjections");

        stereoViewMatrixUniform->setElement(0, frustumViewMatrixInverse * mLeftCamera->getViewMatrix());
        stereoViewMatrixUniform->setElement(1, frustumViewMatrixInverse * mRightCamera->getViewMatrix());
        stereoViewProjectionsUniform->setElement(0, frustumViewMatrixInverse * mLeftCamera->getViewMatrix() * mLeftCamera->getProjectionMatrix());
        stereoViewProjectionsUniform->setElement(1, frustumViewMatrixInverse * mRightCamera->getViewMatrix() * mRightCamera->getProjectionMatrix());
    }

    void StereoView::setUpdateViewCallback(std::shared_ptr<UpdateViewCallback> cb)
    {
        mUpdateViewCallback = cb;
    }

    void disableStereoForCamera(osg::Camera* camera)
    {
        auto* viewport = camera->getViewport();
        camera->getOrCreateStateSet()->setAttribute(new osg::ViewportIndexed(0, viewport->x(), viewport->y(), viewport->width(), viewport->height()), osg::StateAttribute::OVERRIDE);
        camera->getOrCreateStateSet()->addUniform(new osg::Uniform("geometryPassthrough", true), osg::StateAttribute::OVERRIDE);
    }

    void enableStereoForCamera(osg::Camera* camera, bool horizontalSplit)
    {
        auto* viewport = camera->getViewport();
        auto x1 = viewport->x();
        auto y1 = viewport->y();
        auto width = viewport->width();
        auto height = viewport->height();

        auto x2 = x1;
        auto y2 = y1;

        if (horizontalSplit)
        {
            width /= 2;
            x2 += width;
        }
        else
        {
            height /= 2;
            y2 += height;
        }

        camera->getOrCreateStateSet()->setAttribute(new osg::ViewportIndexed(0, x1, y1, width, height));
        camera->getOrCreateStateSet()->setAttribute(new osg::ViewportIndexed(1, x2, y2, width, height));
        camera->getOrCreateStateSet()->addUniform(new osg::Uniform("geometryPassthrough", false));
    }

    StereoView::Technique getStereoTechnique(void)
    {
        auto stereoMethodString = Settings::Manager::getString("stereo method", "Stereo");
        auto stereoMethodStringLowerCase = Misc::StringUtils::lowerCase(stereoMethodString);
        if (stereoMethodStringLowerCase == "geometryshader")
        {
            return Misc::StereoView::Technique::GeometryShader_IndexedViewports;
        }
        if (stereoMethodStringLowerCase == "bruteforce")
        {
            return Misc::StereoView::Technique::BruteForce;
        }
        Log(Debug::Warning) << "Unknown stereo technique \"" << stereoMethodString << "\", defaulting to BruteForce";
        return StereoView::Technique::BruteForce;
    }

    void StereoView::DefaultUpdateViewCallback::updateView(View& left, View& right)
    {
        left.pose.position = osg::Vec3(-2.2, 0, 0);
        right.pose.position = osg::Vec3(2.2, 0, 0);
        left.fov = { -0.767549932, 0.620896876, 0.726982594, -0.837898076 };
        right.fov = { -0.620896876, 0.767549932, 0.726982594, -0.837898076 };
    }

    void StereoView::setCullCallback(osg::ref_ptr<osg::NodeCallback> cb)
    {
        mMainCamera->setCullCallback(cb);
    }

    void StereoView::setCullMask(osg::Node::NodeMask cullMask)
    {
        mCullMask = cullMask;
        if (mTechnique == Technique::GeometryShader_IndexedViewports)
        {
            mMainCamera->setCullMask(cullMask & ~mNoShaderMask);
            mLeftCamera->setCullMask((cullMask & mNoShaderMask) | mSceneMask);
            mRightCamera->setCullMask((cullMask & mNoShaderMask) | mSceneMask);
        }
        else
        {
            mMainCamera->setCullMask(cullMask);
            mMainCamera->setCullMaskLeft(cullMask);
            mMainCamera->setCullMaskRight(cullMask);
        }
    }
    osg::Node::NodeMask StereoView::getCullMask()
    {
        return mCullMask;
    }
    osg::Matrixd StereoView::computeLeftEyeProjection(const osg::Matrixd& projection) const
    {
        return mLeftCamera->getProjectionMatrix();
    }
    osg::Matrixd StereoView::computeLeftEyeView(const osg::Matrixd& view) const
    {
        return mLeftCamera->getViewMatrix();
    }
    osg::Matrixd StereoView::computeRightEyeProjection(const osg::Matrixd& projection) const
    {
        return mRightCamera->getProjectionMatrix();
    }
    osg::Matrixd StereoView::computeRightEyeView(const osg::Matrixd& view) const
    {
        return mRightCamera->getViewMatrix();
    }
    void StereoView::StereoDrawCallback::operator()(osg::RenderInfo& info) const
    {
        // OSG does not give any information about stereo in these callbacks so i have to infer this myself.
        // And hopefully OSG won't change this behaviour.

        View view = View::Both;

        auto camera = info.getCurrentCamera();
        auto viewport = camera->getViewport();

        // Find the current scene view. 
        osg::GraphicsOperation* graphicsOperation = info.getCurrentCamera()->getRenderer();
        osgViewer::Renderer* renderer = dynamic_cast<osgViewer::Renderer*>(graphicsOperation);
        for (int i = 0; i < 2; i++) // OSG alternates between two sceneviews.
        {
            auto* sceneView = renderer->getSceneView(i);
            // The render info argument is a member of scene view, allowing me to identify it.
            if (&sceneView->getRenderInfo() == &info)
            {
                // Now i can simply examine the viewport.
                auto activeViewport = static_cast<osg::Viewport*>(sceneView->getLocalStateSet()->getAttribute(osg::StateAttribute::Type::VIEWPORT, 0));
                if (activeViewport)
                {
                    if (activeViewport->width() == viewport->width() && activeViewport->height() == viewport->height())
                        view = View::Both;
                    else if (activeViewport->x() == viewport->x() && activeViewport->y() == viewport->y())
                        view = View::Left;
                    else
                        view = View::Right;
                }
                else
                {
                    // OSG always sets a viewport in the local stateset if osg's stereo is enabled.
                    // If it isn't, assume both.
                    view = View::Both;
                }

                break;
            }
        }

        operator()(info, view);
    }
}
