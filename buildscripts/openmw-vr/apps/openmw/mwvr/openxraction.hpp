#ifndef OPENXR_ACTION_HPP
#define OPENXR_ACTION_HPP

#include <openxr/openxr.h>

#include <string>


namespace MWVR
{
    /// \brief C++ wrapper for the XrAction type
    struct OpenXRAction
    {
    private:
        OpenXRAction(const OpenXRAction&) = default;
        OpenXRAction& operator=(const OpenXRAction&) = default;
    public:
        OpenXRAction(XrAction action, XrActionType actionType, const std::string& actionName, const std::string& localName);
        ~OpenXRAction();

        //! Convenience
        operator XrAction() { return mAction; }

        bool getFloat(XrPath subactionPath, float& value);
        bool getBool(XrPath subactionPath, bool& value);
        bool getPoseIsActive(XrPath subactionPath);
        bool applyHaptics(XrPath subactionPath, float amplitude);

        XrAction mAction = XR_NULL_HANDLE;
        XrActionType mType;
        std::string mName;
        std::string mLocalName;
    };
}

#endif
