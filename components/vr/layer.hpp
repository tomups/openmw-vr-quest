#ifndef VR_LAYER_H
#define VR_LAYER_H

#include <cstdint>
#include <memory>
#include <optional>

#include <components/stereo/stereomanager.hpp>
#include <components/vr/constants.hpp>

namespace VR
{
    class Swapchain;
    class Space;

    // Describes a subregion of a swapchain
    struct SubImage
    {
        uint32_t x = 0;
        uint32_t y = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t index = 0;
    };

    struct Layer
    {
        enum class EyeVisibility
        {
            Both = 0,
            Left = 1,
            Right = 2,
        };

        enum class Type
        {
            ProjectionLayer,
            QuadLayer
        };

        virtual ~Layer(){}

        virtual Type getType() const = 0;
    };

    struct ProjectionLayerView
    {
    public:
        ProjectionLayerView();
        ~ProjectionLayerView();

        std::shared_ptr<Swapchain> colorSwapchain;
        std::shared_ptr<Swapchain> depthSwapchain;
        SubImage subImage;
        Stereo::View view;
    };

    struct ProjectionLayer : public Layer
    {
    public:
        ProjectionLayer();
        ~ProjectionLayer() override;

        Type getType() const override { return Type::ProjectionLayer; }

        std::array<ProjectionLayerView, 2> views;
    };

    struct QuadLayer : public Layer
    {
        QuadLayer();
        ~QuadLayer();

        std::shared_ptr<Swapchain> colorSwapchain = nullptr;
        bool blendAlpha = false;
        bool premultipliedAlpha = false;
        // Optional subimage. If not set, full swapchain image is used
        std::optional<SubImage> subImage;
        Stereo::Pose pose = {};
        osg::Vec2f extent = {};
        EyeVisibility eyeVisibility = EyeVisibility::Both;
        ReferenceSpace space = ReferenceSpace::View;

        Type getType() const override { return Type::QuadLayer; }
    };
}

#endif
