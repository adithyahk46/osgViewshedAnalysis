#pragma once


#include <QVector>

#include <osg/Vec3>
#include <osg/Group>
#include <osgViewer/Viewer>
#include <osg/TextureCubeMap>
#include <osg/PositionAttitudeTransform>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include <QColor>

namespace osg {
    class PositionAttitudeTransform;
    class TextureCubeMap;
}

class VisibilityTestArea
{

public:
    VisibilityTestArea(osg::Group* sceneRoot, osgViewer::Viewer* viewer, osg::Vec3 lightSource);
    ~VisibilityTestArea();

    void clear();
    void setViwerPosition(const osg::Vec3 position);

    void buildModel();
    void updateAttributes();

protected:
    void generateTestSphere(osg::ref_ptr<osg::TextureCubeMap> depthMap, osg::ref_ptr<osg::TextureCubeMap> colorMap);
    osg::Camera* generateCubeCamera(osg::ref_ptr<osg::TextureCubeMap> cubeMap, unsigned face, osg::Camera::BufferComponent component);

private:

    osg::ref_ptr<osg::Group> _shadowedScene;
    osg::ref_ptr<osg::Group> _parentScene;
    osgViewer::Viewer* _mainViewer;


    osg::Vec3 _lightSource;
    int _userHeight;
    int _viweingRadius;

    osg::ref_ptr<osg::PositionAttitudeTransform> debugNode;

    osg::ref_ptr<osg::Program> _visibilityShader;

    osg::ref_ptr<osg::MatrixTransform> _lightIndicator;


    osg::ref_ptr<osg::TextureCubeMap>  depthMap;
    osg::ref_ptr<osg::Program>  depthShader;
    osg::ref_ptr<osg::Camera> _depthCameras[6];
    osg::ref_ptr<osg::Camera> _colorCameras[6];

    QColor  visibleColor   = QColor { 159, 255, 61 };
    QColor  invisibleColor = QColor { 255, 87, 61 };

    osg::ref_ptr<osg::Uniform> _lightPosUniform;
    osg::ref_ptr<osg::Uniform> _viewRadiusUniform;
    osg::ref_ptr<osg::Uniform> _inverseViewUniform[6] ;




};
