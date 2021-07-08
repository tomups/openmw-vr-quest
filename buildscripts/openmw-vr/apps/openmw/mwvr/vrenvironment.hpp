#ifndef MWVR_ENVIRONMENT_H
#define MWVR_ENVIRONMENT_H

namespace MWVR
{
    class VRAnimation;
    class VRGUIManager;
    class VRInputManager;
    class VRSession;
    class VRTrackingManager;
    class VRViewer;
    class OpenXRManager;

    /// \brief Central hub for mw vr/openxr subsystems
    ///
    /// This class allows each mw subsystem to access any vr subsystem's top-level manager class.
    ///
    /// \attention Environment takes ownership of the manager class instances it is handed over in
    /// the set* functions.
    class Environment
    {

        static Environment* sThis;

        Environment(const Environment&) = delete;
        ///< not implemented

        Environment& operator= (const Environment&) = delete;
        ///< not implemented

    public:

        Environment();

        ~Environment();

        void cleanup();
        ///< Delete all mwvr-subsystems.

        static Environment& get();
        ///< Return instance of this class.

        MWVR::VRInputManager* getInputManager() const;

        // The OpenXRInputManager supplants the regular input manager
        // which is stored in MWBase::Environment
        // void setInputManager(MWVR::OpenXRInputManager*);

        MWVR::VRGUIManager* getGUIManager() const;
        void setGUIManager(MWVR::VRGUIManager* xrGUIManager);

        MWVR::VRAnimation* getPlayerAnimation() const;
        void setPlayerAnimation(MWVR::VRAnimation* xrAnimation);

        MWVR::VRSession* getSession() const;
        void setSession(MWVR::VRSession* xrSession);

        MWVR::VRViewer* getViewer() const;
        void setViewer(MWVR::VRViewer* xrViewer);

        MWVR::OpenXRManager* getManager() const;
        void setManager(MWVR::OpenXRManager* xrManager);

        MWVR::VRTrackingManager* getTrackingManager() const;
        void setTrackingManager(MWVR::VRTrackingManager* xrManager);

    private:
        MWVR::VRSession* mSession{ nullptr };
        MWVR::VRGUIManager* mGUIManager{ nullptr };
        MWVR::VRAnimation* mPlayerAnimation{ nullptr };
        MWVR::VRViewer* mViewer{ nullptr };
        MWVR::OpenXRManager* mOpenXRManager{ nullptr };
        MWVR::VRTrackingManager* mTrackingManager{ nullptr };
    };
}

#endif
