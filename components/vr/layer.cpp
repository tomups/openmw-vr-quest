#include "layer.hpp"
#include "frame.hpp"

namespace VR
{

    ProjectionLayerView::ProjectionLayerView()
        : colorSwapchain()
        , depthSwapchain()
        , subImage()
        , view()
    {
    }

    ProjectionLayerView::~ProjectionLayerView() {}

    ProjectionLayer::ProjectionLayer()
        : views()
    {
    }

    ProjectionLayer::~ProjectionLayer() {}

    QuadLayer::QuadLayer() {}
    QuadLayer::~QuadLayer() {}

}
