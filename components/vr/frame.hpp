#ifndef VR_FRAME_H
#define VR_FRAME_H

#include <cstdint>
#include <memory>
#include <vector>

namespace VR
{
    struct Layer;

    struct Frame
    {
    public:
        Frame();
        ~Frame();

        bool shouldSyncFrameLoop = false;
        bool shouldRender = false;
        bool shouldSyncInput = false;
        uint64_t predictedDisplayTime = 0;
        uint64_t predictedDisplayPeriod = 0;
        uint64_t frameNumber = 0;

        std::vector<std::shared_ptr<Layer>> layers;
    };
}

#endif
