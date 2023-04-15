#ifndef MWVR_VRTYPES_H
#define MWVR_VRTYPES_H

// #include <memory>
// #include <array>
// #include <mutex>
// #include <components/debug/debuglog.hpp>
// #include <components/sdlutil/sdlgraphicswindow.hpp>
// #include <components/settings/settings.hpp>
// #include <components/misc/stereo.hpp>
// #include <components/vr/swapchain.hpp>
// #include <osg/Camera>
// #include <osgViewer/Viewer>
//
// namespace MWVR
//{
//     class OpenXRSwapchain;
//     class OpenXRManager;
//
//     //! Describes what limb to track.
//     enum class TrackedLimb
//     {
//         LEFT_HAND,
//         RIGHT_HAND,
//         HEAD
//     };
//
//     //! Describes what space to track the limb in
//     enum class ReferenceSpace
//     {
//         STAGE = 0, //!< Track limb in the VR stage space. Meaning a space with a floor level origin and fixed
//         horizontal orientation. VIEW = 1 //!< Track limb in the VR view space. Meaning a space with the head as
//         origin and orientation.
//     };
//
//     //! Self-descriptive
//     enum class Side
//     {
//         LEFT_SIDE = 0,
//         RIGHT_SIDE = 1
//     };
//
//     using DisplayTime = int64_t;
//
//     using Pose = Misc::Pose;
//     using FieldOfView = Misc::FieldOfView;
//     using View = Misc::View;
//
//     //! The complete set of poses tracked each frame by MWVR.
//     struct PoseSet
//     {
//         Pose eye[2]{};   //!< Stage-relative
//         Pose hands[2]{}; //!< Stage-relative
//         Pose head{};     //!< Stage-relative
//         View view[2]{};  //!< Head-relative
//
//         bool operator==(const PoseSet& rhs) const;
//     };
//
//     struct SubImage
//     {
//         VR::Swapchain* colorSwapchain = nullptr;
//         VR::Swapchain* depthSwapchain = nullptr;
//         int32_t x = 0;
//         int32_t y = 0;
//         int32_t width = 0;
//         int32_t height = 0;
//     };
//
//     struct CompositionLayerProjectionView
//     {
//         SubImage subImage;
//         Pose pose;
//         FieldOfView fov;
//     };
//
//     struct SwapchainConfig
//     {
//         int recommendedWidth = -1;
//         int recommendedHeight = -1;
//         int recommendedSamples = -1;
//         int maxWidth = -1;
//         int maxHeight = -1;
//         int maxSamples = -1;
//         int offsetWidth = 0;
//         int offsetHeight = 0;
//         std::string name = "";
//     };
//
//     struct FrameInfo
//     {
//         long long   runtimePredictedDisplayTime;
//         long long   runtimePredictedDisplayPeriod;
//         bool        runtimeRequestsRender;
//     };
//
//     // Serialization methods for VR types.
//     std::ostream& operator <<(std::ostream& os, const Pose& pose);
//     std::ostream& operator <<(std::ostream& os, const FieldOfView& fov);
//     std::ostream& operator <<(std::ostream& os, const View& view);
//     std::ostream& operator <<(std::ostream& os, const PoseSet& poseSet);
//     std::ostream& operator <<(std::ostream& os, TrackedLimb limb);
//     std::ostream& operator <<(std::ostream& os, ReferenceSpace space);
//     std::ostream& operator <<(std::ostream& os, Side side);
// }

#endif
