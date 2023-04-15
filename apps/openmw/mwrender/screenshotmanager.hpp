#ifndef MWRENDER_SCREENSHOTMANAGER_H
#define MWRENDER_SCREENSHOTMANAGER_H

#include <osg/ref_ptr>

#include <osgViewer/Viewer>

#include <memory>

namespace MWRender
{
    class NotifyDrawCompletedCallback;

    class ScreenshotManager
    {
    public:
        ScreenshotManager(osgViewer::Viewer* viewer);
        ~ScreenshotManager();

        void screenshot(osg::Image* image, int w, int h);

    private:
        osg::ref_ptr<osgViewer::Viewer> mViewer;
//## VR_PATCH BEGIN
        std::shared_ptr<NotifyDrawCompletedCallback> mDrawCompleteCallback;
//## VR_PATCH END
    };
}

#endif
