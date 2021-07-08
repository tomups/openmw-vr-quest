#ifndef MISC_STEREO_H
#define MISC_STEREO_H

#include <osg/Matrix>
#include <osg/Vec3>
#include <osg/Camera>
#include <osg/StateSet>

#include <memory>

#include <components/sceneutil/mwshadowtechnique.hpp>

// Some cursed headers like to define these
#if defined(near) || defined(far)
#undef near
#undef far
#endif

namespace osgViewer
{
    class Viewer;
}

namespace Misc
{
    //! Represents the relative pose in space of some object
    struct Pose
    {
        //! Position in space
        osg::Vec3 position{ 0,0,0 };
        //! Orientation in space.
        osg::Quat orientation{ 0,0,0,1 };

        //! Add one pose to another
        Pose operator+(const Pose& rhs);
        const Pose& operator+=(const Pose& rhs);

        //! Scale a pose (does not affect orientation)
        Pose operator*(float scalar);
        const Pose& operator*=(float scalar);
        Pose operator/(float scalar);
        const Pose& operator/=(float scalar);

        bool operator==(const Pose& rhs) const;

        osg::Matrix viewMatrix(bool useGLConventions);
    };

    //! Fov that defines all 4 angles from center
    struct FieldOfView {
        float    angleLeft{ 0.f };
        float    angleRight{ 0.f };
        float    angleUp{ 0.f };
        float    angleDown{ 0.f };

        bool operator==(const FieldOfView& rhs) const;

        //! Generate a perspective matrix from this fov
        osg::Matrix perspectiveMatrix(float near, float far) const;
    };

    //! Represents an eye including both pose and fov.
    struct View
    {
        Pose pose;
        FieldOfView fov;
        bool operator==(const View& rhs) const;
    };

    //! Represent two eyes. The eyes are in relative terms, and are assumed to lie on the horizon plane.
    struct StereoView
    {
        struct UpdateViewCallback
        {
            //! Called during the update traversal of every frame to source updated stereo values.
            virtual void updateView(View& left, View& right) = 0;
        };

        //! Default implementation of UpdateViewCallback that just provides some hardcoded values for debugging purposes
        struct DefaultUpdateViewCallback : public UpdateViewCallback
        {
            void updateView(View& left, View& right) override;
        };

        enum class Technique
        {
            None = 0, //!< Stereo disabled (do nothing).
            BruteForce, //!< Two slave cameras culling and drawing everything.
            GeometryShader_IndexedViewports, //!< Frustum camera culls and draws stereo into indexed viewports using an automatically generated geometry shader.
        };

        //! A draw callback that adds stereo information to the operator.
        //! The stereo information is an enum describing which of the views the callback concerns.
        //! With some stereo methods, there is only one callback, in which case the enum will be 'Both'.
        //! 
        //! A typical use case of this callback is to prevent firing callbacks twice and correctly identifying the last/first callback.
        struct StereoDrawCallback : public osg::Camera::DrawCallback
        {
        public:
            enum class View
            {
                Both, Left, Right
            };
        public:
            StereoDrawCallback()
            {}

            void operator()(osg::RenderInfo& info) const override;

            virtual void operator()(osg::RenderInfo& info, View view) const = 0;

        private:
        };

        static StereoView& instance();

        //! Adds two cameras in stereo to the mainCamera.
        //! All nodes matching the mask are rendered in stereo using brute force via two camera transforms, the rest are rendered in stereo via a geometry shader.
        //! \param noShaderMask mask in all nodes that do not use shaders and must be rendered brute force.
        //! \param sceneMask must equal MWRender::VisMask::Mask_Scene. Necessary while VisMask is still not in components/
        //! \note the masks apply only to the GeometryShader_IndexdViewports technique and can be 0 for the BruteForce technique.
        StereoView(osg::Node::NodeMask noShaderMask, osg::Node::NodeMask sceneMask);

        //! Updates uniforms with the view and projection matrices of each stereo view, and replaces the camera's view and projection matrix
        //! with a view and projection that closely envelopes the frustums of the two eyes.
        void update();
        void updateStateset(osg::StateSet* stateset);

        void initializeStereo(osgViewer::Viewer* viewer, Technique technique);
        //! Initialized scene. Call when the "scene root" node has been created
        void initializeScene();

        void setStereoTechnique(Technique technique);

        //! Callback that updates stereo configuration during the update pass
        void setUpdateViewCallback(std::shared_ptr<UpdateViewCallback> cb);

        //! Set the cull callback on the appropriate camera object
        void setCullCallback(osg::ref_ptr<osg::NodeCallback> cb);

        //! Apply the cullmask to the appropriate camera objects
        void setCullMask(osg::Node::NodeMask cullMask);

        //! Get the last applied cullmask.
        osg::Node::NodeMask getCullMask();


        osg::Matrixd computeLeftEyeProjection(const osg::Matrixd& projection) const;
        osg::Matrixd computeLeftEyeView(const osg::Matrixd& view) const;

        osg::Matrixd computeRightEyeProjection(const osg::Matrixd& projection) const;
        osg::Matrixd computeRightEyeView(const osg::Matrixd& view) const;

    private:
        void setupBruteForceTechnique();
        void setupGeometryShaderIndexedViewportTechnique();
        void removeBruteForceTechnique();
        void removeGeometryShaderIndexedViewportTechnique();
        void disableStereo();
        void enableStereo();

        osg::ref_ptr<osgViewer::Viewer> mViewer;
        osg::ref_ptr<osg::Camera>       mMainCamera;
        osg::ref_ptr<osg::Group>        mRoot;
        osg::ref_ptr<osg::Group>        mScene;
        osg::ref_ptr<osg::Group>        mStereoRoot;
        osg::ref_ptr<osg::Callback>     mUpdateCallback;
        Technique                       mTechnique;

        // Keeps state relevant to doing stereo via the geometry shader
        osg::ref_ptr<osg::Group>    mStereoGeometryShaderRoot{ new osg::Group };
        osg::Node::NodeMask         mNoShaderMask;
        osg::Node::NodeMask         mSceneMask;
        osg::Node::NodeMask         mCullMask;

        // Keeps state and cameras relevant to doing stereo via brute force
        osg::ref_ptr<osg::Group>    mStereoBruteForceRoot{ new osg::Group };
        osg::ref_ptr<osg::Camera>   mLeftCamera{ new osg::Camera };
        osg::ref_ptr<osg::Camera>   mRightCamera{ new osg::Camera };

        using SharedShadowMapConfig = SceneUtil::MWShadowTechnique::SharedShadowMapConfig;
        osg::ref_ptr<SharedShadowMapConfig> mMasterConfig;
        osg::ref_ptr<SharedShadowMapConfig> mSlaveConfig;
        bool                                mSharedShadowMaps;

        // Camera viewports
        bool flipViewOrder{ true };

        // Updates stereo configuration during the update pass
        std::shared_ptr<UpdateViewCallback> mUpdateViewCallback;

        // OSG camera callbacks set using set*callback. StereoView manages that these are always set on the appropriate camera(s);
        osg::ref_ptr<osg::NodeCallback>         mCullCallback{ nullptr };
    };

    //! Overrides all stereo-related states/uniforms to disable stereo for the scene rendered by camera
    void disableStereoForCamera(osg::Camera* camera);

    //! Overrides all stereo-related states/uniforms to enable stereo for the scene rendered by camera
    void enableStereoForCamera(osg::Camera* camera, bool horizontalSplit);

    //! Reads settings to determine stereo technique
    StereoView::Technique getStereoTechnique(void);
}

#endif
