#ifndef MWVR_OPENRXSESSION_H
#define MWVR_OPENRXSESSION_H

#include <memory>
#include <mutex>
#include <array>
#include <chrono>
#include <queue>
#include <thread>
#include <condition_variable>
#include <components/debug/debuglog.hpp>
#include <components/sdlutil/sdlgraphicswindow.hpp>
#include <components/settings/settings.hpp>
#include "openxrmanager.hpp"
#include "vrviewer.hpp"

namespace MWVR
{

    extern void getEulerAngles(const osg::Quat& quat, float& yaw, float& pitch, float& roll);

    /// \brief Manages VR logic, such as managing frames, predicting their poses, and handling frame synchronization with the VR runtime.
    /// Should not be confused with the openxr session object.
    class VRSession
    {
    public:
        using seconds = std::chrono::duration<double>;
        using nanoseconds = std::chrono::nanoseconds;
        using clock = std::chrono::steady_clock;
        using time_point = clock::time_point;

        //! Describes different phases of updating/rendering each frame.
        //! While two phases suffice for disambiguating current and next frame,
        //! greater granularity allows better control of synchronization
        //! with the VR runtime.
        enum class FramePhase
        {
            Update = 0, //!< The frame currently in update traversals
            Draw, //!< The frame currently in draw
            NumPhases
        };

        struct VRFrameMeta
        {
            
            DisplayTime mFrameNo{ 0 };
            std::array<View, 2> mViews[2]{};
            bool        mShouldRender{ false };
            bool        mShouldSyncFrameLoop{ false };
            FrameInfo   mFrameInfo{};
        };

    public:
        VRSession();
        ~VRSession();

        void swapBuffers(osg::GraphicsContext* gc, VRViewer& viewer);

        //! Starts a new frame
        void prepareFrame();

        void beginPhase(FramePhase phase);
        std::unique_ptr<VRFrameMeta>& getFrame(FramePhase phase);
        bool seatedPlay() const { return mSeatedPlay; }

        float playerScale() const { return mPlayerScale; }
        void setPlayerScale(float scale) { mPlayerScale = scale; }

        float eyeLevel() const { return mEyeLevel; }
        void setEyeLevel(float eyeLevel) { mEyeLevel = eyeLevel; }

        std::array<std::unique_ptr<VRFrameMeta>, (int)FramePhase::NumPhases> mFrame{ nullptr };

        void processChangedSettings(const std::set< std::pair<std::string, std::string> >& changed);

        void beginFrame();
        void endFrame();

    protected:
        void setSeatedPlay(bool seatedPlay);

    private:
        std::mutex mMutex{};
        std::condition_variable mCondition{};

        bool mSeatedPlay{ false };
        long long mFrames{ 0 };
        long long mLastRenderedFrame{ 0 };
        long long mLastPredictedDisplayTime{ 0 };
        long long mLastPredictedDisplayPeriod{ 0 };
        std::chrono::steady_clock::time_point mStart{ std::chrono::steady_clock::now() };
        std::chrono::nanoseconds mLastFrameInterval{};
        std::chrono::steady_clock::time_point mLastRenderedFrameTimestamp{ std::chrono::steady_clock::now() };

        float mPlayerScale{ 1.f };
        float mEyeLevel{ 1.f };
    };

}

#endif
