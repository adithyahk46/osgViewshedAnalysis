#pragma once


#include <QVector>

#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/Group>
#include <osgViewer/Viewer>
#include <osg/TextureCubeMap>
#include <osg/PositionAttitudeTransform>
#include <osg/Geode>
#include <osg/MatrixTransform>

namespace osg {
    class PositionAttitudeTransform;
    class TextureCubeMap;
}

class VisibilityTestArea
{

public:
    VisibilityTestArea(osg::Group* sceneRoot, osgViewer::Viewer* viewer, osg::Vec3 lightSource, int radius);
    ~VisibilityTestArea();

    void clear();
    void setViwerPosition(const osg::Vec3 position);

    void setRadius(int radius);

    void setVerticalFOV(int fov);
    void setHorizontalFOV(int fov);

    void setVisibleAreaColor(const osg::Vec4 color);
    void setInvisibleAreaColor(const osg::Vec4 color);

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

    osg::ref_ptr<osg::AutoTransform> _lightIndicator;


    osg::ref_ptr<osg::TextureCubeMap>  depthMap;
    osg::ref_ptr<osg::Program>  depthShader;
    osg::ref_ptr<osg::Camera> _depthCameras[6];
    osg::ref_ptr<osg::Camera> _colorCameras[6];

    int _verticalFOV = 90;
    int _horizontalFOV = 90;

    float near_plane = 0.5f;
    float far_plane ;;


    osg::Vec4  visibleColor   = osg::Vec4(159.0f / 255.0f, 255.0f / 255.0f, 61.0f / 255.0f, 0.5f);
    osg::Vec4  invisibleColor = osg::Vec4(255.0f / 255.0f, 87.0f / 255.0f, 61.0f / 255.0f, 0.5f);

    osg::ref_ptr<osg::Uniform> _lightPosUniform;
    osg::ref_ptr<osg::Uniform> _viewRadiusUniform;
    osg::ref_ptr<osg::Uniform> _inverseViewUniform[6] ;
    osg::ref_ptr<osg::Uniform> _farPlaneUniform;
    osg::ref_ptr<osg::Uniform> _nearPlaneUniform;

    osg::ref_ptr<osg::Uniform> _baseTextureUniform;
    osg::ref_ptr<osg::Uniform> _shadowMapUniform;

    osg::ref_ptr<osg::Uniform> _visibleColorUniform;
    osg::ref_ptr<osg::Uniform> _invisibleColorUniform;

};
