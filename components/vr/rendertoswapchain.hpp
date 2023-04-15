// #ifndef RENDERTOSWAPCHAIN_H
// #define RENDERTOSWAPCHAIN_H
//
// #include <osg/Camera>
//
// #include <components/vr/swapchain.hpp>
//
// #include <map>
// #include <memory>
// #include <mutex>
//
// namespace osg
//{
//     class FrameBufferObject;
// }
//
// namespace VR
//{
//     //! Similar to rttnode from components/sceneutil/rtt, targetting openxr swapchain textures rather than regular
//     opengl textures
//     //! More limited in its features than rttnode as it is intended for submission as an OpenXR layer, and not for
//     sampling.
//     //! In this version, only a stereo unaware version intended for rendering UI elements is available.
//     class RenderToSwapchainNode : public osg::Node
//     {
//         struct RenderToSwapchainInitialDrawCallback;
//         struct RenderToSwapchainFinalDrawCallback;
//         void initialDraw(osg::RenderInfo& renderInfo);
//         void finalDraw(osg::RenderInfo& renderInfo);
//
//     public:
//         RenderToSwapchainNode(uint32_t width, uint32_t height, uint32_t samples);
//         ~RenderToSwapchainNode();
//
//         std::shared_ptr<Swapchain> getColorSwapchain();
//
//         //! Set default settings - optionally override in derived classes
//         virtual void setDefaults(osg::Camera* camera) {};
//
//         virtual void apply(osg::Camera* camera) {};
//
//         void cull(osgUtil::CullVisitor* cv);
//
//         uint32_t width() const { return mWidth; }
//         uint32_t height() const { return mHeight; }
//         uint32_t samples() const { return mSamples; }
//
//     private:
//         uint32_t mWidth;
//         uint32_t mHeight;
//         uint32_t mSamples;
//         int64_t mFrameNumber;
//
//         std::shared_ptr<Swapchain> mColorSwapchain;
//         osg::ref_ptr<osg::Camera> mCamera;
//         std::unique_ptr<SwapchainToFrameBufferObjectMapper> mFboMapper;
//         osg::ref_ptr<osg::FrameBufferObject> mRenderFbo;
//     };
// }
//
// #endif
