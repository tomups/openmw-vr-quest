#pragma once

#include <components/vr/space.hpp>
#include <components/vr/vr.hpp>

#include <openxr/openxr.h>

namespace XR
{
    class Space : public VR::Space
    {
    public:
        Space(XrSpace space);
        ~Space() override;

        XrSpace xrSpace() const { return mSpace; }

    protected:
        VR::TrackingPose locate(const std::shared_ptr<VR::Space>& reference) const override;
        VR::TrackingPose locateInWorld() const override;

        XrSpace mSpace;
    };
}
