#include "VisibilityTestArea.h"


#include <osg/LineWidth>
#include <osg/ShapeDrawable>
#include <osg/TextureCubeMap>
#include <osg/BoundingSphere>
#include <osg/PositionAttitudeTransform>
#include <osgDB/ReadFile>
#include <osgDB/Registry>

#include <osgUtil/SmoothingVisitor>
#include <osgSim/OverlayNode>

#include <osg/AutoTransform>


#include "ViewshedShaders.h"

enum TraversalOption
{
    INTERSECT_IGNORE = 0x00000004,
    TRAVERSAL_IGNORE = 0x00000010
};

static const int   SM_TEXTURE_WIDTH  = 1024;
static const bool  SHOW_DEBUG_CAMERA = false;

static osg::ref_ptr<osg::Program>  generateShader(const std::string &vertSource, const std::string &fragSource, std::string geomSource = "")
{
    osg::ref_ptr<osg::Program>  program = new osg::Program;

    if (!vertSource.empty())
    {
        osg::ref_ptr<osg::Shader>  shader = new osg::Shader(osg::Shader::VERTEX);
        shader->setShaderSource(vertSource);
        program->addShader(shader);
    }

    if (!fragSource.empty())
    {
        osg::ref_ptr<osg::Shader>  shader = new osg::Shader(osg::Shader::FRAGMENT);
        shader->setShaderSource(fragSource);
        program->addShader(shader);
    }

    if (!geomSource.empty())
    {
        osg::ref_ptr<osg::Shader>  shader = new osg::Shader(osg::Shader::GEOMETRY);
        shader->setShaderSource(geomSource);
        program->addShader(shader);
    }
    return program;
}

VisibilityTestArea::VisibilityTestArea(osg::Group* sceneRoot, osgViewer::Viewer* viewer,osg::Vec3 lightSource, int radius):
    _shadowedScene(sceneRoot),
    _mainViewer(viewer),
    _lightSource(lightSource),
    _viweingRadius(radius)
{
        // _parentScene = new osg::Group;
        // _shadowedScene = new osg::Group;
        // _parentScene->addChild(_shadowedScene);
        // _mainViewer->getSceneData()->asGroup()->addChild(_parentScene);

        _parentScene = _shadowedScene->getParent(0);
}

VisibilityTestArea::~VisibilityTestArea()
{
    clear();
}

osg::AutoTransform* makeIndicator(osg::Vec3 eye)
{
    osg::ref_ptr<osg::Sphere> sphere = new osg::Sphere(osg::Vec3(0.0f, 0.0f, 0.0f), 5.0f);

    osg::ref_ptr<osg::ShapeDrawable> drawable = new osg::ShapeDrawable(sphere.get());

    drawable->setColor(osg::Vec4(0, 0, 1, 1));

    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    geode->addDrawable(drawable.get());

    geode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    osg::ref_ptr<osg::AutoTransform> xform = new osg::AutoTransform();
    xform->setAutoRotateMode(osg::AutoTransform::NO_ROTATION);
    xform->setAutoScaleToScreen(true);

    xform->addChild(geode.get());

    xform->setPosition(eye);

    return xform.release();
}

osg::Camera * VisibilityTestArea::generateCubeCamera(osg::ref_ptr<osg::TextureCubeMap> cubeMap, unsigned face, osg::Camera::BufferComponent component)
{
    osg::ref_ptr<osg::Camera>  camera = new osg::Camera;

    camera->setClearMask(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    camera->setRenderOrder(osg::Camera::PRE_RENDER);
    camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
    camera->setViewport(0, 0, SM_TEXTURE_WIDTH, SM_TEXTURE_WIDTH);
    camera->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    camera->attach(component, cubeMap, 0, face);

    camera->setNodeMask(0xffffffff & (~INTERSECT_IGNORE));
    return camera.release();
}

void VisibilityTestArea::setViwerPosition(const osg::Vec3 position)
{
    _lightSource = position;
    _lightPosUniform->set(_lightSource);
    _farPlaneUniform->set(_lightSource.z()+(float)_viweingRadius);
    updateAttributes();

    if (_lightIndicator.valid())
       {
           _lightIndicator->setPosition(position);
       }
}

void VisibilityTestArea::setRadius(int radius)
{
    if(_viewRadiusUniform.valid()){
        _viewRadiusUniform->set((float)radius);
        _farPlaneUniform->set(_lightSource.z()+(float)radius);
    }
    else _viweingRadius = radius;
}

void VisibilityTestArea::setVisibleAreaColor(const osg::Vec4 color)
{
    if(_visibleColorUniform.valid()){
        _visibleColorUniform->set(color);
    }
    else visibleColor = color;
}

void VisibilityTestArea::setInvisibleAreaColor(const osg::Vec4 color)
{
    if(_invisibleColorUniform.valid()){
        _invisibleColorUniform->set(color);
    }
    else invisibleColor = color;
}

void VisibilityTestArea::setVerticalFOV(int fov)
{
    _verticalFOV = fov;
    osg::Matrix               shadowProj = osg::Matrix::perspective(_verticalFOV, SM_TEXTURE_WIDTH / SM_TEXTURE_WIDTH, near_plane, far_plane);
    for (int i = 0; i < 6; i++)
    {
        _depthCameras[i]->setProjectionMatrix(shadowProj);
    }
    
}

void  VisibilityTestArea::updateAttributes()
{
    // Light source info
    osg::Vec3  lightPos = _lightSource;

    std::vector<osg::Matrix>  shadowViews;
    shadowViews.push_back(
        osg::Matrix::lookAt(lightPos, lightPos + osg::Vec3(1.0, 0.0, 0.0), osg::Vec3(0.0, -1.0, 0.0)));
    shadowViews.push_back(
        osg::Matrix::lookAt(lightPos, lightPos + osg::Vec3(-1.0, 0.0, 0.0), osg::Vec3(0.0, -1.0, 0.0)));
    shadowViews.push_back(
        osg::Matrix::lookAt(lightPos, lightPos + osg::Vec3(0.0, 1.0, 0.0), osg::Vec3(0.0, 0.0, 1.0)));
    shadowViews.push_back(
        osg::Matrix::lookAt(lightPos, lightPos + osg::Vec3(0.0, -1.0, 0.0), osg::Vec3(0.0, 0.0, -1.0)));
    shadowViews.push_back(
        osg::Matrix::lookAt(lightPos, lightPos + osg::Vec3(0.0, 0.0, 1.0), osg::Vec3(0.0, -1.0, 0.0)));
    shadowViews.push_back(
        osg::Matrix::lookAt(lightPos, lightPos + osg::Vec3(0.0, 0.0, -1.0), osg::Vec3(0.0, -1.0, 0.0)));

    // Update light source info for shadow map
    for (int i = 0; i < 6; i++)
    {
        auto  depthCamera = _depthCameras[i];
        depthCamera->setViewMatrix(shadowViews[i]);
    }

    if(!_inverseViewUniform[0].valid())
    {
        for (int i = 0; i < 6; i++){
            _inverseViewUniform[i] = new osg::Uniform("inverse_view", osg::Matrixf::inverse(shadowViews[i]));
        }
        for (int i = 0; i < 6; i++)
        {
            auto  depthCamera = _depthCameras[i];
            depthCamera->getOrCreateStateSet()->addUniform(_inverseViewUniform[i]);
        }
    }
    else{
        for (int i = 0; i < 6; i++){
            _inverseViewUniform[i]->set(osg::Matrixf::inverse(shadowViews[i]));
        }
    }
}

void  VisibilityTestArea::buildModel()
{
    // If shadow exist
    if (!_shadowedScene.valid())
    {
        osg::notify(osg::WARN) << "shadow scene is not valid" << std::endl;
        return;
    }

    _mainViewer->getCamera()->getGraphicsContext()->getState()->setUseModelViewAndProjectionUniforms(true);

    depthMap = new osg::TextureCubeMap;
    depthMap->setTextureSize(SM_TEXTURE_WIDTH, SM_TEXTURE_WIDTH);
    depthMap->setInternalFormat(GL_DEPTH_COMPONENT);
    depthMap->setSourceFormat(GL_DEPTH_COMPONENT);
    depthMap->setSourceType(GL_FLOAT);
    depthMap->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    depthMap->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    depthMap->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
    depthMap->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    depthMap->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);

    // Depth shader that writes unnormaized depth into buffer
    depthShader = generateShader(depthMapVert, depthMapFrag);
    if (!depthShader.valid())
    {
        osg::notify(osg::WARN) << "Failed to load depth shader" << std::endl;
        return;
    }

    // Render the result in shader
    _visibilityShader = generateShader(visibilityShaderVert, visibilityShaderFrag);
    if (!_visibilityShader.valid())
    {
        osg::notify(osg::WARN) << "Failed to load visibility shader" << std::endl;
        return;
    }

    // Light source in shader
    far_plane  = _lightSource.z()+(float)_viweingRadius;

    _lightPosUniform = new osg::Uniform("lightPos", _lightSource);
    _viewRadiusUniform = new osg::Uniform("user_area", (float)_viweingRadius);
    _farPlaneUniform = new osg::Uniform("far_plane",far_plane);
    _nearPlaneUniform = new osg::Uniform("near_plane", near_plane);

    _visibleColorUniform = new osg::Uniform("visibleColor", visibleColor);
    _invisibleColorUniform = new osg::Uniform("invisibleColor", invisibleColor);

    _baseTextureUniform = new osg::Uniform("baseTexture", 0);
    _shadowMapUniform = new osg::Uniform("shadowMap", 1);

     // Generate shadow map cameras and corresponding textures
    osg::Matrix  shadowProj = osg::Matrix::perspective(_verticalFOV, SM_TEXTURE_WIDTH / SM_TEXTURE_WIDTH, near_plane, far_plane);

    // Generate one camera for each side of the shadow cubemap
    for (int i = 0; i < 6; i++)
    {
        _depthCameras[i] = generateCubeCamera(depthMap, i, osg::Camera::DEPTH_BUFFER);
        _depthCameras[i]->setProjectionMatrix(shadowProj);
        _depthCameras[i]->getOrCreateStateSet()->setAttribute(depthShader, osg::StateAttribute::ON);
        _depthCameras[i]->getOrCreateStateSet()->addUniform(_lightPosUniform);
        _depthCameras[i]->getOrCreateStateSet()->addUniform(_farPlaneUniform);
        _depthCameras[i]->getOrCreateStateSet()->addUniform(_nearPlaneUniform);

        _depthCameras[i]->addChild(_shadowedScene);
        _parentScene->addChild(_depthCameras[i]);
    }

    _parentScene->getOrCreateStateSet()->setTextureAttributeAndModes(1, depthMap, osg::StateAttribute::ON);
    _parentScene->getOrCreateStateSet()->setAttribute(_visibilityShader, osg::StateAttribute::ON);
    _parentScene->getOrCreateStateSet()->addUniform(_baseTextureUniform);
    _parentScene->getOrCreateStateSet()->addUniform(_shadowMapUniform);
    _parentScene->getOrCreateStateSet()->addUniform(_visibleColorUniform);
    _parentScene->getOrCreateStateSet()->addUniform(_invisibleColorUniform);

    // Update light source info for shadowing scene
    _parentScene->getOrCreateStateSet()->addUniform(_lightPosUniform);
    _parentScene->getOrCreateStateSet()->addUniform(_farPlaneUniform);
    _parentScene->getOrCreateStateSet()->addUniform(_nearPlaneUniform);
    _parentScene->getOrCreateStateSet()->addUniform(_viewRadiusUniform);

    _lightIndicator = makeIndicator(_lightSource);
    _parentScene->getParent(0)->addChild(_lightIndicator);


    updateAttributes();
}

void  VisibilityTestArea::clear()
{
    for (int i = 0; i < 6; i++)
    {
        _depthCameras[i]->getOrCreateStateSet()->setAttribute(depthShader, osg::StateAttribute::OFF);
        _depthCameras[i]->getOrCreateStateSet()->removeUniform(_lightPosUniform);
        _depthCameras[i]->getOrCreateStateSet()->removeUniform(_farPlaneUniform);
        _depthCameras[i]->getOrCreateStateSet()->removeUniform(_nearPlaneUniform);
        _depthCameras[i]->getOrCreateStateSet()->removeUniform(_inverseViewUniform[i]);
        _depthCameras[i]->removeChild(_shadowedScene);
        _parentScene->removeChild(_depthCameras[i]);
    }

    _parentScene->getOrCreateStateSet()->removeUniform(_visibleColorUniform);
    _parentScene->getOrCreateStateSet()->removeUniform(_invisibleColorUniform);
    _parentScene->getOrCreateStateSet()->removeUniform(_lightPosUniform);
    _parentScene->getOrCreateStateSet()->removeUniform(_farPlaneUniform);
    _parentScene->getOrCreateStateSet()->removeUniform(_nearPlaneUniform);
    _parentScene->getOrCreateStateSet()->removeUniform(_viewRadiusUniform);
    _parentScene->getOrCreateStateSet()->removeUniform(_baseTextureUniform);
    _parentScene->getOrCreateStateSet()->removeUniform(_shadowMapUniform);

    _parentScene->getOrCreateStateSet()->setAttribute(_visibilityShader, osg::StateAttribute::OFF);
    _parentScene->getOrCreateStateSet()->setTextureAttributeAndModes(1, depthMap, osg::StateAttribute::OFF);
    _parentScene->getOrCreateStateSet()->removeAttribute(osg::StateAttribute::PROGRAM);
    _parentScene->getOrCreateStateSet()->removeTextureAttribute(1, osg::StateAttribute::TEXTURE);

    _parentScene->removeChild(_shadowedScene);
    _parentScene->getParent(0)->removeChild(_lightIndicator);

    // _lightIndicator = NULL;


    // if (SHOW_DEBUG_CAMERA)
    // {
    //     for (auto camera : _colorCameras)
    //     {
    //         _parentScene->removeChild(camera);
    //         camera = NULL;
    //     }

    //     _parentScene->removeChild(debugNode);
    // }

    // _parentScene->getOrCreateStateSet()->setAttribute(_visibilityShader, osg::StateAttribute::OFF);
    // _parentScene->getOrCreateStateSet()->removeAttribute(osg::StateAttribute::PROGRAM);
    // _parentScene->getOrCreateStateSet()->removeTextureAttribute(1, osg::StateAttribute::TEXTURE);

    // osgDB::Registry::instance()->setReadFileCallback(NULL);

    // _mainViewer->getCamera()->getGraphicsContext()->getState()->setUseModelViewAndProjectionUniforms(false);

    // _shadowedScene = NULL;

}




