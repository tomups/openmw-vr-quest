#include "vrinput.hpp"
#include "openxrdebug.hpp"
#include "vrenvironment.hpp"
#include "openxrmanagerimpl.hpp"
#include "openxrtypeconversions.hpp"

#include <vector>
#include <deque>
#include <array>
#include <iostream>
#include <cassert>

namespace MWVR
{

    //! Delay before a long-press action is activated (and regular press is discarded)
    //! TODO: Make this configurable?
    static std::chrono::milliseconds gActionTime{ 666 };
    //! Magnitude above which an axis action is considered active
    static float gAxisEpsilon{ 0.01f };

    void HapticsAction::apply(float amplitude)
    {
        mAmplitude = std::max(0.f, std::min(1.f, amplitude));
        mXRAction->applyHaptics(XR_NULL_PATH, mAmplitude);
    }

    PoseAction::PoseAction(std::unique_ptr<OpenXRAction> xrAction)
        : mXRAction(std::move(xrAction))
        , mXRSpace{ XR_NULL_HANDLE }
    {
        auto* xr = Environment::get().getManager();
        XrActionSpaceCreateInfo createInfo{ XR_TYPE_ACTION_SPACE_CREATE_INFO };
        createInfo.action = *mXRAction;
        createInfo.poseInActionSpace.orientation.w = 1.f;
        createInfo.subactionPath = XR_NULL_PATH;
        CHECK_XRCMD(xrCreateActionSpace(xr->impl().xrSession(), &createInfo, &mXRSpace));
        VrDebug::setName(mXRSpace, "OpenMW XR Action Space " + mXRAction->mName);
    }

    void PoseAction::update(long long time)
    {
        mPrevious = mValue;

        auto* xr = Environment::get().getManager();
        XrSpace referenceSpace = xr->impl().getReferenceSpace(ReferenceSpace::STAGE);

        XrSpaceLocation location{ XR_TYPE_SPACE_LOCATION };
        XrSpaceVelocity velocity{ XR_TYPE_SPACE_VELOCITY };
        location.next = &velocity;
        CHECK_XRCMD(xrLocateSpace(mXRSpace, referenceSpace, time, &location));
        if (!(location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT))
            // Quat must have a magnitude of 1 but openxr sets it to 0 when tracking is unavailable.
            // I want a no-track pose to still be a valid quat so osg won't throw errors
            location.pose.orientation.w = 1;

        mValue = Pose{
            fromXR(location.pose.position),
            fromXR(location.pose.orientation)
        };
    }

    void Action::updateAndQueue(std::deque<const Action*>& queue)
    {
        bool old = mActive;
        mPrevious = mValue;
        update();
        bool changed = old != mActive;
        mOnActivate = changed && mActive;
        mOnDeactivate = changed && !mActive;

        if (shouldQueue())
        {
            queue.push_back(this);
        }
    }

    void ButtonPressAction::update()
    {
        mActive = false;
        bool old = mPressed;
        mXRAction->getBool(0, mPressed);
        bool changed = old != mPressed;
        if (changed && mPressed)
        {
            mPressTime = std::chrono::steady_clock::now();
            mTimeout = mPressTime + gActionTime;
        }
        if (changed && !mPressed)
        {
            if (std::chrono::steady_clock::now() < mTimeout)
            {
                mActive = true;
            }
        }

        mValue = mPressed ? 1.f : 0.f;
    }

    void ButtonLongPressAction::update()
    {
        mActive = false;
        bool old = mPressed;
        mXRAction->getBool(0, mPressed);
        bool changed = old != mPressed;
        if (changed && mPressed)
        {
            mPressTime = std::chrono::steady_clock::now();
            mTimein = mPressTime + gActionTime;
            mActivated = false;
        }
        if (mPressed && !mActivated)
        {
            if (std::chrono::steady_clock::now() >= mTimein)
            {
                mActive = mActivated = true;
                mValue = 1.f;
            }
        }
        if (changed && !mPressed)
        {
            mValue = 0.f;
        }
    }


    void ButtonHoldAction::update()
    {
        mXRAction->getBool(0, mPressed);
        mActive = mPressed;

        mValue = mPressed ? 1.f : 0.f;
    }


    AxisAction::AxisAction(int openMWAction, std::unique_ptr<OpenXRAction> xrAction, std::shared_ptr<AxisAction::Deadzone> deadzone)
        : Action(openMWAction, std::move(xrAction))
        , mDeadzone(deadzone)
    {
    }

    void AxisAction::update()
    {
        mActive = false;
        mXRAction->getFloat(0, mValue);
        mDeadzone->applyDeadzone(mValue);

        if (std::fabs(mValue) > gAxisEpsilon)
            mActive = true;
        else
            mValue = 0.f;
    }

    void AxisAction::Deadzone::applyDeadzone(float& value)
    {
        float sign = std::copysignf(1.f, value);
        float magnitude = std::fabs(value);
        magnitude = std::min(mActiveRadiusOuter, magnitude);
        magnitude = std::max(0.f, magnitude - mActiveRadiusInner);
        value = sign * magnitude * mActiveScale;

    }

    void AxisAction::Deadzone::setDeadzoneRadius(float deadzoneRadius)
    {
        deadzoneRadius = std::min(std::max(deadzoneRadius, 0.0f), 0.5f - 1e-5f);
        mActiveRadiusInner = deadzoneRadius;
        mActiveRadiusOuter = 1.f - deadzoneRadius;
        float activeRadius = mActiveRadiusOuter - mActiveRadiusInner;
        assert(activeRadius > 0.f);
        mActiveScale = 1.f / activeRadius;
    }

}
