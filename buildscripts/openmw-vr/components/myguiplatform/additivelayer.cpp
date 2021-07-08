#include "additivelayer.hpp"

#include <osg/BlendFunc>
#include <osg/StateSet>

#include "myguirendermanager.hpp"

namespace osgMyGUI
{

    AdditiveLayer::AdditiveLayer()
    {
        mStateSet = new osg::StateSet;
        mStateSet->setAttributeAndModes(new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE));
    }

    AdditiveLayer::~AdditiveLayer()
    {
        // defined in .cpp file since we can't delete incomplete types
    }

    void AdditiveLayer::renderToTarget(MyGUI::IRenderTarget *_target, bool _update)
    {
        StateInjectableRenderTarget* injectableTarget = static_cast<StateInjectableRenderTarget*>(_target);

        injectableTarget->setInjectState(mStateSet.get());

        MyGUI::OverlappedLayer::renderToTarget(_target, _update);

        injectableTarget->setInjectState(nullptr);
    }

}
