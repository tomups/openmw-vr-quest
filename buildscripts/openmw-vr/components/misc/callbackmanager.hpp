#ifndef MISC_CALLBACKMANAGER_H
#define MISC_CALLBACKMANAGER_H

#include <osg/Camera>
#include <osgViewer/Viewer>

#include <vector>
#include <map>
#include <mutex>
#include <condition_variable>

namespace Misc
{
    /// Manager of DrawCallbacks on an OSG camera.
    /// The motivation behind this class is that OSG's draw callbacks are inherently thread unsafe.
    /// Primarily, as the main thread has no way to synchronize with the draw thread when adding/removing draw callbacks, 
    /// when adding a draw callback the callback must manually compare frame numbers to make sure it doesn't fire for the 
    /// previous frame's draw traversals. This class automates this.
    /// Secondly older versions of OSG that are still supported by openmw do not support nested callbacks. And the ones that do
    /// make no attempt at synchronizing adding/removing nested callbacks, causing a potentially fatal race condition when needing
    /// to dynamically add or remove a nested callback.
    class CallbackManager
    {

    public:
        enum class DrawStage
        {
            Initial, PreDraw, PostDraw, Final
        };

        using DrawCallback = osg::Camera::DrawCallback;
        struct CallbackInfo
        {
            osg::ref_ptr<DrawCallback> callback = nullptr;
            unsigned int frame = 0;
            bool oneshot = false;
        };

        static CallbackManager& instance();

        CallbackManager(osg::ref_ptr<osgViewer::Viewer> viewer);

        /// Internal
        void callback(DrawStage stage, osg::RenderInfo& info);

        /// Add a callback to a specific stage
        void addCallback(DrawStage stage, DrawCallback* cb);

        /// Remove a callback from a specific stage
        void removeCallback(DrawStage stage, DrawCallback* cb);

        /// Add a callback that will only fire once before being automatically removed.
        void addCallbackOneshot(DrawStage stage, DrawCallback* cb);

        /// Waits for a oneshot callback to complete. Returns immediately if already complete or no such callback exists
        void waitCallbackOneshot(DrawStage stage, DrawCallback* cb);

        ///
        void beginFrame();

    private:

        bool hasOneshot(DrawStage stage, DrawCallback* cb);

        std::map<DrawStage, osg::ref_ptr<DrawCallback> > mInternalCallbacks;
        std::map<DrawStage, std::vector<CallbackInfo> > mUserCallbacks;
        std::mutex mMutex;
        std::condition_variable mCondition;

        uint32_t mFrame;
        osg::ref_ptr<osgViewer::Viewer> mViewer;
    };
}

#endif
