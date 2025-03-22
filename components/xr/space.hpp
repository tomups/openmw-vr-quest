#pragma once

#include <components/vr/space.hpp>
#include <components/vr/vr.hpp>

#include <openxr/openxr.h>

#include <optional>

namespace XR
{
    class Space : public VR::Space
    {
    public:
        Space(XrSpace space);
        ~Space() override;

        virtual XrSpace xrSpace() const { return mSpace; }

        VR::TrackingPose locate(const VR::Space& reference) const override;
        VR::TrackingPose locateInWorld() const override;
    protected:

        mutable XrSpace mSpace;
    };

    class ReferenceSpace : public XR::Space
    {
    public:
        ReferenceSpace(VR::ReferenceSpace type, std::optional<VR::ReferenceSpace> recenterTo);
        ~ReferenceSpace();

        XrSpace xrSpace() const override;

        void recreate() const;
        void recenter();

        VR::TrackingPose locate(const VR::Space& reference) const override;
        VR::TrackingPose locateInWorld() const override;

    private:
        VR::ReferenceSpace mType;
        std::optional<VR::ReferenceSpace> mRecenterTo;
        mutable bool mRecenter;
    };
}
