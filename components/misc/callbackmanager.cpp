#include "callbackmanager.hpp"

#include "osgUtil/RenderStage"
#include "osgViewer/Renderer"
#include "osgViewer/Viewer"

#include <components/debug/debuglog.hpp>

namespace Misc
{
    struct CallbackInfo
    {
        std::shared_ptr<CallbackManager::MwDrawCallback> callback = nullptr;
        unsigned int frame = 0;
        bool oneshot = false;
    };

    //! Customized RenderStage that handles callbacks thread-safely
    //! and without any risk that unrelated code will delete your callbacks.
    class CustomRenderStage : public osgUtil::RenderStage
    {
    public:
        CustomRenderStage();

        CustomRenderStage(const osgUtil::RenderStage& rhs, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);
        CustomRenderStage(const CustomRenderStage& rhs, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

        void setCallbackManager(CallbackManager* manager) { mCallbackManager = manager; }

        void setStereoView(CallbackManager::View view) { mView = view; }

        osg::Object* cloneType() const override { return new CustomRenderStage(); }
        osg::Object* clone(const osg::CopyOp& copyop) const override
        {
            return new CustomRenderStage(*this, copyop);
        } // note only implements a clone of type.
        bool isSameKindAs(const osg::Object* obj) const override
        {
            return dynamic_cast<const CustomRenderStage*>(obj) != 0L;
        }
        const char* className() const override { return "OMWRenderStage"; }
        void draw(osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous) override;

    private:
        CallbackManager* mCallbackManager = nullptr;
        CallbackManager::View mView = CallbackManager::View::NotApplicable;
    };

    static CallbackManager* sInstance = nullptr;

    CallbackManager& CallbackManager::instance()
    {
        return *sInstance;
    }

    CallbackManager::CallbackManager(osg::ref_ptr<osgViewer::Viewer> viewer)
        : mUserCallbacks{}
        , mViewer{ viewer }
    {
        if (sInstance)
            throw std::logic_error("Double instance og StereoView");
        sInstance = this;

        auto* camera = mViewer->getCamera();

        // To make draw callbacks reliable, we have to replace the RenderStage of OSG
        // with one that calls our callback manager (in addition to calling OSG's callbacks as usual).
        osg::GraphicsOperation* graphicsOperation = camera->getRenderer();
        osgViewer::Renderer* renderer = dynamic_cast<osgViewer::Renderer*>(graphicsOperation);
        if (renderer != nullptr)
        {
            for (unsigned int i : { 0, 1 })
            {
                auto* sceneView = renderer->getSceneView(i);
                if (sceneView)
                {
                    CustomRenderStage* newRenderStage = nullptr;
                    if (auto* oldRenderStage = sceneView->getRenderStage())
                        newRenderStage = new CustomRenderStage(*oldRenderStage, osg::CopyOp::DEEP_COPY_ALL);
                    else
                        newRenderStage = new CustomRenderStage();
                    newRenderStage->setCallbackManager(this);
                    sceneView->setRenderStage(newRenderStage);

                    CustomRenderStage* newRenderStageLeft = nullptr;
                    if (auto* oldRenderStageLeft = sceneView->getRenderStageLeft())
                        newRenderStageLeft = new CustomRenderStage(*oldRenderStageLeft, osg::CopyOp::DEEP_COPY_ALL);
                    else
                        newRenderStageLeft = new CustomRenderStage(*newRenderStage, osg::CopyOp::DEEP_COPY_ALL);
                    newRenderStageLeft->setCallbackManager(this);
                    newRenderStageLeft->setStereoView(View::Left);
                    sceneView->setRenderStageLeft(newRenderStageLeft);

                    CustomRenderStage* newRenderStageRight = nullptr;
                    if (auto* oldRenderStageRight = sceneView->getRenderStageRight())
                        newRenderStageRight = new CustomRenderStage(*oldRenderStageRight, osg::CopyOp::DEEP_COPY_ALL);
                    else
                        newRenderStageRight = new CustomRenderStage(*newRenderStage, osg::CopyOp::DEEP_COPY_ALL);
                    newRenderStageRight->setCallbackManager(this);
                    newRenderStageRight->setStereoView(View::Right);
                    sceneView->setRenderStageRight(newRenderStageRight);
                }
                else
                {
                    Log(Debug::Warning) << "Warning: renderer has no SceneViews, unable to setup callback management";
                }
            }
        }
        else
        {
            Log(Debug::Warning) << "Warning: camera has no Renderer, unable to setup callback management";
        }
    }

    CallbackManager::~CallbackManager() {}

    void CallbackManager::callback(DrawStage stage, View view, osg::RenderInfo& info)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        auto frameNo = info.getState()->getFrameStamp()->getFrameNumber();
        auto& callbacks = mUserCallbacks[static_cast<int>(stage)];

        for (auto it = callbacks.begin(); it != callbacks.end(); it++)
        {
            if (frameNo >= it->frame)
            {
                bool callbackWasRun = it->callback->operator()(info, view);

                if (callbackWasRun && it->oneshot)
                {
                    it = callbacks.erase(it);
                    if (it == callbacks.end())
                        break;
                }
            }
        }

        mCondition.notify_all();
    }

    void CallbackManager::addCallback(DrawStage stage, std::shared_ptr<MwDrawCallback> cb)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        CallbackInfo callbackInfo;
        callbackInfo.callback = cb;
        callbackInfo.frame = mViewer->getFrameStamp()->getFrameNumber();
        callbackInfo.oneshot = false;
        mUserCallbacks[static_cast<int>(stage)].push_back(callbackInfo);
    }

    void CallbackManager::removeCallback(DrawStage stage, std::shared_ptr<MwDrawCallback> cb)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        auto& cbs = mUserCallbacks[static_cast<int>(stage)];
        for (uint32_t i = 0; i < cbs.size(); i++)
            if (cbs[i].callback == cb)
                cbs.erase(cbs.begin() + i);
    }

    void CallbackManager::addCallbackOneshot(DrawStage stage, std::shared_ptr<MwDrawCallback> cb)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        CallbackInfo callbackInfo;
        callbackInfo.callback = cb;
        callbackInfo.frame = mViewer->getFrameStamp()->getFrameNumber();
        callbackInfo.oneshot = true;
        mUserCallbacks[static_cast<int>(stage)].push_back(callbackInfo);
    }

    void CallbackManager::waitCallbackOneshot(DrawStage stage, std::shared_ptr<MwDrawCallback> cb)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        while (hasOneshot(stage, cb))
            mCondition.wait(lock);
    }

    bool CallbackManager::hasOneshot(DrawStage stage, std::shared_ptr<MwDrawCallback> cb)
    {
        for (auto& callbackInfo : mUserCallbacks[static_cast<int>(stage)])
            if (callbackInfo.callback == cb)
                return true;
        return false;
    }

    CustomRenderStage::CustomRenderStage() {}

    CustomRenderStage::CustomRenderStage(const osgUtil::RenderStage& rhs, const osg::CopyOp& copyop)
        : osgUtil::RenderStage(rhs, copyop)
    {
    }

    CustomRenderStage::CustomRenderStage(const CustomRenderStage& rhs, const osg::CopyOp& copyop)
        : osgUtil::RenderStage(rhs, copyop)
        , mCallbackManager(rhs.mCallbackManager)
        , mView(rhs.mView)
    {
    }

    void CustomRenderStage::draw(osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous)
    {
        // Need to inject initial/final draw callbacks roughly where they are in OSG.
        // But i don't want to copy the entire RenderStage::draw method in here.
        // I set the initial view matrix, and make an otherwise redundant push/pop of the camera to replicate the state
        // expected in OSG's initial/final callback.
        if (_stageDrawnThisFrame)
            return;

        if (mCallbackManager)
        {
            if (_initialViewMatrix.valid())
                renderInfo.getState()->setInitialViewMatrix(_initialViewMatrix.get());
            if (_camera.valid())
                renderInfo.pushCamera(_camera.get());
            mCallbackManager->callback(CallbackManager::DrawStage::Initial, mView, renderInfo);
        }

        osgUtil::RenderStage::draw(renderInfo, previous);
        if (mCallbackManager)
        {
            mCallbackManager->callback(CallbackManager::DrawStage::Final, mView, renderInfo);
            if (_camera.valid())
                renderInfo.popCamera();
        }
    }
}
