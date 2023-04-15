#ifndef OPENMW_MWRENDER_UTIL_H
#define OPENMW_MWRENDER_UTIL_H

#include <osg/Camera>
#include <osg/LightModel>
#include <osg/NodeCallback>

#include <string_view>

namespace osg
{
    class Node;
    class Texture2D;
}

namespace Resource
{
    class ResourceSystem;
}

namespace MWRender
{
    // Overrides the texture of nodes in the mesh that had the same NiTexturingProperty as the first NiTexturingProperty
    // of the .NIF file's root node, if it had a NiTexturingProperty. Used for applying "particle textures" to magic
    // effects.
    void overrideFirstRootTexture(std::string_view texture, Resource::ResourceSystem* resourceSystem, osg::Node& node);

    void overrideTexture(std::string_view texture, Resource::ResourceSystem* resourceSystem, osg::Node& node);

    // Node callback to entirely skip the traversal.
    class NoTraverseCallback : public osg::NodeCallback
    {
    public:
        void operator()(osg::Node* node, osg::NodeVisitor* nv) override
        {
            // no traverse()
        }
    };

//## VR_PATCH BEGIN
    /// Draw callback for RTT that can be used to regenerate mipmaps
    /// either as a predraw before use or a postdraw after RTT.
    class MipmapCallback : public osg::Camera::DrawCallback
    {
    public:
        MipmapCallback(osg::Texture2D* texture)
            : mTexture(texture)
        {
        }

        ~MipmapCallback();

        void operator()(osg::RenderInfo& info) const override;

    private:
        osg::ref_ptr<osg::Texture2D> mTexture;
    };

//## VR_PATCH END
    bool shouldAddMSAAIntermediateTarget();

    const osg::ref_ptr<osg::LightModel>& getVFXLightModelInstance();
}

#endif
