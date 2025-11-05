#include "myguitexture.hpp"

#include <stdexcept>

#include <osg/StateSet>
#include <osg/Texture2D>
//## VR_PATCH BEGIN
// Change members from osg::Texture2D to osg::Texture
#include <osg/Texture>
//## VR_PATCH END

#include <components/debug/debuglog.hpp>
#include <components/resource/imagemanager.hpp>

namespace MyGUIPlatform
{

    OSGTexture::OSGTexture(const std::string& name, Resource::ImageManager* imageManager)
        : mName(name)
        , mImageManager(imageManager)
        , mFormat(MyGUI::PixelFormat::Unknow)
        , mUsage(MyGUI::TextureUsage::Default)
        , mNumElemBytes(0)
        , mWidth(0)
        , mHeight(0)
    {
    }

//## VR_PATCH BEGIN
    OSGTexture::OSGTexture(osg::Texture* texture, osg::StateSet* injectState)
//## VR_PATCH END
        : mImageManager(nullptr)
        , mTexture(texture)
        , mInjectState(injectState)
        , mFormat(MyGUI::PixelFormat::Unknow)
        , mUsage(MyGUI::TextureUsage::Default)
        , mNumElemBytes(0)
        , mWidth(texture->getTextureWidth())
        , mHeight(texture->getTextureHeight())
    {
    }

    OSGTexture::~OSGTexture() {}

    void OSGTexture::createManual(int width, int height, MyGUI::TextureUsage usage, MyGUI::PixelFormat format)
    {
        GLenum glfmt = GL_NONE;
        size_t numelems = 0;
        switch (format.getValue())
        {
            case MyGUI::PixelFormat::L8:
                glfmt = GL_LUMINANCE;
                numelems = 1;
                break;
            case MyGUI::PixelFormat::L8A8:
                glfmt = GL_LUMINANCE_ALPHA;
                numelems = 2;
                break;
            case MyGUI::PixelFormat::R8G8B8:
                glfmt = GL_RGB;
                numelems = 3;
                break;
            case MyGUI::PixelFormat::R8G8B8A8:
                glfmt = GL_RGBA;
                numelems = 4;
                break;
        }
        if (glfmt == GL_NONE)
            throw std::runtime_error("Texture format not supported");

//## VR_PATCH BEGIN
        auto* texture2D = new osg::Texture2D();
        mTexture = texture2D;
        texture2D->setTextureSize(width, height);
        texture2D->setSourceFormat(glfmt);
        texture2D->setSourceType(GL_UNSIGNED_BYTE);

        mWidth = width;
        mHeight = height;

        texture2D->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        texture2D->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        texture2D->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        texture2D->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
//## VR_PATCH END

        mFormat = format;
        mUsage = usage;
        mNumElemBytes = numelems;
    }

    void OSGTexture::destroy()
    {
        mTexture = nullptr;
        mFormat = MyGUI::PixelFormat::Unknow;
        mUsage = MyGUI::TextureUsage::Default;
        mNumElemBytes = 0;
        mWidth = 0;
        mHeight = 0;
    }

    void OSGTexture::loadFromFile(const std::string& fname)
    {
        if (!mImageManager)
            throw std::runtime_error("No imagemanager set");

        osg::ref_ptr<osg::Image> image(mImageManager->getImage(VFS::Path::Normalized(fname)));
//## VR_PATCH BEGIN
        auto* texture2D = new osg::Texture2D(image);
        mTexture = texture2D;
        texture2D->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        texture2D->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        texture2D->setTextureWidth(image->s());
        texture2D->setTextureHeight(image->t());
        // disable mip-maps
        texture2D->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
//## VR_PATCH END

        mWidth = image->s();
        mHeight = image->t();

        mUsage = MyGUI::TextureUsage::Static;
    }

    void OSGTexture::saveToFile(const std::string& fname)
    {
        Log(Debug::Warning) << "Would save image to file " << fname;
    }

    void* OSGTexture::lock(MyGUI::TextureUsage /*access*/)
    {
        if (!mTexture.valid())
            throw std::runtime_error("Texture is not created");
        if (mLockedImage.valid())
            throw std::runtime_error("Texture already locked");

        mLockedImage = new osg::Image();
        mLockedImage->allocateImage(mTexture->getTextureWidth(), mTexture->getTextureHeight(),
            mTexture->getTextureDepth(), mTexture->getSourceFormat(), mTexture->getSourceType());

        return mLockedImage->data();
    }

    void OSGTexture::unlock()
    {
        if (!mLockedImage.valid())
            throw std::runtime_error("Texture not locked");

        mLockedImage->flipVertical();

        // mTexture might be in use by the draw thread, so create a new texture instead and use that.
        osg::ref_ptr<osg::Texture2D> newTexture = new osg::Texture2D;
        newTexture->setTextureSize(getWidth(), getHeight());
        newTexture->setSourceFormat(mTexture->getSourceFormat());
        newTexture->setSourceType(mTexture->getSourceType());
        newTexture->setFilter(osg::Texture::MIN_FILTER, mTexture->getFilter(osg::Texture::MIN_FILTER));
        newTexture->setFilter(osg::Texture::MAG_FILTER, mTexture->getFilter(osg::Texture::MAG_FILTER));
        newTexture->setWrap(osg::Texture::WRAP_S, mTexture->getWrap(osg::Texture::WRAP_S));
        newTexture->setWrap(osg::Texture::WRAP_T, mTexture->getWrap(osg::Texture::WRAP_T));
        newTexture->setImage(mLockedImage.get());
        // Tell the texture it can get rid of the image for static textures (since
        // they aren't expected to update much at all).
        newTexture->setUnRefImageDataAfterApply(mUsage.isValue(MyGUI::TextureUsage::Static) ? true : false);

        mTexture = newTexture;

        mLockedImage = nullptr;
    }

    // Render-to-texture not currently implemented.
    MyGUI::IRenderTarget* OSGTexture::getRenderTarget()
    {
        return nullptr;
    }

    void OSGTexture::setShader(const std::string& /*shaderName*/)
    {
        Log(Debug::Warning) << "OSGTexture::setShader is not implemented";
    }
}
