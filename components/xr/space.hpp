#pragma once

#include <components/vr/space.hpp>
#include <components/vr/vr.hpp>

#include <openxr/openxr.h>

#include <optional>

namespace XR
{
    struct XrSpaceWrapper
    {
        XrSpaceWrapper(XrSpace space)
            : mSpace(space)
        {
        }

        ~XrSpaceWrapper();

        void operator=(XrSpace space);

        XrSpaceWrapper(const XrSpaceWrapper&) = delete;
        void operator=(const XrSpaceWrapper&) = delete;

        XrSpace mSpace = XR_NULL_HANDLE;
    };

    class Space : public VR::Space
    {
    public:
        Space(XrSpace space);
        ~Space() override;

        virtual XrSpace xrSpace() { return mSpace.mSpace; }

        VR::TrackingPose locate(VR::Space& reference) override;
        VR::TrackingPose locateInWorld() override;
    protected:

        mutable XrSpaceWrapper mSpace;
    };

    class ReferenceSpace : public XR::Space
    {
    public:
        ReferenceSpace(VR::ReferenceSpace type, std::optional<VR::ReferenceSpace> recenterTo);
        ~ReferenceSpace();

        XrSpace xrSpace() override;

        void recreate();
        void recenter(bool recenterX, bool recenterY, bool recenterZ);

    private:
        VR::ReferenceSpace mType;
        Stereo::Pose mCenter;
        std::optional<VR::ReferenceSpace> mRecenterTo;
        mutable bool mRecenter;
    };
}
