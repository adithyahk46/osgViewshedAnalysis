#include "Viewshed3DAreaAnalysis.h"

#include <osg/Geometry>
#include <osg/LineWidth>

#include <osgShadow/ShadowedScene>
#include <osgShadow/ShadowMap>



Viewshed3DAreaAnalysis::Viewshed3DAreaAnalysis()
{

    createCamere();

}

osg::ref_ptr<osg::Geode> Viewshed3DAreaAnalysis::createCamere()
{

    _cameraNode = new osg::Geode();
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry();

    osg::ref_ptr<osg::Vec3Array> verts = new osg::Vec3Array();

    // Apex (touching the map at GeoPoint)
   verts->push_back(osg::Vec3(0, 0, 0));        // 0
// Base square ABOVE apex
    verts->push_back(osg::Vec3(-50, -50, 80));   // 1
    verts->push_back(osg::Vec3( 50, -50, 80));   // 2
    verts->push_back(osg::Vec3( 50,  50, 80));   // 3
    verts->push_back(osg::Vec3(-50,  50, 80));   // 4

    geom->setVertexArray(verts.get());

    osg::ref_ptr<osg::DrawElementsUInt> lines =
        new osg::DrawElementsUInt(GL_LINES);

    // Apex edges
    lines->push_back(0); lines->push_back(1);
    lines->push_back(0); lines->push_back(2);
    lines->push_back(0); lines->push_back(3);
    lines->push_back(0); lines->push_back(4);

    // Base square
    lines->push_back(1); lines->push_back(2);
    lines->push_back(2); lines->push_back(3);
    lines->push_back(3); lines->push_back(4);
    lines->push_back(4); lines->push_back(1);

    geom->addPrimitiveSet(lines.get());

    _borderLineColor = new osg::Vec4Array();
    _borderLineColor->push_back(osg::Vec4(1, 0, 0, 1));
    geom->setColorArray(_borderLineColor.get(), osg::Array::BIND_OVERALL);

    _cameraNode->addDrawable(geom.get());

    osg::StateSet* ss = geom->getOrCreateStateSet();
    ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    osg::ref_ptr<osg::LineWidth> lw = new osg::LineWidth();
    lw->setWidth(1.0f);
    geom->getOrCreateStateSet()->setAttributeAndModes(lw.get(),osg::StateAttribute::ON);
    _cameraNode->addDrawable(geom.get());

    return _cameraNode;

}

