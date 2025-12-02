#ifndef MWINPUT_MWMOUSEMANAGER_H
#define MWINPUT_MWMOUSEMANAGER_H

#include <components/sdlutil/events.hpp>
#include <components/settings/settings.hpp>

namespace SDLUtil
{
    class InputWrapper;
}

namespace MWInput
{
    class BindingsManager;

    class MouseManager : public SDLUtil::MouseListener
    {
    public:
        MouseManager(BindingsManager* bindingsManager, SDLUtil::InputWrapper* inputWrapper, SDL_Window* window);

        virtual ~MouseManager() = default;

        void updateCursorMode();
        void update(float dt);

        void mouseMoved(const SDLUtil::MouseMotionEvent& arg) override;
        void mousePressed(const SDL_MouseButtonEvent& arg, Uint8 id) override;
        void mouseReleased(const SDL_MouseButtonEvent& arg, Uint8 id) override;
        void mouseWheelMoved(const SDL_MouseWheelEvent& arg) override;

        bool injectMouseButtonPress(Uint8 button);
        bool injectMouseButtonRelease(Uint8 button);
        void injectMouseMove(float xMove, float yMove, int mouseWheelMove, bool allowedForVR = false);
        void warpMouse();
        void warpMouseToWidget(MyGUI::Widget* widget);

        void setMouseLookEnabled(bool enabled) { mMouseLookEnabled = enabled; }
        void setGuiCursorEnabled(bool enabled) { mGuiCursorEnabled = enabled; }

        int getMouseMoveX() const { return mMouseMoveX; }
        int getMouseMoveY() const { return mMouseMoveY; }
        int getMouseWheel() const { return mMouseWheel; }

    private:
        BindingsManager* mBindingsManager;
        SDLUtil::InputWrapper* mInputWrapper;

        float mGuiCursorX;
        float mGuiCursorY;
        int mMouseWheel;
        bool mMouseLookEnabled;
        bool mGuiCursorEnabled;
        float mLastWarpX;
        float mLastWarpY;

        int mMouseMoveX;
        int mMouseMoveY;
//## VR_PATCH BEGIN
    public:
        // void mouseMovedVR(const SDLUtil::MouseMotionEvent& arg);
        // VR computes mouse position based on intersections with 3d gui elements
        void setMousePosition(int x, int y);
    private:
        float mPreviousXAxis;
//## VR_PATCH END
    };
}
#endif
