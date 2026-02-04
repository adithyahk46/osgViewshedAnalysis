#pragma once


#include <QVector>

#include <osg/Vec3>
#include <osg/Group>
#include <osgViewer/Viewer>
#include <osg/TextureCubeMap>
#include <osg/PositionAttitudeTransform>
#include <osg/Geode>


namespace osg {
    class PositionAttitudeTransform;
    class TextureCubeMap;
}

class VisibilityTestArea 
{

public:
	VisibilityTestArea(osg::Group* sceneRoot, osgViewer::Viewer* viewer, osg::Vec3 userPosition);
	~VisibilityTestArea();

    void clear();
    void setViwerPosition(const osg::Vec3d position);

    void buildModel();
	void updateAttributes();

protected:
    void showControlPanel();

    void generateTestSphere(osg::ref_ptr<osg::TextureCubeMap> depthMap, osg::ref_ptr<osg::TextureCubeMap> colorMap);
    osg::Camera* generateCubeCamera(osg::ref_ptr<osg::TextureCubeMap> cubeMap, unsigned face, osg::Camera::BufferComponent component);

private:

	osg::ref_ptr<osg::Group> _shadowedScene;
	osg::ref_ptr<osg::Group> _parentScene;
	osgViewer::Viewer* _mainViewer;


    osg::Vec3 _userPosition;
    int _userHeight;
    int _viweingRadius;

    osg::ref_ptr<osg::PositionAttitudeTransform> debugNode;
	
	osg::ref_ptr<osg::Program> _renderProgram;
	osg::ref_ptr<osg::Camera> _depthCameras[6];
    osg::ref_ptr<osg::Camera> _colorCameras[6];

	osg::ref_ptr<osg::Geode>  _currentDrawNode;


    osg::ref_ptr<osg::PositionAttitudeTransform>  _currentAnchor;
	osg::Vec3  _anchoredOffset;
	osg::Vec3  _anchoredWorldPos;
    osg::Vec3d _currentWorldPos;


};
