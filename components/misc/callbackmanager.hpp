#ifndef MISC_CALLBACKMANAGER_H
#define MISC_CALLBACKMANAGER_H

#include <osg/Camera>
#include <osgUtil/CullVisitor>

#include <array>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <vector>

namespace osgViewer
{
    class Viewer;
}

namespace Misc
{
    struct CallbackInfo;

    /// Manager of DrawCallbacks on an OSG camera.
    /// The motivation behind this class is that OSG's draw callbacks are unsafe and inherently unreliable.
    ///  - OSG does not synchronize set/add/remove DrawCallback methods, making them thread unsafe. The update thread
    ///  needs to manipulate
    ///     callbacks without racing the draw thread. This class synchronizes access.
    ///  - When adding a draw callback during the update traversals of the next frame, the callback must manually
    ///  compare frame numbers to avoid firing for the
    ///     frame currently being drawn. This class automates this.
    ///  - The set*DrawCallback calls do not respect nesting and will delete any other draw callbacks. Meaning your
    ///  callback will
    ///     (seemingly) randomly disappear. This class does not have this issue.
    ///  - Your must manually implement oneshot callbacks. This class automates this. \sa addCallbackOneshot,
    ///  waitCallbackOneshot
    ///  - OSG draw callbacks provide no information about stereo. This class adds this information.
    ///
    /// \note Only initial and final draw are implemented, as the render stage details would make implementing
    /// predraw/postdraw a bigger task.
    class CallbackManager
    {

    public:
        enum class DrawStage
        {
            Initial = 0,
            Final = 1,
            StagesMax = 2
        };
        enum class View
        {
            Left,
            Right,
            NotApplicable
        };

        //! A draw callback that adds stereo information to the operator, as the StereoDrawCallback::View enumerator.
        //! When not rendering stereo or rendering stereo with multiview, this enum will always be "NotApplicable".
        //!
        //! When rendering stereo and multiview is not enabled, this enum will correspond to the current draw traversal.
        //! One callback will be issued for each (one Left, and one Right).
        //!
        //! Note that every callback whose behaviour changes with stereo or number of traversals should check the value
        //! of view, even if it does not care about Stereo. For example, a common pattern is to use DrawStage::Final to
        //! sync on end of frame rendering. If run on View::Left, this will be when only one of two views has completed
        //! rendering. Instead it should be run only when the value of view is not View::Left.
        //!
        //! \note On oneshot callbacks: These are only consumed if the callback operator returns true.

        struct MwDrawCallback
        {
        public:
            virtual ~MwDrawCallback() = default;

            //! The callback operator to be overriden.
            //! \return A bool that is true if the callback was run, and false if it was ignored. Oneshot callbacks are
            //! only consumed when the returned value is true.
            virtual bool operator()(osg::RenderInfo& info, View view) const = 0;
        };

        static CallbackManager& instance();

        CallbackManager(osg::ref_ptr<osgViewer::Viewer> viewer);
        ~CallbackManager();

        /// Internal
        void callback(DrawStage stage, View view, osg::RenderInfo& info);

        /// Add a callback to a specific stage
        void addCallback(DrawStage stage, std::shared_ptr<MwDrawCallback> cb);

        /// Remove a callback from a specific stage
        void removeCallback(DrawStage stage, std::shared_ptr<MwDrawCallback> cb);

        /// Add a callback that will only fire once before being automatically removed.
        void addCallbackOneshot(DrawStage stage, std::shared_ptr<MwDrawCallback> cb);

        /// Waits for a oneshot callback to complete. Returns immediately if already complete or no such callback exists
        void waitCallbackOneshot(DrawStage stage, std::shared_ptr<MwDrawCallback> cb);

    private:
        bool hasOneshot(DrawStage stage, std::shared_ptr<MwDrawCallback> cb);

        std::array<std::vector<CallbackInfo>, static_cast<int>(DrawStage::StagesMax)> mUserCallbacks;
        std::mutex mMutex;
        std::condition_variable mCondition;

        osg::ref_ptr<osgViewer::Viewer> mViewer;
    };
}

#endif
