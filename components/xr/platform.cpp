#include <components/misc/strings/lower.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/sceneutil/color.hpp>
#include <components/sceneutil/depth.hpp>

#include "debug.hpp"
#include "instance.hpp"
#include "platform.hpp"
#include "session.hpp"
#include "swapchain.hpp"

// The OpenXR SDK's platform headers assume we've included platform headers
#ifdef _WIN32
#ifdef NOGDI
#undef NOGDI
#endif
#include <Windows.h>
#include <objbase.h>

#ifdef XR_USE_GRAPHICS_API_D3D11
#include <d3d11_1.h>
#endif

#elif __linux__
#include <GL/glx.h>
#undef None

#else
#error Unsupported platform
#endif

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_platform_defines.h>
#include <openxr/openxr_reflection.h>

#include <cassert>
#include <deque>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>

#ifndef GL_DEPTH32F_STENCIL8
#define GL_DEPTH32F_STENCIL8 0x8CAD
#endif

#ifndef GL_DEPTH32F_STENCIL8_NV
#define GL_DEPTH32F_STENCIL8_NV 0x8DAC
#endif

#ifndef GL_DEPTH24_STENCIL8
#define GL_DEPTH24_STENCIL8 0x88F0
#endif

namespace XR
{
    struct PlatformPrivate
    {
        PlatformPrivate(osg::GraphicsContext* gc);
        ~PlatformPrivate();

#ifdef XR_USE_GRAPHICS_API_D3D11
        bool mWGL_NV_DX_interop2 = false;
#endif
    };

    PlatformPrivate::PlatformPrivate(osg::GraphicsContext* gc)
    {
#ifdef XR_USE_GRAPHICS_API_D3D11
        typedef const char*(WINAPI * PFNWGLGETEXTENSIONSSTRINGARBPROC)(HDC hdc);
        PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = 0;
        wglGetExtensionsStringARB
            = reinterpret_cast<PFNWGLGETEXTENSIONSSTRINGARBPROC>(wglGetProcAddress("wglGetExtensionsStringARB"));
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

    PlatformPrivate::~PlatformPrivate() {}

    Platform::Platform(osg::GraphicsContext* gc)
        : mPrivate(new PlatformPrivate(gc))
    {
        mMWColorFormatsGL.push_back(GL_SRGB8);
        mMWColorFormatsGL.push_back(GL_SRGB8_ALPHA8);
        mMWColorFormatsGL.push_back(GL_RGB10);
        mMWColorFormatsGL.push_back(GL_RGB10_A2);
        mMWColorFormatsGL.push_back(GL_RGB10_A2UI);
        mMWColorFormatsGL.push_back(GL_R11F_G11F_B10F);
        mMWColorFormatsGL.push_back(GL_RGB16F);
        mMWColorFormatsGL.push_back(GL_RGBA16F);
        mMWColorFormatsGL.push_back(GL_RGB16);
        mMWColorFormatsGL.push_back(GL_RGB16I);
        mMWColorFormatsGL.push_back(GL_RGB16UI);
        mMWColorFormatsGL.push_back(GL_RGB16_SNORM);
        mMWColorFormatsGL.push_back(GL_RGBA16);
        mMWColorFormatsGL.push_back(GL_RGBA16I);
        mMWColorFormatsGL.push_back(GL_RGBA16UI);
        mMWColorFormatsGL.push_back(GL_RGBA16_SNORM);
        mMWColorFormatsGL.push_back(GL_RGB8);
        mMWColorFormatsGL.push_back(GL_RGB8I);
        mMWColorFormatsGL.push_back(GL_RGB8UI);
        mMWColorFormatsGL.push_back(GL_RGBA8);
        mMWColorFormatsGL.push_back(GL_RGBA8I);
        mMWColorFormatsGL.push_back(GL_RGBA8UI);
        mMWColorFormatsGL.push_back(GL_RGBA8_SNORM);

        mMWDepthFormatsGL.push_back(GL_DEPTH_COMPONENT32F);
        mMWDepthFormatsGL.push_back(GL_DEPTH_COMPONENT32F_NV);
        mMWDepthFormatsGL.push_back(GL_DEPTH32F_STENCIL8);
        mMWDepthFormatsGL.push_back(GL_DEPTH32F_STENCIL8_NV);
        mMWDepthFormatsGL.push_back(GL_DEPTH24_STENCIL8);
        mMWDepthFormatsGL.push_back(GL_DEPTH_COMPONENT32);
        mMWDepthFormatsGL.push_back(GL_DEPTH_COMPONENT24);
        mMWDepthFormatsGL.push_back(GL_DEPTH_COMPONENT16);
    }

    Platform::~Platform() {}

    bool Platform::selectDirectX()
    {
#ifdef XR_USE_GRAPHICS_API_D3D11
        if (mPrivate->mWGL_NV_DX_interop2)
        {
            if (KHR_D3D11_enable.supported())
            {
                Extensions::instance().selectGraphicsAPIExtension(&KHR_D3D11_enable);
                return true;
            }
            else
                Log(Debug::Warning) << "Warning: Failed to select DirectX swapchains: OpenXR runtime does not support "
                                       "essential extension '"
                                    << KHR_D3D11_enable.name() << "'";
        }
        else
            Log(Debug::Warning) << "Warning: Failed to select DirectX swapchains: Essential WGL extension "
                                   "'WGL_NV_DX_interop2' not supported by the graphics driver.";
#endif
        return false;
    }

    bool Platform::selectOpenGL()
    {
#ifdef XR_USE_GRAPHICS_API_OPENGL
        if (KHR_opengl_enable.supported())
        {
            Extensions::instance().selectGraphicsAPIExtension(&KHR_opengl_enable);
            return true;
        }
        else
            Log(Debug::Warning)
                << "Warning: Failed to select OpenGL swapchains: OpenXR runtime does not support essential extension '"
                << KHR_opengl_enable.name() << "'";
#endif
        return false;
    }

    void Platform::selectGraphicsAPIExtension()
    {
        if (selectDirectX() || selectOpenGL())
            return;

        Log(Debug::Verbose) << "Error: No graphics API supported by OpenMW VR is supported by the OpenXR runtime.";
        throw std::runtime_error("Error: No graphics API supported by OpenMW VR is supported by the OpenXR runtime.");
    }

    static std::vector<GLenum> getBlacklistedFormatsForSystem(const std::string& systemName)
    {
        // The format blacklist is a list in the format systemName:formatEnum ...
        // e.g. steamvr:0x8059 steamvr:0x906F steamvr:0x8052
        // if system name is omitted, assume format should be blacklisted always
        auto list = Settings::Manager::getString("disable texture format", "VR Debug");

        std::vector<std::string> formats;
        Misc::StringUtils::split(list, formats, " \t");

        std::vector<GLenum> blacklistedFormats;

        for (auto format : formats)
        {
            auto blacklistSystemNameEnd = format.find_first_of(':');
            if (blacklistSystemNameEnd != std::string::npos && blacklistSystemNameEnd != 0)
            {
                // System name was included, split it out and check
                auto blacklistSystemNameLowerCase
                    = Misc::StringUtils::lowerCase(format.substr(0, blacklistSystemNameEnd));
                auto systemNameLowerCase = Misc::StringUtils::lowerCase(systemName);

                // Rather than an exact comparison, i check if the system name identified
                // by the blacklist entry is a substring of the system name. case insensitive
                if (systemNameLowerCase.find(blacklistSystemNameLowerCase) == std::string::npos)
                    // It isn't, do not add this to the blacklist
                    continue;

                format = format.substr(blacklistSystemNameEnd + 1);
            }

            if (format.empty())
                continue;

            GLenum formatValue = 0;
            std::istringstream iss(format);
            // using std::setbase(0) should let istringstream automatically parse hex vs dec correctly.
            iss >> std::setbase(0) >> formatValue;

            if (formatValue != 0)
                blacklistedFormats.push_back(formatValue);
        }

        return blacklistedFormats;
    }

    XrSession Platform::createXrSession(
        XrInstance instance, XrSystemId systemId, [[maybe_unused]] const std::string& systemName)
    {
        XrSession session = XR_NULL_HANDLE;
        XrResult res = XR_SUCCESS;
        std::string apiName = "";
#ifdef _WIN32
        if (KHR_opengl_enable.enabled())
        {
            apiName = "OpenGL";
            // Get system requirements
            PFN_xrGetOpenGLGraphicsRequirementsKHR p_getRequirements = nullptr;
            CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrGetOpenGLGraphicsRequirementsKHR",
                reinterpret_cast<PFN_xrVoidFunction*>(&p_getRequirements)));
            XrGraphicsRequirementsOpenGLKHR requirements{};
            requirements.type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR;
            CHECK_XRCMD(p_getRequirements(instance, systemId, &requirements));

            auto major = XR_VERSION_MAJOR(requirements.minApiVersionSupported);
            auto minor = XR_VERSION_MINOR(requirements.minApiVersionSupported);
            auto patch = XR_VERSION_PATCH(requirements.minApiVersionSupported);

            Log(Debug::Verbose) << "Runtime requires OpenGL version " << major << "." << minor << "." << patch;
            auto versionString = glGetString(GL_VERSION);
            Log(Debug::Verbose) << "System version: " << versionString;

            // TODO: Actually get system version
            const XrVersion systemApiVersion = XR_MAKE_VERSION(4, 6, 0);
            if (requirements.minApiVersionSupported > systemApiVersion)
            {
                std::cout << "Runtime does not support desired Graphics API and/or version" << std::endl;
            }

            // Create Session
            auto DC = wglGetCurrentDC();
            auto GLRC = wglGetCurrentContext();

            XrGraphicsBindingOpenGLWin32KHR graphicsBindings{};
            graphicsBindings.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR;
            graphicsBindings.hDC = DC;
            graphicsBindings.hGLRC = GLRC;

            if (!graphicsBindings.hDC)
                Log(Debug::Warning) << "Missing DC";

            XrSessionCreateInfo createInfo{};
            createInfo.type = XR_TYPE_SESSION_CREATE_INFO;
            createInfo.next = &graphicsBindings;
            createInfo.systemId = systemId;
            res = CHECK_XRCMD(xrCreateSession(instance, &createInfo, &session));
        }
#ifdef XR_USE_GRAPHICS_API_D3D11
        else if (KHR_D3D11_enable.enabled())
        {
            apiName = "D3D11";
            mDxInterop = std::make_shared<VR::DirectXWGLInterop>();
            PFN_xrGetD3D11GraphicsRequirementsKHR p_getRequirements = nullptr;
            CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrGetD3D11GraphicsRequirementsKHR",
                reinterpret_cast<PFN_xrVoidFunction*>(&p_getRequirements)));
            XrGraphicsRequirementsD3D11KHR requirements{};
            requirements.type = XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR;
            CHECK_XRCMD(p_getRequirements(instance, systemId, &requirements));

            XrGraphicsBindingD3D11KHR d3D11bindings{};
            d3D11bindings.type = XR_TYPE_GRAPHICS_BINDING_D3D11_KHR;
            d3D11bindings.device = reinterpret_cast<ID3D11Device*>(mDxInterop->d3d11DeviceHandle());
            XrSessionCreateInfo createInfo{};
            createInfo.type = XR_TYPE_SESSION_CREATE_INFO;
            createInfo.next = &d3D11bindings;
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
            apiName = "OpenGL";
            // Get system requirements
            PFN_xrGetOpenGLGraphicsRequirementsKHR p_getRequirements = nullptr;
            xrGetInstanceProcAddr(instance, "xrGetOpenGLGraphicsRequirementsKHR",
                reinterpret_cast<PFN_xrVoidFunction*>(&p_getRequirements));
            XrGraphicsRequirementsOpenGLKHR requirements{};
            requirements.type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR;
            CHECK_XRCMD(p_getRequirements(instance, systemId, &requirements));

            // TODO: Actually get system version
            const XrVersion systemApiVersion = XR_MAKE_VERSION(4, 6, 0);
            if (requirements.minApiVersionSupported > systemApiVersion)
            {
                std::cout << "Runtime does not support desired Graphics API and/or version" << std::endl;
            }

            // Create Session
            GLXContext glxContext = glXGetCurrentContext();
            GLXDrawable glxDrawable = glXGetCurrentDrawable();
            Display* xDisplay = glXGetCurrentDisplay();

            // TODO: runtimes don't actually care (yet)
            GLXFBConfig glxFBConfig = 0;
            uint32_t visualid = 0;

            XrGraphicsBindingOpenGLXlibKHR graphicsBindings{};
            graphicsBindings.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR;
            graphicsBindings.xDisplay = xDisplay;
            graphicsBindings.glxContext = glxContext;
            graphicsBindings.glxDrawable = glxDrawable;
            graphicsBindings.glxFBConfig = glxFBConfig;
            graphicsBindings.visualid = visualid;

            if (!graphicsBindings.glxContext)
                Log(Debug::Warning) << "Missing glxContext";

            if (!graphicsBindings.glxDrawable)
                Log(Debug::Warning) << "Missing glxDrawable";

            XrSessionCreateInfo createInfo{};
            createInfo.type = XR_TYPE_SESSION_CREATE_INFO;
            createInfo.next = &graphicsBindings;
            createInfo.systemId = systemId;
            res = CHECK_XRCMD(xrCreateSession(instance, &createInfo, &session));
        }
#endif
        if (!XR_SUCCEEDED(res))
            initFailure(res, instance);

        uint32_t swapchainFormatCount;
        std::vector<int64_t> swapchainFormats;
        CHECK_XRCMD(xrEnumerateSwapchainFormats(session, 0, &swapchainFormatCount, nullptr));
        swapchainFormats.resize(swapchainFormatCount);
        CHECK_XRCMD(xrEnumerateSwapchainFormats(
            session, (uint32_t)swapchainFormats.size(), &swapchainFormatCount, swapchainFormats.data()));

        {
            std::stringstream ss;
            ss << "Available " << apiName << " Swapchain formats : (" << swapchainFormatCount << ")" << std::endl;

            for (auto format : swapchainFormats)
            {
                ss << "  Enum=" << std::dec << format << " (0x=" << std::hex << format << ")" << std::dec << std::endl;
            }
            Log(Debug::Verbose) << ss.str();
        }

#ifdef XR_USE_GRAPHICS_API_D3D11
        // Need to translate DXGI formats to GL equivalents for texture sharing to work.
        // This discards any DXGI formats that do not have exact GL equivalents.
        if (KHR_D3D11_enable.enabled())
        {
            std::stringstream ss;
            ss << "Available GL convertible Swapchain formats: " << std::endl;
            for (auto dxFormat : swapchainFormats)
            {
                auto glFormat = VR::DXGIFormatToGLFormat(dxFormat);
                if (glFormat)
                {
                    ss << "  glFormat=" << std::dec << glFormat << " (0x=" << std::hex << glFormat << ")" << std::dec;
                    ss << "  from dxFormat=" << std::dec << dxFormat << " (0x=" << std::hex << dxFormat << ")"
                       << std::dec;
                    ss << std::endl;
                }
                mRuntimeFormatsGL.push_back(glFormat);
                mRuntimeFormatsDX.push_back(dxFormat);
            }
            Log(Debug::Verbose) << ss.str();
        }
        else
#endif
            mRuntimeFormatsGL.insert(mRuntimeFormatsGL.end(), swapchainFormats.begin(), swapchainFormats.end());

        auto blacklistedFormats = getBlacklistedFormatsForSystem(systemName);

        for (auto it = mRuntimeFormatsGL.begin(); it != mRuntimeFormatsGL.end();)
        {
            auto found = std::find(blacklistedFormats.begin(), blacklistedFormats.end(), *it);
            if (found != blacklistedFormats.end())
            {
                std::stringstream ss;
                ss << "Disabling glFormat=" << std::dec << *it << " (0x=" << std::hex << *it
                   << "), blacklisted by config" << std::dec;
                Log(Debug::Verbose) << ss.str();
                it = mRuntimeFormatsGL.erase(it);
            }
            else
                it++;
        }

        // Discard all color formats not supported by the OpenXR runtime
        for (auto it = mMWColorFormatsGL.begin(); it != mMWColorFormatsGL.end();)
        {
            auto found = std::find(mRuntimeFormatsGL.begin(), mRuntimeFormatsGL.end(), *it);
            if (found == mRuntimeFormatsGL.end())
                it = mMWColorFormatsGL.erase(it);
            else
                it++;
        }

        // Do the same for depth formats
        for (auto it = mMWDepthFormatsGL.begin(); it != mMWDepthFormatsGL.end();)
        {
            auto found = std::find(mRuntimeFormatsGL.begin(), mRuntimeFormatsGL.end(), *it);
            if (found == mRuntimeFormatsGL.end())
                it = mMWDepthFormatsGL.erase(it);
            else
                it++;
        }

        return session;
    }

    std::pair<int64_t, GLenum> Platform::selectSwapchainFormat(
        VR::Swapchain::Attachment attachment)
    {
        std::string typeString = attachment == VR::Swapchain::Attachment::Color ? "color" : "depth";
        GLenum glFormat = 0;
        if (attachment == VR::Swapchain::Attachment::Color)
        {
            if (mMWColorFormatsGL.empty())
                throw std::runtime_error("Could not find a usable runtime color format");
            glFormat = SceneUtil::Color::colorInternalFormat();
        }
        else
        {
            if (mMWDepthFormatsGL.empty())
                throw std::runtime_error("Could not find a usable runtime depth format");

            glFormat = SceneUtil::AutoDepth::depthInternalFormat();
            if (!Settings::Manager::getBool("submit stencil formats", "VR Debug"))
            {
                auto nonStencilFormat = SceneUtil::getDepthFormatOfDepthStencilFormat(glFormat);
                if (std::find(mMWDepthFormatsGL.begin(), mMWDepthFormatsGL.end(), nonStencilFormat)
                    != mMWDepthFormatsGL.end())
                {
                    glFormat = nonStencilFormat;
                }
                else
                {
                    Log(Debug::Warning) << "Submit stencil formats = false, but the non-stencil format was not "
                                           "supported while the stencil format was. Using the stencil format anyway.";
                }
            }

            if (std::find(mMWDepthFormatsGL.begin(), mMWDepthFormatsGL.end(), glFormat) == mMWDepthFormatsGL.end())
                throw std::runtime_error("OpenMW selected an incompatible depth format, cannot submit depth buffers.");
        }
        Log(Debug::Verbose) << "Selected " << typeString << " format: " << std::dec << glFormat << " (" << std::hex
                            << glFormat << ")" << std::dec;

#ifdef XR_USE_GRAPHICS_API_D3D11
        // Need to translate GL format to DXGI
        if (KHR_D3D11_enable.enabled())
        {
            auto dxFormat = VR::GLFormatToDXGIFormat(glFormat);
            Log(Debug::Verbose) << "Translated format to DXGI format: " << std::dec << dxFormat << " (" << std::hex
                                << dxFormat << ")" << std::dec;
            return std::make_pair(dxFormat, glFormat);
        }
#endif
        return std::make_pair(static_cast<int64_t>(glFormat), glFormat);
    }

    std::vector<std::unique_ptr<VR::SwapchainImage>> enumerateSwapchainImagesOpenGL(XrSwapchain swapchain)
    {

        XrSwapchainImageOpenGLKHR xrimage{};
        xrimage.type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;

        uint32_t imageCount = 0;
        std::vector<XrSwapchainImageOpenGLKHR> xrimages;
        CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain, 0, &imageCount, nullptr));
        xrimages.resize(imageCount, xrimage);
        CHECK_XRCMD(xrEnumerateSwapchainImages(
            swapchain, imageCount, &imageCount, reinterpret_cast<XrSwapchainImageBaseHeader*>(xrimages.data())));

        std::vector<std::unique_ptr<VR::SwapchainImage>> images;
        for (auto& image : xrimages)
        {
            images.push_back(std::make_unique<VR::SwapchainImageGL>(image.image));
        }

        return images;
    }

#ifdef _WIN32
    std::vector<std::unique_ptr<VR::SwapchainImage>> enumerateSwapchainImagesDirectX(XrSwapchain swapchain,
        uint64_t textureTarget, uint64_t dxFormat, std::shared_ptr<VR::DirectXWGLInterop> dxInterop)
    {
        XrSwapchainImageD3D11KHR xrimage{};
        xrimage.type = XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR;

        uint32_t imageCount = 0;
        std::vector<XrSwapchainImageD3D11KHR> xrimages;
        CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain, 0, &imageCount, nullptr));
        xrimages.resize(imageCount, xrimage);
        CHECK_XRCMD(xrEnumerateSwapchainImages(
            swapchain, imageCount, &imageCount, reinterpret_cast<XrSwapchainImageBaseHeader*>(xrimages.data())));

        std::vector<std::unique_ptr<VR::SwapchainImage>> images;
        for (auto& image : xrimages)
        {
            images.push_back(std::make_unique<VR::SwapchainImageDX>(
                reinterpret_cast<uint64_t>(image.texture), textureTarget, dxFormat, dxInterop));
        }

        return images;
    }
#endif

    std::vector<std::unique_ptr<VR::SwapchainImage>> Platform::enumerateSwapchainImages(
        XrSwapchain swapchain, uint32_t textureTarget, uint64_t swapchainFormat)
    {
        if (KHR_opengl_enable.enabled())
        {
            return enumerateSwapchainImagesOpenGL(swapchain);
        }
#ifdef _WIN32
        else if (KHR_D3D11_enable.enabled())
        {
            return enumerateSwapchainImagesDirectX(swapchain, textureTarget, swapchainFormat, mDxInterop);
        }
#endif
        else
        {
            // TODO: Vulkan swapchains?
            throw std::logic_error("Not implemented");
        }
    }
}
