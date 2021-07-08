#include "openxrswapchainimage.hpp"
#include "openxrmanagerimpl.hpp"
#include "openxrplatform.hpp"
#include "vrenvironment.hpp"

// The OpenXR SDK's platform headers assume we've included platform headers
#ifdef _WIN32
#include <Windows.h>
#include <objbase.h>

#ifdef XR_USE_GRAPHICS_API_D3D11
#include <d3d11_1.h>
#endif

#elif __linux__
#include <X11/Xlib.h>
#include <GL/glx.h>
#undef None

#else
#error Unsupported platform
#endif

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_platform_defines.h>
#include <openxr/openxr_reflection.h>

#include <stdexcept>
#include <deque>
#include <cassert>
#include <optional>

namespace MWVR
{

    XrResult CheckXrResult(XrResult res, const char* originator, const char* sourceLocation) {
        static bool initialized = false;
        static bool sLogAllXrCalls = false;
        static bool sContinueOnErrors = false;
        if (!initialized)
        {
            initialized = true;
            sLogAllXrCalls = Settings::Manager::getBool("log all openxr calls", "VR Debug");
            sContinueOnErrors = Settings::Manager::getBool("continue on errors", "VR Debug");
        }

        auto resultString = XrResultString(res);

        if (XR_FAILED(res)) {
            std::stringstream ss;
#ifdef _WIN32
            ss << sourceLocation << ": OpenXR[Error: " << resultString << "][Thread: " << std::this_thread::get_id() << "]: " << originator;
#elif __linux__
            ss << sourceLocation << ": OpenXR[Error: " << resultString << "][Thread: " << std::this_thread::get_id() << "]: " << originator;
#endif
            Log(Debug::Error) << ss.str();
            if (!sContinueOnErrors)
                throw std::runtime_error(ss.str().c_str());
        }
        else if (res != XR_SUCCESS || sLogAllXrCalls)
        {
#ifdef _WIN32
            Log(Debug::Verbose) << sourceLocation << ": OpenXR[" << resultString << "][" << std::this_thread::get_id() << "]: " << originator;
#elif __linux__
            Log(Debug::Verbose) << sourceLocation << ": OpenXR[" << resultString << "][" << std::this_thread::get_id() << "]: " << originator;
#endif
        }

        return res;
    }

    void OpenXRPlatform::enumerateExtensions(const char* layerName, int logIndent)
    {
        uint32_t extensionCount = 0;
        std::vector<XrExtensionProperties> availableExtensions;
        CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layerName, 0, &extensionCount, nullptr));
        availableExtensions.resize(extensionCount, XrExtensionProperties{ XR_TYPE_EXTENSION_PROPERTIES });
        CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layerName, availableExtensions.size(), &extensionCount, availableExtensions.data()));

        std::vector<std::string> extensionNames;
        const std::string indentStr(logIndent, ' ');
        for (auto& extension : availableExtensions)
        {
            if (layerName)
                mAvailableLayerExtensions[layerName][extension.extensionName] = extension;
            else
                mAvailableExtensions[extension.extensionName] = extension;

            Log(Debug::Verbose) << indentStr << "Name=" << extension.extensionName << " SpecVersion=" << extension.extensionVersion;
        }
    }

    struct OpenXRPlatformPrivate
    {
        OpenXRPlatformPrivate(osg::GraphicsContext* gc);
        ~OpenXRPlatformPrivate();

#ifdef XR_USE_GRAPHICS_API_D3D11
        typedef BOOL(WINAPI* P_wglDXSetResourceShareHandleNV)(void* dxObject, HANDLE shareHandle);
        typedef HANDLE(WINAPI* P_wglDXOpenDeviceNV)(void* dxDevice);
        typedef BOOL(WINAPI* P_wglDXCloseDeviceNV)(HANDLE hDevice);
        typedef HANDLE(WINAPI* P_wglDXRegisterObjectNV)(HANDLE hDevice, void* dxObject,
            GLuint name, GLenum type, GLenum access);
        typedef BOOL(WINAPI* P_wglDXUnregisterObjectNV)(HANDLE hDevice, HANDLE hObject);
        typedef BOOL(WINAPI* P_wglDXObjectAccessNV)(HANDLE hObject, GLenum access);
        typedef BOOL(WINAPI* P_wglDXLockObjectsNV)(HANDLE hDevice, GLint count, HANDLE* hObjects);
        typedef BOOL(WINAPI* P_wglDXUnlockObjectsNV)(HANDLE hDevice, GLint count, HANDLE* hObjects);

        void initializeD3D11(XrGraphicsRequirementsD3D11KHR requirements)
        {
            mD3D11Dll = LoadLibrary("D3D11.dll");

            if (!mD3D11Dll)
                throw std::runtime_error("Current OpenXR runtime requires DirectX >= 11.0 but D3D11.dll was not found.");

            pD3D11CreateDevice = reinterpret_cast<PFN_D3D11_CREATE_DEVICE>(GetProcAddress(mD3D11Dll, "D3D11CreateDevice"));

            if (!pD3D11CreateDevice)
                throw std::runtime_error("Symbol 'D3D11CreateDevice' not found in D3D11.dll");

            // Create the device and device context objects
            pD3D11CreateDevice(
                nullptr,
                D3D_DRIVER_TYPE_HARDWARE,
                nullptr,
                0,
                nullptr,
                0,
                D3D11_SDK_VERSION,
                &mD3D11Device,
                nullptr,
                &mD3D11ImmediateContext);

            mD3D11bindings.device = mD3D11Device;

            //typedef HANDLE (WINAPI* P_wglDXOpenDeviceNV)(void* dxDevice);
            //P_wglDXOpenDeviceNV wglDXOpenDeviceNV = reinterpret_cast<P_wglDXOpenDeviceNV>(wglGetProcAddress("wglDXOpenDeviceNV"));
            //P_wglDXOpenDeviceNV wglDXOpenDeviceNV = reinterpret_cast<P_wglDXOpenDeviceNV>(wglGetProcAddress("wglDXOpenDeviceNV"));

#define LOAD_WGL(a) a = reinterpret_cast<decltype(a)>(wglGetProcAddress(#a)); if(!a) throw std::runtime_error("Extension WGL_NV_DX_interop2 required to run OpenMW VR via DirectX missing expected symbol '" #a "'.")
            LOAD_WGL(wglDXSetResourceShareHandleNV);
            LOAD_WGL(wglDXOpenDeviceNV);
            LOAD_WGL(wglDXCloseDeviceNV);
            LOAD_WGL(wglDXRegisterObjectNV);
            LOAD_WGL(wglDXUnregisterObjectNV);
            LOAD_WGL(wglDXObjectAccessNV);
            LOAD_WGL(wglDXLockObjectsNV);
            LOAD_WGL(wglDXUnlockObjectsNV);
#undef LOAD_WGL

            wglDXDevice = wglDXOpenDeviceNV(mD3D11Device);
        }

        bool mWGL_NV_DX_interop2 = false;
        XrGraphicsBindingD3D11KHR mD3D11bindings{ XR_TYPE_GRAPHICS_BINDING_D3D11_KHR };
        ID3D11Device* mD3D11Device = nullptr;
        ID3D11DeviceContext* mD3D11ImmediateContext = nullptr;
        HMODULE mD3D11Dll = nullptr;
        PFN_D3D11_CREATE_DEVICE pD3D11CreateDevice = nullptr;

        P_wglDXSetResourceShareHandleNV wglDXSetResourceShareHandleNV = nullptr;
        P_wglDXOpenDeviceNV wglDXOpenDeviceNV = nullptr;
        P_wglDXCloseDeviceNV wglDXCloseDeviceNV = nullptr;
        P_wglDXRegisterObjectNV wglDXRegisterObjectNV = nullptr;
        P_wglDXUnregisterObjectNV wglDXUnregisterObjectNV = nullptr;
        P_wglDXObjectAccessNV wglDXObjectAccessNV = nullptr;
        P_wglDXLockObjectsNV wglDXLockObjectsNV = nullptr;
        P_wglDXUnlockObjectsNV wglDXUnlockObjectsNV = nullptr;

        HANDLE wglDXDevice = nullptr;
#endif
    };

    OpenXRPlatformPrivate::OpenXRPlatformPrivate(osg::GraphicsContext* gc)
    {
#ifdef XR_USE_GRAPHICS_API_D3D11
        typedef const char* (WINAPI* PFNWGLGETEXTENSIONSSTRINGARBPROC) (HDC hdc);
        PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = 0;
        wglGetExtensionsStringARB = reinterpret_cast<PFNWGLGETEXTENSIONSSTRINGARBPROC>(wglGetProcAddress("wglGetExtensionsStringARB"));
        if (wglGetExtensionsStringARB)
        {
            std::string wglExtensions = wglGetExtensionsStringARB(wglGetCurrentDC());
            Log(Debug::Verbose) << "WGL Extensions: " << wglExtensions;
            mWGL_NV_DX_interop2 = wglExtensions.find("WGL_NV_DX_interop2") != std::string::npos;
        }
        else
            Log(Debug::Verbose) << "Unable to query WGL extensions";
#endif
    }

    OpenXRPlatformPrivate::~OpenXRPlatformPrivate()
    {
#ifdef XR_USE_GRAPHICS_API_D3D11
        if (wglDXDevice)
            wglDXCloseDeviceNV(wglDXDevice);
        if (mD3D11ImmediateContext)
            mD3D11ImmediateContext->Release();
        if (mD3D11Device)
            mD3D11Device->Release();
        if (mD3D11Dll)
            FreeLibrary(mD3D11Dll);
#endif
    }

    OpenXRPlatform::OpenXRPlatform(osg::GraphicsContext* gc)
        : mPrivate(new OpenXRPlatformPrivate(gc))
    {
        // Enumerate layers and their extensions.
        uint32_t layerCount;
        CHECK_XRCMD(xrEnumerateApiLayerProperties(0, &layerCount, nullptr));
        std::vector<XrApiLayerProperties> layers(layerCount, XrApiLayerProperties{ XR_TYPE_API_LAYER_PROPERTIES });
        CHECK_XRCMD(xrEnumerateApiLayerProperties((uint32_t)layers.size(), &layerCount, layers.data()));

        Log(Debug::Verbose) << "Available Extensions: ";
        enumerateExtensions(nullptr, 2);
        Log(Debug::Verbose) << "Available Layers: ";

        if (layers.size() == 0)
        {
            Log(Debug::Verbose) << "  No layers available";
        }
        for (const XrApiLayerProperties& layer : layers) {
            Log(Debug::Verbose) << "  Name=" << layer.layerName << " SpecVersion=" << layer.layerVersion;
            mAvailableLayers[layer.layerName] = layer;
            enumerateExtensions(layer.layerName, 4);
        }

        setupExtensions();
    }

    OpenXRPlatform::~OpenXRPlatform()
    {
    }

#if    !XR_KHR_composition_layer_depth \
    || !XR_EXT_hp_mixed_reality_controller \
    || !XR_EXT_debug_utils \
    || !XR_HTC_vive_cosmos_controller_interaction \
    || !XR_HUAWEI_controller_interaction

#error "OpenXR extensions missing. Please upgrade your copy of the OpenXR SDK to 1.0.13 minimum"
#endif
    void OpenXRPlatform::setupExtensions()
    {
        std::vector<const char*> optionalExtensions = {
            XR_EXT_HP_MIXED_REALITY_CONTROLLER_EXTENSION_NAME,
            XR_HTC_VIVE_COSMOS_CONTROLLER_INTERACTION_EXTENSION_NAME,
            XR_HUAWEI_CONTROLLER_INTERACTION_EXTENSION_NAME
        };

        if (Settings::Manager::getBool("enable XR_KHR_composition_layer_depth", "VR Debug"))
            optionalExtensions.emplace_back(XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME);

        if (Settings::Manager::getBool("enable XR_EXT_debug_utils", "VR Debug"))
            optionalExtensions.emplace_back(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);

        selectGraphicsAPIExtension();

        Log(Debug::Verbose) << "Using extensions:";

        auto* graphicsAPIExtension = graphicsAPIExtensionName();
        if (!graphicsAPIExtension || !enableExtension(graphicsAPIExtension, true))
        {
            throw std::runtime_error("No graphics APIs supported by openmw are supported by the OpenXR runtime.");
        }

        for (auto optionalExtension : optionalExtensions)
            enableExtension(optionalExtension, true);
    }

    bool OpenXRPlatform::selectDirectX()
    {
#ifdef XR_USE_GRAPHICS_API_D3D11
        if (mPrivate->mWGL_NV_DX_interop2)
        {
            if (mAvailableExtensions.count(XR_KHR_D3D11_ENABLE_EXTENSION_NAME))
            {
                mGraphicsAPIExtension = XR_KHR_D3D11_ENABLE_EXTENSION_NAME;
                return true;
            }
            else
                Log(Debug::Warning) << "Warning: Failed to select DirectX swapchains: OpenXR runtime does not support essential extension '" << XR_KHR_D3D11_ENABLE_EXTENSION_NAME << "'";
        }
        else
            Log(Debug::Warning) << "Warning: Failed to select DirectX swapchains: Essential WGL extension 'WGL_NV_DX_interop2' not supported by the graphics driver.";
#endif
        return false;
    }

    bool OpenXRPlatform::selectOpenGL()
    {
#ifdef XR_USE_GRAPHICS_API_OPENGL
        if (mAvailableExtensions.count(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME))
        {
            mGraphicsAPIExtension = XR_KHR_OPENGL_ENABLE_EXTENSION_NAME;
            return true;
        }
        else
            Log(Debug::Warning) << "Warning: Failed to select OpenGL swapchains: OpenXR runtime does not support essential extension '" << XR_KHR_OPENGL_ENABLE_EXTENSION_NAME << "'";
#endif
        return false;
    }

#ifdef XR_USE_GRAPHICS_API_OPENGL
#if    !XR_KHR_opengl_enable
#error "OpenXR extensions missing. Please upgrade your copy of the OpenXR SDK to 1.0.13 minimum"
#endif
#endif

#ifdef XR_USE_GRAPHICS_API_D3D11
#if    !XR_KHR_D3D11_enable
#error "OpenXR extensions missing. Please upgrade your copy of the OpenXR SDK to 1.0.13 minimum"
#endif
#endif

    const char* OpenXRPlatform::graphicsAPIExtensionName()
    {
        return mGraphicsAPIExtension;
    }

    void OpenXRPlatform::selectGraphicsAPIExtension()
    {
        bool preferDirectX = Settings::Manager::getBool("Prefer DirectX swapchains", "VR");

        if (preferDirectX)
            if (selectDirectX() || selectOpenGL())
                return;
        if (selectOpenGL() || selectDirectX())
            return;

        Log(Debug::Verbose) << "Error: No graphics API supported by OpenMW VR is supported by the OpenXR runtime.";
        throw std::runtime_error("Error: No graphics API supported by OpenMW VR is supported by the OpenXR runtime.");
    }

    bool OpenXRPlatform::supportsExtension(const std::string& extensionName) const
    {
        return mAvailableExtensions.count(extensionName) > 0;
    }

    bool OpenXRPlatform::supportsExtension(const std::string& extensionName, uint32_t minimumVersion) const
    {
        auto it = mAvailableExtensions.find(extensionName);
        return it != mAvailableExtensions.end() && it->second.extensionVersion > minimumVersion;
    }

    bool OpenXRPlatform::supportsLayer(const std::string& layerName) const
    {
        return mAvailableLayers.count(layerName) > 0;
    }

    bool OpenXRPlatform::supportsLayer(const std::string& layerName, uint32_t minimumVersion) const
    {
        auto it = mAvailableLayers.find(layerName);
        return it != mAvailableLayers.end() && it->second.layerVersion > minimumVersion;
    }

    bool OpenXRPlatform::enableExtension(const std::string& extensionName, bool optional)
    {
        auto it = mAvailableExtensions.find(extensionName);
        if (it != mAvailableExtensions.end())
        {
            Log(Debug::Verbose) << "  " << extensionName << ": enabled";
            mEnabledExtensions.push_back(it->second.extensionName);
            return true;
        }
        else
        {
            Log(Debug::Verbose) << "  " << extensionName << ": disabled (not supported)";
            if (!optional)
            {
                throw std::runtime_error(std::string("Required OpenXR extension ") + extensionName + " not supported by the runtime");
            }
            return false;
        }
    }

    bool OpenXRPlatform::enableExtension(const std::string& extensionName, bool optional, uint32_t minimumVersion)
    {
        auto it = mAvailableExtensions.find(extensionName);
        if (it != mAvailableExtensions.end() && it->second.extensionVersion > minimumVersion)
        {
            Log(Debug::Verbose) << "  " << extensionName << ": enabled";
            mEnabledExtensions.push_back(it->second.extensionName);
            return true;
        }
        else
        {
            Log(Debug::Verbose) << "  " << extensionName << ": disabled (not supported)";
            if (!optional)
            {
                throw std::runtime_error(std::string("Required OpenXR extension ") + extensionName + " not supported by the runtime");
            }
            return false;
        }
    }
    bool OpenXRPlatform::extensionEnabled(const std::string& extensionName) const
    {
        for (auto* extension : mEnabledExtensions)
            if (extension == extensionName)
                return true;
        return false;
    }
    XrInstance OpenXRPlatform::createXrInstance(const std::string& name)
    {
        XrInstance instance = XR_NULL_HANDLE;
        XrInstanceCreateInfo createInfo{ XR_TYPE_INSTANCE_CREATE_INFO };
        createInfo.next = nullptr;
        createInfo.enabledExtensionCount = mEnabledExtensions.size();
        createInfo.enabledExtensionNames = mEnabledExtensions.data();
        strcpy(createInfo.applicationInfo.applicationName, "openmw_vr");
        createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

        auto res = CHECK_XRCMD(xrCreateInstance(&createInfo, &instance));
        if (!XR_SUCCEEDED(res))
            initFailure(res, instance);
        return instance;
    }

    XrSession OpenXRPlatform::createXrSession(XrInstance instance, XrSystemId systemId)
    {
        XrSession session = XR_NULL_HANDLE;
        XrResult res = XR_SUCCESS;
#ifdef _WIN32
        std::string graphicsAPIExtension = graphicsAPIExtensionName();
        if(graphicsAPIExtension == XR_KHR_OPENGL_ENABLE_EXTENSION_NAME)
        { 
            // Get system requirements
            PFN_xrGetOpenGLGraphicsRequirementsKHR p_getRequirements = nullptr;
            CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrGetOpenGLGraphicsRequirementsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&p_getRequirements)));
            XrGraphicsRequirementsOpenGLKHR requirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
            CHECK_XRCMD(p_getRequirements(instance, systemId, &requirements));

            // TODO: Actually get system version
            const XrVersion systemApiVersion = XR_MAKE_VERSION(4, 6, 0);
            if (requirements.minApiVersionSupported > systemApiVersion) {
                std::cout << "Runtime does not support desired Graphics API and/or version" << std::endl;
            }

            // Create Session
            auto DC = wglGetCurrentDC();
            auto GLRC = wglGetCurrentContext();

            XrGraphicsBindingOpenGLWin32KHR graphicsBindings;
            graphicsBindings.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR;
            graphicsBindings.next = nullptr;
            graphicsBindings.hDC = DC;
            graphicsBindings.hGLRC = GLRC;

            if (!graphicsBindings.hDC)
                Log(Debug::Warning) << "Missing DC";

            XrSessionCreateInfo createInfo{ XR_TYPE_SESSION_CREATE_INFO };
            createInfo.next = &graphicsBindings;
            createInfo.systemId = systemId;
            res = CHECK_XRCMD(xrCreateSession(instance, &createInfo, &session));
        }
#ifdef XR_USE_GRAPHICS_API_D3D11
        else if(graphicsAPIExtension == XR_KHR_D3D11_ENABLE_EXTENSION_NAME)
        {
            PFN_xrGetD3D11GraphicsRequirementsKHR p_getRequirements = nullptr;
            CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrGetD3D11GraphicsRequirementsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&p_getRequirements)));
            XrGraphicsRequirementsD3D11KHR requirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR };
            CHECK_XRCMD(p_getRequirements(instance, systemId, &requirements));
            mPrivate->initializeD3D11(requirements);

            XrSessionCreateInfo createInfo{ XR_TYPE_SESSION_CREATE_INFO };
            createInfo.next = &mPrivate->mD3D11bindings;
            createInfo.systemId = systemId;
            res = CHECK_XRCMD(xrCreateSession(instance, &createInfo, &session));
        }
#endif
        else
        {
            throw std::logic_error("Enum value not implemented");
        }
#elif __linux__
        {
            // Get system requirements
            PFN_xrGetOpenGLGraphicsRequirementsKHR p_getRequirements = nullptr;
            xrGetInstanceProcAddr(instance, "xrGetOpenGLGraphicsRequirementsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&p_getRequirements));
            XrGraphicsRequirementsOpenGLKHR requirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
            CHECK_XRCMD(p_getRequirements(instance, systemId, &requirements));

            // TODO: Actually get system version
            const XrVersion systemApiVersion = XR_MAKE_VERSION(4, 6, 0);
            if (requirements.minApiVersionSupported > systemApiVersion) {
                std::cout << "Runtime does not support desired Graphics API and/or version" << std::endl;
            }

            // Create Session
            Display* xDisplay = XOpenDisplay(NULL);
            GLXContext glxContext = glXGetCurrentContext();
            GLXDrawable glxDrawable = glXGetCurrentDrawable();

            // TODO: runtimes don't actually care (yet)
            GLXFBConfig glxFBConfig = 0;
            uint32_t visualid = 0;

            XrGraphicsBindingOpenGLXlibKHR graphicsBindings;
            graphicsBindings.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR;
            graphicsBindings.next = nullptr;
            graphicsBindings.xDisplay = xDisplay;
            graphicsBindings.glxContext = glxContext;
            graphicsBindings.glxDrawable = glxDrawable;
            graphicsBindings.glxFBConfig = glxFBConfig;
            graphicsBindings.visualid = visualid;

            if (!graphicsBindings.glxContext)
                Log(Debug::Warning) << "Missing glxContext";

            if (!graphicsBindings.glxDrawable)
                Log(Debug::Warning) << "Missing glxDrawable";

            XrSessionCreateInfo createInfo{ XR_TYPE_SESSION_CREATE_INFO };
            createInfo.next = &graphicsBindings;
            createInfo.systemId = systemId;
            res = CHECK_XRCMD(xrCreateSession(instance, &createInfo, &session));
        }
#endif
        if (!XR_SUCCEEDED(res))
            initFailure(res, instance);

        uint32_t swapchainFormatCount;
        CHECK_XRCMD(xrEnumerateSwapchainFormats(session, 0, &swapchainFormatCount, nullptr));
        mSwapchainFormats.resize(swapchainFormatCount);
        CHECK_XRCMD(xrEnumerateSwapchainFormats(session, (uint32_t)mSwapchainFormats.size(), &swapchainFormatCount, mSwapchainFormats.data()));

        std::stringstream ss;
        ss << "Available Swapchain formats: (" << swapchainFormatCount << ")" << std::endl;

        for (auto format : mSwapchainFormats)
        {
            ss << "  Enum=" << std::dec << format << " (0x=" << std::hex << format << ")" << std::dec << std::endl;
        }

        Log(Debug::Verbose) << ss.str();

        return session;
    }

    /*
    * For reference: These are the DXGI formats offered by WMR when using D3D11:
          Enum=29 //(0x=1d) DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
          Enum=91 //(0x=5b) DXGI_FORMAT_B8G8R8A8_UNORM_SRGB
          Enum=28 //(0x=1c) DXGI_FORMAT_R8G8B8A8_UNORM
          Enum=87 //(0x=57) DXGI_FORMAT_B8G8R8A8_UNORM
          Enum=40 //(0x=28) DXGI_FORMAT_D32_FLOAT
          Enum=20 //(0x=14) DXGI_FORMAT_D32_FLOAT_S8X24_UINT
          Enum=45 //(0x=2d) DXGI_FORMAT_D24_UNORM_S8_UINT
          Enum=55 //(0x=37) DXGI_FORMAT_D16_UNORM
    * And these extra formats are offered by SteamVR:
                0xa , // DXGI_FORMAT_R16G16B16A16_FLOAT
                0x18 , // DXGI_FORMAT_R10G10B10A2_UNORM
    */

    int64_t OpenXRPlatform::selectColorFormat()
    {
        std::string graphicsAPIExtension = graphicsAPIExtensionName();
        if (graphicsAPIExtension == XR_KHR_OPENGL_ENABLE_EXTENSION_NAME)
        {
            std::vector<int64_t> requestedColorSwapchainFormats;

            if (Settings::Manager::getBool("Prefer sRGB swapchains", "VR"))
            {
                requestedColorSwapchainFormats.push_back(0x8C43); // GL_SRGB8_ALPHA8
                requestedColorSwapchainFormats.push_back(0x8C41); // GL_SRGB8
            }

            requestedColorSwapchainFormats.push_back(0x8058); // GL_RGBA8
            requestedColorSwapchainFormats.push_back(0x8F97); // GL_RGBA8_SNORM
            requestedColorSwapchainFormats.push_back(0x881A); // GL_RGBA16F
            requestedColorSwapchainFormats.push_back(0x881B); // GL_RGB16F
            requestedColorSwapchainFormats.push_back(0x8C3A); // GL_R11F_G11F_B10F

            return selectFormat(requestedColorSwapchainFormats);
        }
#ifdef XR_USE_GRAPHICS_API_D3D11
        else if (graphicsAPIExtension == XR_KHR_D3D11_ENABLE_EXTENSION_NAME)
        {
            std::vector<int64_t> requestedColorSwapchainFormats;

            if (Settings::Manager::getBool("Prefer sRGB swapchains", "VR"))
            {
                requestedColorSwapchainFormats.push_back(0x1d); // DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
                requestedColorSwapchainFormats.push_back(0x5b); // DXGI_FORMAT_B8G8R8A8_UNORM_SRGB
            }

            requestedColorSwapchainFormats.push_back(0x1c); // DXGI_FORMAT_R8G8B8A8_UNORM
            requestedColorSwapchainFormats.push_back(0x57); // DXGI_FORMAT_B8G8R8A8_UNORM
            requestedColorSwapchainFormats.push_back(0xa); // DXGI_FORMAT_R16G16B16A16_FLOAT
            requestedColorSwapchainFormats.push_back(0x18); // DXGI_FORMAT_R10G10B10A2_UNORM

            return selectFormat(requestedColorSwapchainFormats);
        }
#endif
        else
        {
            throw std::logic_error("Enum value not implemented");
        }

    }

    int64_t OpenXRPlatform::selectDepthFormat()
    {
        std::string graphicsAPIExtension = graphicsAPIExtensionName();
        if (graphicsAPIExtension == XR_KHR_OPENGL_ENABLE_EXTENSION_NAME)
        {
            // Find supported depth swapchain format.
            std::vector<int64_t> requestedDepthSwapchainFormats = {
                0x88F0, // GL_DEPTH24_STENCIL8
                0x8CAC, // GL_DEPTH_COMPONENT32F
                0x81A7, // GL_DEPTH_COMPONENT32
                0x8DAB, // GL_DEPTH_COMPONENT32F_NV
                0x8CAD, // GL_DEPTH32_STENCIL8
                0x81A6, // GL_DEPTH_COMPONENT24
                // Need 32bit minimum: // 0x81A5, // GL_DEPTH_COMPONENT16
            };

            return selectFormat(requestedDepthSwapchainFormats);
        }
#ifdef XR_USE_GRAPHICS_API_D3D11
        else if (graphicsAPIExtension == XR_KHR_D3D11_ENABLE_EXTENSION_NAME)
        {
            // Find supported color swapchain format.
            std::vector<int64_t> requestedDepthSwapchainFormats = {
                0x2d, // DXGI_FORMAT_D24_UNORM_S8_UINT
                0x14, // DXGI_FORMAT_D32_FLOAT_S8X24_UINT
                0x28, // DXGI_FORMAT_D32_FLOAT
                // Need 32bit minimum: 0x37, // DXGI_FORMAT_D16_UNORM
            };
            return selectFormat(requestedDepthSwapchainFormats);
        }
#endif
        else
        {
            throw std::logic_error("Enum value not implemented");
        }
    }

    void OpenXRPlatform::eraseFormat(int64_t format)
    {
        for (auto it = mSwapchainFormats.begin(); it != mSwapchainFormats.end(); it++)
        {
            if (*it == format)
            {
                mSwapchainFormats.erase(it);
                return;
            }
        }
    }

    int64_t OpenXRPlatform::selectFormat(const std::vector<int64_t>& requestedFormats)
    {
        auto it =
            std::find_first_of(std::begin(requestedFormats), std::end(requestedFormats),
                mSwapchainFormats.begin(), mSwapchainFormats.end());
        if (it == std::end(requestedFormats))
        {
            return 0;
        }
        return *it;
    }
    void* OpenXRPlatform::DXRegisterObject(void* dxResource, uint32_t glName, uint32_t glType, bool discard, void* ntShareHandle)
    {
#ifdef XR_USE_GRAPHICS_API_D3D11
        if (ntShareHandle)
        {
            mPrivate->wglDXSetResourceShareHandleNV(dxResource, ntShareHandle);
        }
        return mPrivate->wglDXRegisterObjectNV(mPrivate->wglDXDevice, dxResource, glName, glType, 1);
#else
        return nullptr;
#endif
    }
    void OpenXRPlatform::DXUnregisterObject(void* dxResourceShareHandle)
    {
#ifdef XR_USE_GRAPHICS_API_D3D11
        mPrivate->wglDXUnregisterObjectNV(mPrivate->wglDXDevice, dxResourceShareHandle);
#endif
    }
    void OpenXRPlatform::DXLockObject(void* dxResourceShareHandle)
    {
#ifdef XR_USE_GRAPHICS_API_D3D11
        mPrivate->wglDXLockObjectsNV(mPrivate->wglDXDevice, 1, &dxResourceShareHandle);
#endif
    }
    void OpenXRPlatform::DXUnlockObject(void* dxResourceShareHandle)
    {
#ifdef XR_USE_GRAPHICS_API_D3D11
        mPrivate->wglDXUnlockObjectsNV(mPrivate->wglDXDevice, 1, &dxResourceShareHandle);
#endif
    }

    static XrInstanceProperties
        getInstanceProperties(XrInstance instance)
    {
        XrInstanceProperties properties{ XR_TYPE_INSTANCE_PROPERTIES };
        if (instance)
            xrGetInstanceProperties(instance, &properties);
        return properties;
    }

    std::string OpenXRPlatform::getInstanceName(XrInstance instance)
    {
        if (instance)
            return getInstanceProperties(instance).runtimeName;
        return "unknown";
    }

    XrVersion OpenXRPlatform::getInstanceVersion(XrInstance instance)
    {
        if (instance)
            return getInstanceProperties(instance).runtimeVersion;
        return 0;
    }
    void OpenXRPlatform::initFailure(XrResult res, XrInstance instance)
    {
        std::stringstream ss;
        std::string runtimeName = getInstanceName(instance);
        XrVersion runtimeVersion = getInstanceVersion(instance);
        ss << "Error caught while initializing VR device: " << XrResultString(res) << std::endl;
        ss << "Device: " << runtimeName << std::endl;
        ss << "Version: " << runtimeVersion << std::endl;
        if (res == XR_ERROR_FORM_FACTOR_UNAVAILABLE)
        {
            ss << "Cause: Unable to open VR device. Make sure your device is plugged in and the VR driver is running." << std::endl;
            ss << std::endl;
            if (runtimeName == "Oculus" || runtimeName == "Quest")
            {
                ss << "Your device has been identified as an Oculus device." << std::endl;
                ss << "The most common cause for this error when using an oculus device, is quest users attempting to run the game via Virtual Desktop." << std::endl;
                ss << "Unfortunately this is currently broken, and quest users will need to play via a link cable." << std::endl;
            }
        }
        else if (res == XR_ERROR_LIMIT_REACHED)
        {
            ss << "Cause: Device resources exhausted. Close other VR applications if you have any open. If you have none, you may need to reboot to reset the driver." << std::endl;
        }
        else
        {
            ss << "Cause: Unknown. Make sure your device is plugged in and ready." << std::endl;
        }
        throw std::runtime_error(ss.str());
    }
}
