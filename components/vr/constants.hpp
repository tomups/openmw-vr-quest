#ifndef VR_CONSTANTS_HPP
#define VR_CONSTANTS_HPP

namespace VR
{

    //! Self-descriptive.
    //! I avoid using enum class for this enum, as it is frequently used as an index.
    enum Side : unsigned
    {
        Side_Left = 0,
        Side_Right = 1
    };

    //! Equivalent to OpenXR subactions, but as an enum instead of XrPath
    enum class SubAction
    {
        Head,
        HandLeft,
        HandRight,
        Gamepad,
        ALL
    };

    //! Describes the status of the tracking data. Negative values indicate failure, non-zero positive values are
    //! success states.
    enum class TrackingStatus : signed
    {
        Unknown = 0, //!< No data has been written (default value)
        Good = 1, //!< Accurate, up-to-date tracking data was used.
        Stale = 2, //!< Inaccurate, stale tracking data was used. This code is a status warning, not an error, and the
                   //!< tracking pose should be used.
        NotTracked = -1, //!< No tracking data was returned because no tracking source provides this path
        TimeInvalid = -2, //!< No tracking data was returned because the provided time stamp was invalid
        Lost = -3, //!< No tracking data was returned because the tracking source could not be read (occluded
                   //!< controller, network connectivity issues, etc.).
        RuntimeFailure = -4 //!< No tracking data was returned because of a runtime failure.
    };

    //! Returns false if status is not a success state
    inline bool operator!(TrackingStatus status)
    {
        return static_cast<signed>(status) < static_cast<signed>(TrackingStatus::Good);
    }
}

#endif
