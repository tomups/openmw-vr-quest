#ifndef OPENMW_COMPONENTS_SDLUTIL_SDLVIDEOWRAPPER_H
#define OPENMW_COMPONENTS_SDLUTIL_SDLVIDEOWRAPPER_H

#include <osg/ref_ptr>

#include <SDL_types.h>

struct SDL_Window;

namespace osgViewer
{
    class Viewer;
}

namespace SDLUtil
{

    class VideoWrapper
    {
    public:
        VideoWrapper(SDL_Window* window, osg::ref_ptr<osgViewer::Viewer> viewer, bool shouldManageGamma);
        ~VideoWrapper();

        void setSyncToVBlank(bool sync);

        void setGammaContrast(float gamma, float contrast);

        void setVideoMode(int width, int height, bool fullscreen, bool windowBorder);

        void centerWindow();

    private:
        SDL_Window* mWindow;
        osg::ref_ptr<osgViewer::Viewer> mViewer;

        float mGamma;
        float mContrast;
        bool mShouldManageGamma;
        bool mHasSetGammaContrast;

        // Store system gamma ramp on window creation. Restore system gamma ramp on exit
        Uint16 mOldSystemGammaRamp[256*3];
    };

}

#endif
