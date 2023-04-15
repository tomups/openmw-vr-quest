// #include "rendertoswapchain.hpp"
//
// #include <components/sceneutil/color.hpp>
// #include <components/sceneutil/nodecallback.hpp>
// #include <components/stereo/multiview.hpp>
// #include <components/vr/session.hpp>
//
// #include <osg/FrameBufferObject>
// #include <osgViewer/Renderer>
//
// namespace VR
//{
//
//     struct RenderToSwapchainNode::RenderToSwapchainInitialDrawCallback : public osg::Camera::DrawCallback
//     {
//         RenderToSwapchainInitialDrawCallback(RenderToSwapchainNode* node)
//             : mNode(node)
//         {
//
//         }
//
//         virtual void operator () (osg::RenderInfo& renderInfo) const
//         {
//             mNode->initialDraw(renderInfo);
//         }
//
//         RenderToSwapchainNode* mNode;
//     };
//
//     struct RenderToSwapchainNode::RenderToSwapchainFinalDrawCallback : public osg::Camera::DrawCallback
//     {
//         RenderToSwapchainFinalDrawCallback(RenderToSwapchainNode* node)
//             : mNode(node)
//         {
//
//         }
//
//         virtual void operator () (osg::RenderInfo& renderInfo) const
//         {
//             mNode->finalDraw(renderInfo);
//         }
//
//         RenderToSwapchainNode* mNode;
//     };
//
//     void RenderToSwapchainNode::initialDraw(osg::RenderInfo& renderInfo)
//     {
//         // Override draw framebuffer with one targetting our swapchain
//         if (mRenderFbo)
//         {
//             mRenderFbo->apply(*renderInfo.getState(), osg::FrameBufferObject::DRAW_FRAMEBUFFER);
//         }
//         else
//         {
//             auto* fbo = mFboMapper->beginFrame(renderInfo);
//             fbo->apply(*renderInfo.getState(), osg::FrameBufferObject::DRAW_FRAMEBUFFER);
//         }
//     }
//
//     void RenderToSwapchainNode::finalDraw(osg::RenderInfo& renderInfo)
//     {
//         if (mRenderFbo)
//         {
//             auto* state = renderInfo.getState();
//             auto* gl = osg::GLExtensions::Get(state->getContextID(), false);
//
//             auto* fbo = mFboMapper->beginFrame(renderInfo);
//             fbo->apply(*renderInfo.getState(), osg::FrameBufferObject::DRAW_FRAMEBUFFER);
//             mRenderFbo->apply(*renderInfo.getState(), osg::FrameBufferObject::READ_FRAMEBUFFER);
//
//             gl->glBlitFramebuffer(0, 0, mWidth, mHeight, 0, mHeight, mWidth, 0, GL_COLOR_BUFFER_BIT, GL_NEAREST);
//         }
//         mFboMapper->endFrame(renderInfo);
//     }
//
//     RenderToSwapchainNode::RenderToSwapchainNode(uint32_t width, uint32_t height, uint32_t samples)
//         : mWidth(width)
//         , mHeight(height)
//         , mSamples(samples)
//         , mFrameNumber(-1)
//         , mColorSwapchain(nullptr)
//         , mCamera(nullptr)
//     {
//         setName("RenderToSwapchainNode");
//
//         mColorSwapchain = std::shared_ptr<Swapchain>(VR::Session::instance().createSwapchain(mWidth, mHeight,
//         mSamples, 1, VR::SwapchainUse::Color, getName())); mFboMapper =
//         std::make_unique<SwapchainToFrameBufferObjectMapper>(mColorSwapchain, nullptr); if
//         (mColorSwapchain->mustFlipVertical() || SceneUtil::isSrgbxFormat(mColorSwapchain->format()))
//         {
//             // Only way to do this is a redundant blit operation, so we'll render into a 'normal' fbo first.
//             mRenderFbo = new osg::FrameBufferObject;
//             mRenderFbo->setAttachment(
//                 osg::FrameBufferObject::BufferComponent::COLOR_BUFFER,
//                 osg::FrameBufferAttachment(new osg::RenderBuffer(width, height, (mColorSwapchain->format()),
//                 samples)));
//         }
//
//         class CullCallback : public SceneUtil::NodeCallback<CullCallback, RenderToSwapchainNode*,
//         osgUtil::CullVisitor*>
//         {
//         public:
//
//             void operator()(RenderToSwapchainNode* node, osgUtil::CullVisitor* cv)
//             {
//                 node->cull(cv);
//             }
//         };
//         addCullCallback(new CullCallback);
//         setCullingActive(false);
//     }
//
//     RenderToSwapchainNode::~RenderToSwapchainNode()
//     {
//     }
//
//     std::shared_ptr<Swapchain> RenderToSwapchainNode::getColorSwapchain()
//     {
//         return mColorSwapchain;
//     }
//
//     void RenderToSwapchainNode::cull(osgUtil::CullVisitor* cv)
//     {
//         // Note: Hypothetically, we could acquire the swapchain and map to a camera using the acquired texture
//         // as its attachment. However, acquiring needs to be done on the draw thread and so would be very difficult
//         to achieve
//         // using OSG's methodology. So instead, we use a regular osg camera, and then acquire the swapchain in
//         // that camera's initial draw callback.
//         if (!mCamera)
//         {
//             mCamera = new osg::Camera;
//             mCamera->setName(getName() + "Camera");
//             mCamera->setRenderOrder(osg::Camera::POST_RENDER);
//             mCamera->setClearMask(GL_COLOR_BUFFER_BIT);
//             mCamera->setViewport(0, 0, mWidth, mHeight);
//             mCamera->setInitialDrawCallback(new RenderToSwapchainInitialDrawCallback(this));
//             mCamera->setFinalDrawCallback(new RenderToSwapchainFinalDrawCallback(this));
//             setDefaults(mCamera);
//
//             // With this target OSG will draw to whatever is bound, which is what we want.
//             // With FRAME_BUFFER_OBJECT OSG will make an FBO with attachments and bind that, overriding our own
//             framebuffer. mCamera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER);
//         }
//
//
//         auto frameNumber = static_cast<int64_t> (cv->getFrameStamp()->getFrameNumber());
//         if (frameNumber > mFrameNumber)
//         {
//             mFrameNumber = frameNumber;
//             apply(mCamera);
//             mCamera->accept(*cv);
//         }
//     }
// }
