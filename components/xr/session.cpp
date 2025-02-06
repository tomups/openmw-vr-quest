#include "session.hpp"
#include "debug.hpp"
#include "instance.hpp"
#include "swapchain.hpp"
#include "typeconversion.hpp"

#include <components/misc/constants.hpp>
#include <components/sceneutil/depth.hpp>
#include <components/vr/frame.hpp>
#include <components/vr/layer.hpp>
#include <components/vr/trackingsource.hpp>
#include <components/vr/vr.hpp>
#include <components/xr/tracking.hpp>

#include <cassert>
#include <sstream>

namespace XR
{
    // OSG doesn't provide API to extract euler angles from a quat, but i need it.
    // Credits goes to Dennis Bunfield, i just copied his formula https://narkive.com/v0re6547.4
    void getEulerAngles(const osg::Quat& quat, float& yaw, float& pitch, float& roll)
    {
        // Now do the computation
        osg::Matrixd m2(osg::Matrixd::rotate(quat));
        double* mat = (double*)m2.ptr();
        double angle_x = 0.0;
        double angle_y = 0.0;
        double angle_z = 0.0;
        double C, tr_x, tr_y;
        angle_y = asin(mat[2]); /* Calculate Y-axis angle */
        C = cos(angle_y);
        if (fabs(C) > 0.005) /* Test for Gimball lock? */
        {
            tr_x = mat[10] / C; /* No, so get X-axis angle */
            tr_y = -mat[6] / C;
            angle_x = atan2(tr_y, tr_x);
            tr_x = mat[0] / C; /* Get Z-axis angle */
            tr_y = -mat[1] / C;
            angle_z = atan2(tr_y, tr_x);
        }
        else /* Gimball lock has occurred */
        {
            angle_x = 0; /* Set X-axis angle to zero
                          */
            tr_x = mat[5]; /* And calculate Z-axis angle
                            */
            tr_y = mat[4];
            angle_z = atan2(tr_y, tr_x);
        }

        yaw = angle_z;
        pitch = angle_x;
        roll = angle_y;
    }

    static Session* sSession = nullptr;

    Session& Session::instance()
    {
        assert(sSession);
        return *sSession;
    }

    Session::Session(XrSession session, XrViewConfigurationType viewConfigType)
        : mXrSession(session)
        , mViewConfigType(viewConfigType)
    {
        if (!sSession)
            sSession = this;
        else
            throw std::logic_error("Duplicated XR::Session singleton");

        Debugging::setName(mXrSession, "OpenMW XR Session");

        init();
    }

    Session::~Session()
    {
        cleanup();
    }

    void Session::xrResourceAcquired()
    {
        std::scoped_lock lock(mMutex);
        mAcquiredResources++;
    }

    void Session::xrResourceReleased()
    {
        assert(mAcquiredResources != 0);

        std::scoped_lock lock(mMutex);
        mAcquiredResources--;
    }

    void Session::newFrame(uint64_t frameNo, bool& shouldSyncFrame, bool& shouldSyncInput)
    {
        handleEvents();
        shouldSyncFrame = mAppShouldSyncFrameLoop;
        shouldSyncInput = mAppShouldReadInput;
    }

    void Session::syncFrameUpdate(
        uint64_t frameNo, bool& shouldRender, uint64_t& predictedDisplayTime, uint64_t& predictedDisplayPeriod)
    {
        XrFrameWaitInfo frameWaitInfo{};
        frameWaitInfo.type = XR_TYPE_FRAME_WAIT_INFO;

        XrFrameState frameState{};
        frameState.type = XR_TYPE_FRAME_STATE;

        CHECK_XRCMD(xrWaitFrame(mXrSession, &frameWaitInfo, &frameState));
        shouldRender = frameState.shouldRender;
        predictedDisplayTime = frameState.predictedDisplayTime;
        predictedDisplayPeriod = frameState.predictedDisplayPeriod;
    }

    void Session::syncFrameRender(VR::Frame& frame)
    {
        XrFrameBeginInfo frameBeginInfo{};
        frameBeginInfo.type = XR_TYPE_FRAME_BEGIN_INFO;
        CHECK_XRCMD(xrBeginFrame(mXrSession, &frameBeginInfo));
    }

    void Session::syncFrameEnd(VR::Frame& frame)
    {
        std::vector<XrCompositionLayerBaseHeader*> layerStack;
        layerStack.reserve(20);

        std::vector<XrCompositionLayerQuad> quadLayers;
        quadLayers.reserve(20);

        XrCompositionLayerProjection xrProjectionLayer{};
        xrProjectionLayer.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION;

        std::array<XrCompositionLayerProjectionView, 2> compositionLayerProjectionViews{};
        compositionLayerProjectionViews[0].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        compositionLayerProjectionViews[1].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;

        std::array<XrCompositionLayerDepthInfoKHR, 2> compositionLayerDepth{};
        compositionLayerDepth[0].type = XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR;
        compositionLayerDepth[1].type = XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR;

        XrCompositionLayerReprojectionInfoMSFT reprojectionInfoDepth{};
        reprojectionInfoDepth.type = XR_TYPE_COMPOSITION_LAYER_REPROJECTION_INFO_MSFT;
        reprojectionInfoDepth.reprojectionMode = XR_REPROJECTION_MODE_DEPTH_MSFT;

        XrFrameEndInfo frameEndInfo{};
        frameEndInfo.type = XR_TYPE_FRAME_END_INFO;
        frameEndInfo.displayTime = frame.predictedDisplayTime;
        frameEndInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;

        if (frame.shouldRender && frame.layers.size() > 0)
        {
            for (auto& layer : frame.layers)
            {
                switch (layer->getType())
                {
                    case VR::Layer::Type::ProjectionLayer:
                    {
                        VR::ProjectionLayer* projectionLayer = static_cast<VR::ProjectionLayer*>(layer.get());

                        xrProjectionLayer.space = mReferenceSpaceLocal;
                        xrProjectionLayer.viewCount = 2;
                        xrProjectionLayer.views = compositionLayerProjectionViews.data();

                        for (uint32_t i = 0; i < 2; i++)
                        {
                            auto& xrView = compositionLayerProjectionViews[i];
                            auto& view = projectionLayer->views[i];
                            xrView.fov = toXR(view.view.fov);
                            xrView.pose = toXR(view.view.pose);
                            xrView.subImage.imageArrayIndex = view.subImage.index;
                            xrView.subImage.imageRect.extent.width = view.subImage.width;
                            xrView.subImage.imageRect.extent.height = view.subImage.height;
                            xrView.subImage.imageRect.offset.x = view.subImage.x;
                            xrView.subImage.imageRect.offset.y = view.subImage.y;
                            xrView.subImage.swapchain = static_cast<XrSwapchain>(view.colorSwapchain->handle());
                        }

                        if (appShouldShareDepthInfo())
                        {
                            // TODO: Cache these values instead?
                            auto nearClip = Settings::Manager::getFloat("near clip", "Camera");
                            auto farClip = Settings::Manager::getFloat("viewing distance", "Camera");
                            if (SceneUtil::AutoDepth::isReversed())
                                std::swap(nearClip, farClip);
                            for (uint32_t i = 0; i < 2; i++)
                            {

                                auto& view = projectionLayer->views[i];
                                if (!view.depthSwapchain)
                                    continue;

                                auto& xrDepth = compositionLayerDepth[i];
                                xrDepth.minDepth = 0.;
                                xrDepth.maxDepth = 1.0;
                                xrDepth.nearZ = nearClip;
                                xrDepth.farZ = farClip;
                                xrDepth.subImage.imageArrayIndex = 0;
                                xrDepth.subImage.imageRect.extent.width = view.subImage.width;
                                xrDepth.subImage.imageRect.extent.height = view.subImage.height;
                                xrDepth.subImage.imageRect.offset.x = view.subImage.x;
                                xrDepth.subImage.imageRect.offset.y = view.subImage.y;
                                xrDepth.subImage.swapchain = static_cast<XrSwapchain>(view.depthSwapchain->handle());

                                auto& xrView = compositionLayerProjectionViews[i];
                                xrView.next = &xrDepth;
                            }
                        }

                        const void** layerNext = &xrProjectionLayer.next;

                        if (mMSFTReprojectionModeDepth)
                        {
                            *layerNext = &reprojectionInfoDepth;
                        }

                        layerStack.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&xrProjectionLayer));
                        break;
                    }
                    case VR::Layer::Type::QuadLayer:
                    {
                        VR::QuadLayer* quadLayer = static_cast<VR::QuadLayer*>(layer.get());
                        quadLayers.push_back({});
                        quadLayers.back().type = XR_TYPE_COMPOSITION_LAYER_QUAD;
                        quadLayers.back().next = nullptr;

                        quadLayers.back().eyeVisibility
                            = XR_EYE_VISIBILITY_BOTH; // static_cast<XrEyeVisibility>(quadLayer->eyeVisibility);

                        quadLayers.back().layerFlags = 0;
                        if (quadLayer->blendAlpha)
                            quadLayers.back().layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
                        if (!quadLayer->premultipliedAlpha)
                            quadLayers.back().layerFlags |= XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT;

                        auto pose = quadLayer->pose;
                        quadLayers.back().pose = toXR(pose);
                        quadLayers.back().size.width = quadLayer->extent.x() / Constants::UnitsPerMeter;
                        quadLayers.back().size.height = quadLayer->extent.y() / Constants::UnitsPerMeter;
                        quadLayers.back().space = quadLayer->space;

                        if (quadLayer->subImage)
                        {
                            quadLayers.back().subImage.imageArrayIndex = quadLayer->subImage->index;
                            quadLayers.back().subImage.imageRect.extent.width = quadLayer->subImage->width;
                            quadLayers.back().subImage.imageRect.extent.height = quadLayer->subImage->height;
                            quadLayers.back().subImage.imageRect.offset.x = quadLayer->subImage->x;
                            quadLayers.back().subImage.imageRect.offset.y = quadLayer->subImage->y;
                        }
                        else
                        {
                            quadLayers.back().subImage.imageArrayIndex = 0;
                            quadLayers.back().subImage.imageRect.extent.width = quadLayer->colorSwapchain->width();
                            quadLayers.back().subImage.imageRect.extent.height = quadLayer->colorSwapchain->height();
                            quadLayers.back().subImage.imageRect.offset.x = 0;
                            quadLayers.back().subImage.imageRect.offset.y = 0;
                        }

                        quadLayers.back().subImage.swapchain
                            = static_cast<XrSwapchain>(quadLayer->colorSwapchain->handle());

                        if (quadLayers.back().subImage.imageRect.extent.width > 0
                            && quadLayers.back().subImage.imageRect.extent.height > 0)
                            layerStack.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&quadLayers.back()));

                        break;
                    }
                }
            }
        }

        if (layerStack.size() > 0)
        {
            frameEndInfo.layerCount = layerStack.size();
            frameEndInfo.layers = layerStack.data();
        }
        else
        {
            frameEndInfo.layerCount = 0;
            frameEndInfo.layers = nullptr;
        }
        CHECK_XRCMD(xrEndFrame(mXrSession, &frameEndInfo));
    }

    void Session::handleEvents()
    {
        xrQueueEvents();

        while (auto* event = nextEvent())
        {
            if (!processEvent(event))
            {
                // Do not consider processing an event optional.
                // Retry once per frame until every event has been successfully processed
                return;
            }
            popEvent();
        }

        if (mXrSessionShouldStop)
        {
            if (checkStopCondition())
            {
                CHECK_XRCMD(xrEndSession(mXrSession));
                mXrSessionShouldStop = false;
            }
        }
    }

    const XrEventDataBaseHeader* Session::nextEvent()
    {
        if (mEventQueue.size() > 0)
            return reinterpret_cast<XrEventDataBaseHeader*>(&mEventQueue.front());
        return nullptr;
    }

    bool Session::processEvent(const XrEventDataBaseHeader* header)
    {
        Log(Debug::Verbose) << "OpenXR: Event received: " << to_string(header->type);
        switch (header->type)
        {
            case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
            {
                const auto* stateChangeEvent = reinterpret_cast<const XrEventDataSessionStateChanged*>(header);
                return handleSessionStateChanged(*stateChangeEvent);
                break;
            }
            case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
                xrInteractionProfileChanged();

                break;
            case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
            case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
            default:
            {
                Log(Debug::Verbose) << "OpenXR: Event ignored";
                break;
            }
        }
        return true;
    }

    bool Session::handleSessionStateChanged(const XrEventDataSessionStateChanged& stateChangedEvent)
    {
        Log(Debug::Verbose) << "XrEventDataSessionStateChanged: state " << to_string(mState) << "->"
                            << to_string(stateChangedEvent.state);
        mState = stateChangedEvent.state;

        // Ref: https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#session-states
        switch (mState)
        {
            case XR_SESSION_STATE_IDLE:
            {
                mAppShouldSyncFrameLoop = false;
                mAppShouldReadInput = false;
                mXrSessionShouldStop = false;
                break;
            }
            case XR_SESSION_STATE_READY:
            {
                mAppShouldSyncFrameLoop = true;
                mAppShouldReadInput = false;
                mXrSessionShouldStop = false;

                XrSessionBeginInfo beginInfo{};
                beginInfo.type = XR_TYPE_SESSION_BEGIN_INFO;
                beginInfo.primaryViewConfigurationType = mViewConfigType;
                CHECK_XRCMD(xrBeginSession(mXrSession, &beginInfo));

                break;
            }
            case XR_SESSION_STATE_STOPPING:
            {
                mAppShouldSyncFrameLoop = false;
                mAppShouldReadInput = false;
                mXrSessionShouldStop = true;
                break;
            }
            case XR_SESSION_STATE_SYNCHRONIZED:
            {
                mAppShouldSyncFrameLoop = true;
                mAppShouldReadInput = false;
                mXrSessionShouldStop = false;
                break;
            }
            case XR_SESSION_STATE_VISIBLE:
            {
                mAppShouldSyncFrameLoop = true;
                mAppShouldReadInput = false;
                mXrSessionShouldStop = false;
                break;
            }
            case XR_SESSION_STATE_FOCUSED:
            {
                mAppShouldSyncFrameLoop = true;
                mAppShouldReadInput = true;
                mXrSessionShouldStop = false;
                VR::recenter();
                VR::resetEyeLevel();
                break;
            }
            default:
                Log(Debug::Warning) << "XrEventDataSessionStateChanged: Ignoring new state " << to_string(mState);
        }

        return true;
    }

    bool Session::checkStopCondition()
    {
        return mAcquiredResources == 0;
    }

    void Session::xrInteractionProfileChanged()
    {
        // Unfortunately, openxr does not tell us WHICH profile has changed.
        std::array<std::string, 5> topLevelUserPaths
            = { "/user/hand/left", "/user/hand/right", "/user/head", "/user/gamepad", "/user/treadmill" };

        for (auto& userPath : topLevelUserPaths)
        {
            XrPath xrPath = VR::stringToXrPath(userPath);

            XrInteractionProfileState interactionProfileState{};
            interactionProfileState.type = XR_TYPE_INTERACTION_PROFILE_STATE;

            xrGetCurrentInteractionProfile(XR::Session::instance().xrSession(), xrPath, &interactionProfileState);
            if (interactionProfileState.interactionProfile)
            {
                uint32_t size;
                xrPathToString(XR::Instance::instance().xrInstance(), interactionProfileState.interactionProfile, 0,
                    &size, nullptr);
                std::string profile(size, ' ');
                xrPathToString(XR::Instance::instance().xrInstance(), interactionProfileState.interactionProfile, size,
                    &size, profile.data());
                Log(Debug::Verbose) << userPath << ": Interaction profile changed to '" << profile.data() << "'";
                setInteractionProfileActive(xrPath, VR::stringToXrPath(profile), true);
            }
            else
            {
                Log(Debug::Verbose) << userPath << ": Interaction profile lost";
                setInteractionProfileActive(xrPath, 0, false);
            }
        }
    }

    void Session::init()
    {
        createXrReferenceSpaces();
        initCompositionLayerDepth();
        initMSFTReprojection();
    }

    void Session::cleanup()
    {
        destroyXrReferenceSpaces();
        destroyXrSession();
    }

    void Session::destroyXrReferenceSpaces()
    {
        if (mReferenceSpaceLocal)
        {
            CHECK_XRCMD(xrDestroySpace(mReferenceSpaceLocal));
            mReferenceSpaceLocal = XR_NULL_HANDLE;
        }
        if (mReferenceSpaceStage)
        {
            CHECK_XRCMD(xrDestroySpace(mReferenceSpaceStage));
            mReferenceSpaceStage = XR_NULL_HANDLE;
        }
        if (mReferenceSpaceView)
        {
            CHECK_XRCMD(xrDestroySpace(mReferenceSpaceView));
            mReferenceSpaceView = XR_NULL_HANDLE;
        }
    }

    void Session::destroyXrSession()
    {
        if (mXrSession)
        {
            CHECK_XRCMD(xrDestroySession(mXrSession));
        }
    }

    void Session::initCompositionLayerDepth()
    {
        setAppShouldShareDepthBuffer(XR::Extensions::instance().extensionEnabled(XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME));
    }

    PFN_xrEnumerateReprojectionModesMSFT enumerateReprojectionModesMSFT = nullptr;

    std::string xrReprojectionModeMSFTToString(XrReprojectionModeMSFT mode)
    {
        switch (mode)
        {
            case XR_REPROJECTION_MODE_DEPTH_MSFT:
                return "XR_REPROJECTION_MODE_DEPTH_MSFT";
            case XR_REPROJECTION_MODE_PLANAR_FROM_DEPTH_MSFT:
                return "XR_REPROJECTION_MODE_PLANAR_FROM_DEPTH_MSFT";
            case XR_REPROJECTION_MODE_PLANAR_MANUAL_MSFT:
                return "XR_REPROJECTION_MODE_PLANAR_MANUAL_MSFT";
            case XR_REPROJECTION_MODE_ORIENTATION_ONLY_MSFT:
                return "XR_REPROJECTION_MODE_ORIENTATION_ONLY_MSFT";
            case XR_REPROJECTION_MODE_MAX_ENUM_MSFT:
                return "XR_REPROJECTION_MODE_MAX_ENUM_MSFT";
            default:
                return std::string() + "Unknown (" + std::to_string(mode) + ")";
        }
    }

    void Session::initMSFTReprojection()
    {
        if (XR::Extensions::instance().extensionEnabled(XR_MSFT_COMPOSITION_LAYER_REPROJECTION_EXTENSION_NAME)
            && XR::Extensions::instance().extensionEnabled(XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME))
        {
            std::vector<XrReprojectionModeMSFT> modes;
            enumerateReprojectionModesMSFT = reinterpret_cast<PFN_xrEnumerateReprojectionModesMSFT>(
                Instance::instance().xrGetFunction("xrEnumerateReprojectionModesMSFT"));

            uint32_t count = 0;
            enumerateReprojectionModesMSFT(Instance::instance().xrInstance(), Instance::instance().xrSystemId(),
                XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &count, nullptr);
            modes.resize(count);
            enumerateReprojectionModesMSFT(Instance::instance().xrInstance(), Instance::instance().xrSystemId(),
                XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, count, &count, modes.data());
            Log(Debug::Verbose) << "Supported XrReprojectionModeMSFT values:";
            for (auto mode : modes)
            {
                Log(Debug::Verbose) << "  " << xrReprojectionModeMSFTToString(mode);

                switch (mode)
                {
                    case XR_REPROJECTION_MODE_DEPTH_MSFT:
                        mMSFTReprojectionModeDepth = true;
                        break;
                    default:
                        break;
                }
            }

            setAppShouldShareDepthBuffer(mMSFTReprojectionModeDepth);
        }
    }

    std::shared_ptr<VR::Swapchain> Session::createSwapchain(uint32_t width, uint32_t height, uint32_t samples,
        uint32_t arraySize, VR::Swapchain::Attachment attachment, const std::string& name)
    {
        return std::make_shared<XR::Swapchain>(width, height, samples, arraySize, attachment, name);
    }

    bool Session::xrNextEvent(XrEventDataBuffer& eventBuffer)
    {
        XrEventDataBaseHeader* baseHeader = reinterpret_cast<XrEventDataBaseHeader*>(&eventBuffer);
        baseHeader->type = XR_TYPE_EVENT_DATA_BUFFER;
        const XrResult result = xrPollEvent(Instance::instance().xrInstance(), &eventBuffer);
        if (result == XR_SUCCESS)
        {
            if (baseHeader->type == XR_TYPE_EVENT_DATA_EVENTS_LOST)
            {
                const XrEventDataEventsLost* const eventsLost
                    = reinterpret_cast<const XrEventDataEventsLost*>(baseHeader);
                Log(Debug::Warning) << "OpenXRManagerImpl: Lost " << eventsLost->lostEventCount << " events";
            }

            return baseHeader;
        }

        if (result != XR_EVENT_UNAVAILABLE)
            CHECK_XRRESULT(result, "xrPollEvent");
        return false;
    }

    void Session::popEvent()
    {
        if (mEventQueue.size() > 0)
            mEventQueue.pop();
    }

    void Session::xrQueueEvents()
    {
        XrEventDataBuffer eventBuffer{};
        while (xrNextEvent(eventBuffer))
        {
            mEventQueue.push(eventBuffer);
        }
    }

    void Session::createXrReferenceSpaces()
    {
        uint32_t typeCount = 0;
        CHECK_XRCMD(xrEnumerateReferenceSpaces(mXrSession, 0, &typeCount, nullptr));
        mReferenceSpaceTypes.resize(typeCount);
        CHECK_XRCMD(xrEnumerateReferenceSpaces(mXrSession, typeCount, &typeCount, mReferenceSpaceTypes.data()));
        Log(Debug::Verbose) << "Available reference spaces:";
        for (auto type : mReferenceSpaceTypes)
            Log(Debug::Verbose) << "  " << to_string(type);

        XrReferenceSpaceCreateInfo createInfo{};
        createInfo.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
        createInfo.poseInReferenceSpace.orientation.w = 1.f; // Identity pose

        createInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
        CHECK_XRCMD(xrCreateReferenceSpace(mXrSession, &createInfo, &mReferenceSpaceView));
        Debugging::setName(mReferenceSpaceView, "OpenMW XR Reference Space View");

        createInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        CHECK_XRCMD(xrCreateReferenceSpace(mXrSession, &createInfo, &mReferenceSpaceStage));
        Debugging::setName(mReferenceSpaceStage, "OpenMW XR Reference Space Stage");

        createInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        CHECK_XRCMD(xrCreateReferenceSpace(mXrSession, &createInfo, &mReferenceSpaceLocal));
        Debugging::setName(mReferenceSpaceLocal, "OpenMW XR Reference Space Local");
    }

    void Session::logXrReferenceSpaces()
    {
        uint32_t spaceCount = 0;
        CHECK_XRCMD(xrEnumerateReferenceSpaces(mXrSession, 0, &spaceCount, nullptr));
        std::vector<XrReferenceSpaceType> spaces(spaceCount);
        CHECK_XRCMD(xrEnumerateReferenceSpaces(mXrSession, spaceCount, &spaceCount, spaces.data()));

        std::stringstream ss;
        ss << "Available reference spaces=" << spaceCount << std::endl;

        for (XrReferenceSpaceType space : spaces)
            ss << "  Name: " << to_string(space) << std::endl;
        Log(Debug::Verbose) << ss.str();
    }

    XrSpace Session::getReferenceSpace(XrReferenceSpaceType space)
    {
        switch (space)
        {
            case XR_REFERENCE_SPACE_TYPE_VIEW:
                return mReferenceSpaceView;
            case XR_REFERENCE_SPACE_TYPE_LOCAL:
                return mReferenceSpaceLocal;
            case XR_REFERENCE_SPACE_TYPE_STAGE:
                return mReferenceSpaceStage;
        }
        return XR_NULL_HANDLE;
    }

    std::array<Stereo::View, 2> Session::locateViews(int64_t predictedDisplayTime, XrReferenceSpaceType space)
    {
        std::array<XrView, 2> xrViews{};
        xrViews[0].type = XR_TYPE_VIEW;
        xrViews[1].type = XR_TYPE_VIEW;

        XrViewState viewState{};
        viewState.type = XR_TYPE_VIEW_STATE;
        uint32_t viewCount = 2;

        XrViewLocateInfo viewLocateInfo{};
        viewLocateInfo.type = XR_TYPE_VIEW_LOCATE_INFO;
        viewLocateInfo.viewConfigurationType = mViewConfigType;
        viewLocateInfo.displayTime = predictedDisplayTime;
        viewLocateInfo.space = getReferenceSpace(space);
        CHECK_XRCMD(xrLocateViews(mXrSession, &viewLocateInfo, &viewState, viewCount, &viewCount, xrViews.data()));

        std::array<Stereo::View, 2> vrViews{};
        for (auto side : { VR::Side_Left, VR::Side_Right })
        {
            vrViews[side].pose = fromXR(xrViews[side].pose);
            vrViews[side].fov = fromXR(xrViews[side].fov);
        }
        return vrViews;
    }

    VR::TrackingPose Session::locateSpace(XrSpace space, XrSpace referenceSpace)
    {
        VR::TrackingPose pose = {};
        pose.status = VR::TrackingStatus::Good;
        pose.time = VR::getPredictedDisplayTime();
        XrSpaceLocation location{};
        location.type = XR_TYPE_SPACE_LOCATION;
        auto res = xrLocateSpace(space, referenceSpace, pose.time, &location);

        if (XR_FAILED(res))
        {
            // Call failed, exit.
            CHECK_XRRESULT(res, "xrLocateSpace");
            pose.status = VR::TrackingStatus::RuntimeFailure;
            return pose;
        }

        // Check that everything is being tracked
        if (!(location.locationFlags
                & (XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT | XR_SPACE_LOCATION_POSITION_TRACKED_BIT)))
        {
            // It's not, data is stale
            pose.status = VR::TrackingStatus::Stale;
        }

        // Check that data is valid
        if (!(location.locationFlags
                & (XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_POSITION_VALID_BIT)))
        {
            // It's not, we've lost tracking
            pose.status = VR::TrackingStatus::Lost;
            return pose;
        }

        pose.pose = fromXR(location.pose);
        return pose;
    }

    VR::TrackingPose Session::locateSpace(XrSpace space, XrReferenceSpaceType referenceSpace)
    {
        return locateSpace(space, getReferenceSpace(referenceSpace));
    }

    std::array<VR::SwapchainConfig, 2> Session::getRecommendedSwapchainConfig() const
    {
        auto xrConfigs = Instance::instance().getRecommendedXrSwapchainConfig();
        std::array<VR::SwapchainConfig, 2> configs{};
        for (uint32_t i = 0; i < 2; i++)
        {
            configs[i].recommendedWidth = xrConfigs[i].recommendedImageRectWidth;
            configs[i].recommendedHeight = xrConfigs[i].recommendedImageRectHeight;
            configs[i].recommendedSamples = xrConfigs[i].recommendedSwapchainSampleCount;
            configs[i].maxWidth = xrConfigs[i].maxImageRectWidth;
            configs[i].maxHeight = xrConfigs[i].maxImageRectHeight;
            configs[i].maxSamples = xrConfigs[i].maxSwapchainSampleCount;
        }

        return configs;
    }
}

