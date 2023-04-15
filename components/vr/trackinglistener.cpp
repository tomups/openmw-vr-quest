#include "trackinglistener.hpp"
#include "trackingmanager.hpp"

namespace VR
{
    TrackingListener::TrackingListener()
    {
        TrackingManager::instance().addListener(this);
    }

    TrackingListener::~TrackingListener()
    {
        TrackingManager::instance().removeListener(this);
    }
}
