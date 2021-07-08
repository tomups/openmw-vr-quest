#include "callbackmanager.hpp"

namespace Misc
{

    static CallbackManager* sInstance = nullptr;

    CallbackManager& CallbackManager::instance()
    {
        return *sInstance;
    }

    struct InternalDrawCallback : public osg::Camera::DrawCallback
    {
    public:
        InternalDrawCallback(CallbackManager* manager, CallbackManager::DrawStage stage)
            : mManager(manager)
            , mStage(stage)
        {}

        void operator()(osg::RenderInfo& info) const override { mManager->callback(mStage, info); };

    private:

        CallbackManager* mManager;
        CallbackManager::DrawStage mStage;
    };

    CallbackManager::CallbackManager(osg::ref_ptr<osgViewer::Viewer> viewer)
        : mInternalCallbacks{ }
        , mUserCallbacks{ }
        , mViewer{ viewer }
    {
        if (sInstance)
            throw std::logic_error("Double instance og StereoView");
        sInstance = this;

        mInternalCallbacks[DrawStage::Initial] = new InternalDrawCallback(this, DrawStage::Initial);
        mInternalCallbacks[DrawStage::PreDraw] = new InternalDrawCallback(this, DrawStage::PreDraw);
        mInternalCallbacks[DrawStage::PostDraw] = new InternalDrawCallback(this, DrawStage::PostDraw);
        mInternalCallbacks[DrawStage::Final] = new InternalDrawCallback(this, DrawStage::Final);

        auto* camera = mViewer->getCamera();
        camera->setInitialDrawCallback(mInternalCallbacks[DrawStage::Initial]);
        camera->setPreDrawCallback(mInternalCallbacks[DrawStage::PreDraw]);
        camera->setPostDrawCallback(mInternalCallbacks[DrawStage::PostDraw]);
        camera->setFinalDrawCallback(mInternalCallbacks[DrawStage::Final]);
    }

    void CallbackManager::callback(DrawStage stage, osg::RenderInfo& info)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        auto frameNo = info.getState()->getFrameStamp()->getFrameNumber();
        auto& callbacks = mUserCallbacks[stage];

        for (int i = 0; static_cast<unsigned int>(i) < callbacks.size(); i++)
        {
            auto& callbackInfo = callbacks[i];
            if (frameNo >= callbackInfo.frame)
            {
                callbackInfo.callback->run(info);
                if (callbackInfo.oneshot)
                {
                    callbacks.erase(callbacks.begin() + 1);
                    i--;
                }
            }
        }

        mCondition.notify_all();
    }

    void CallbackManager::addCallback(DrawStage stage, DrawCallback* cb)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        CallbackInfo callbackInfo;
        callbackInfo.callback = cb;
        callbackInfo.frame = mViewer->getFrameStamp()->getFrameNumber();
        callbackInfo.oneshot = false;
        mUserCallbacks[stage].push_back(callbackInfo);
    }

    void CallbackManager::removeCallback(DrawStage stage, DrawCallback* cb)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        auto& cbs = mUserCallbacks[stage];
        for (uint32_t i = 0; i < cbs.size(); i++)
            if (cbs[i].callback == cb)
                cbs.erase(cbs.begin() + i);
    }

    void CallbackManager::addCallbackOneshot(DrawStage stage, DrawCallback* cb)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        CallbackInfo callbackInfo;
        callbackInfo.callback = cb;
        callbackInfo.frame = mViewer->getFrameStamp()->getFrameNumber();
        callbackInfo.oneshot = true;
        mUserCallbacks[stage].push_back(callbackInfo);
    }

    void CallbackManager::waitCallbackOneshot(DrawStage stage, DrawCallback* cb)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        while (hasOneshot(stage, cb))
            mCondition.wait(lock);
    }

    void CallbackManager::beginFrame()
    {
    }

    bool CallbackManager::hasOneshot(DrawStage stage, DrawCallback* cb)
    {
        for (auto& callbackInfo : mUserCallbacks[stage])
            if (callbackInfo.callback == cb)
                return true;
        return false;
    }


}
