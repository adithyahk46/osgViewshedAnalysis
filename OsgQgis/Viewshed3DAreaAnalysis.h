#ifndef VIEWSHED3DAREAANALYSIS_H
#define VIEWSHED3DAREAANALYSIS_H

#include <osg/Geode>

class Viewshed3DAreaAnalysis
{
public:
    Viewshed3DAreaAnalysis();

    osg::ref_ptr<osg::Vec4Array> getBorderLineColor(){return _borderLineColor;}

    void setBorderLineColor(osg::ref_ptr<osg::Vec4Array> LineColor){_borderLineColor = LineColor;}
    osg::ref_ptr<osg::Geode> getCameraNode(){return _cameraNode;}


private:
    osg::ref_ptr<osg::Geode> createCamere();


private:
    osg::ref_ptr<osg::Geode> _cameraNode;

    osg::ref_ptr<osg::Vec4Array> _borderLineColor;



};

#endif // VIEWSHED3DAREAANALYSIS_H
