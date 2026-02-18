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

enum TraversalOption
{
    INTERSECT_IGNORE = 0x00000004,
    TRAVERSAL_IGNORE = 0x00000010
};



static const int   SM_TEXTURE_WIDTH  = 1024;
static const bool  SHOW_DEBUG_CAMERA = false;
static const QString SHADER_DIR =
    "C:/Users/pnmt1054/Adithya_working_directory/QT_PROJECTS/osgViewshedAnalysis/OsgQgis/VisibilityTestArea/";

static const QString depthMapVertShader        = SHADER_DIR + "depthMap.vert";
static const QString depthMapFragShader        = SHADER_DIR + "depthMap.frag";
static const QString visibilityVertShader      = SHADER_DIR + "visibilityShader.vert";
static const QString visibilityFragShader      = SHADER_DIR + "visibilityShader.frag";
static const QString depthVisualizerVertShader = SHADER_DIR + "depthVisualizer.vert";
static const QString depthVisualizerFragShader = SHADER_DIR + "depthVisualizer.frag";

class SmoothingCallback: public osgDB::Registry::ReadFileCallback
{
public:
    virtual osgDB::ReaderWriter::ReadResult  readNode(const std::string &fileName, const osgDB::ReaderWriter::Options *options) override
    {
        osgDB::ReaderWriter::ReadResult  result     = osgDB::Registry::instance()->readNodeImplementation(fileName, options);
        osg::Node                       *loadedNode = result.getNode();

        if (loadedNode)
        {
            osgUtil::SmoothingVisitor  smoothing;
            loadedNode->accept(smoothing);
        }

        return result;
    }
};

static osg::Vec4  colorToVec(const QColor &color)
{
    return osg::Vec4(color.redF(), color.greenF(), color.blueF(), color.alphaF());
}

static osg::ref_ptr<osg::Program>  generateShader(const QString &vertFile, const QString &fragFile, QString geomFile = "")
{
    osg::ref_ptr<osg::Program>  program = new osg::Program;

    if (!vertFile.isEmpty())
    {
        osg::ref_ptr<osg::Shader>  shader = new osg::Shader(osg::Shader::VERTEX);
        program->addShader(shader);

        if (!shader->loadShaderSourceFromFile(vertFile.toLocal8Bit().toStdString()))
        {
            osg::notify(osg::WARN) << "vertex program load failed" << std::endl;

            return nullptr;
        }
    }

    if (!fragFile.isEmpty())
    {
        osg::ref_ptr<osg::Shader>  shader = new osg::Shader(osg::Shader::FRAGMENT);
        program->addShader(shader);

        if (!shader->loadShaderSourceFromFile(fragFile.toLocal8Bit().toStdString()))
        {
            osg::notify(osg::WARN) << "fragment program load failed" << std::endl;

            return nullptr;
        }
    }

    if (!geomFile.isEmpty())
    {
        osg::ref_ptr<osg::Shader>  shader = new osg::Shader(osg::Shader::GEOMETRY);
        program->addShader(shader);

        if (!shader->loadShaderSourceFromFile(geomFile.toLocal8Bit().toStdString()))
        {
            osg::notify(osg::WARN) << "geometry program load failed" << std::endl;

            return nullptr;
        }
    }

    return program;
}

VisibilityTestArea::VisibilityTestArea(osg::Group* sceneRoot, osgViewer::Viewer* viewer,osg::Vec3 lightSource, int radius):
    // _shadowedScene(sceneRoot),
    _mainViewer(viewer),
    _lightSource(lightSource),
    _viweingRadius(radius)
{

        _parentScene = new osg::Group;
        _shadowedScene = new osg::Group;
        _parentScene->addChild(_shadowedScene);
        _mainViewer->getSceneData()->asGroup()->addChild(_parentScene);

        const float halfSize = 200.0f;   // 200 width
        const int gridSize = 50;          // resolution
        const float maxHeight = 20.0f;    // elevation at origin

        osg::ref_ptr<osg::Geometry> geom = new osg::Geometry();
        osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
        osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();

        float step = (halfSize * 2.0f) / gridSize;

        // Generate vertices
        for (int y = 0; y <= gridSize; ++y)
        {
            for (int x = 0; x <= gridSize; ++x)
            {
                float px = -halfSize + x * step;
                float py = -halfSize + y * step;

                float dist = std::sqrt(px * px + py * py);
                float height = maxHeight * std::exp(-(dist * dist) / 2000.0f);

                vertices->push_back(osg::Vec3(px, py, height));
                normals->push_back(osg::Vec3(0.0f, 0.0f, 1.0f));
            }
        }

        geom->setVertexArray(vertices.get());
        geom->setNormalArray(normals.get(), osg::Array::BIND_PER_VERTEX);

        // Create triangle strips
        for (int y = 0; y < gridSize; ++y)
        {
            osg::ref_ptr<osg::DrawElementsUInt> strip =
                new osg::DrawElementsUInt(GL_TRIANGLE_STRIP);

            for (int x = 0; x <= gridSize; ++x)
            {
                strip->push_back((y + 1) * (gridSize + 1) + x);
                strip->push_back(y * (gridSize + 1) + x);
            }
            geom->addPrimitiveSet(strip.get());
        }

        osg::ref_ptr<osg::Geode> geode = new osg::Geode();
        geode->addDrawable(geom.get());


        _shadowedScene->addChild(geode);



}

VisibilityTestArea::~VisibilityTestArea()
{
    clear();
}

osg::MatrixTransform* makeIndicator(osg::Vec3 eye)
{
    osg::ref_ptr<osg::Sphere> sphere = new osg::Sphere(osg::Vec3(0.0f, 0.0f, 0.0f), 10.0f);

    osg::ref_ptr<osg::ShapeDrawable> drawable = new osg::ShapeDrawable(sphere.get());

    drawable->setColor(osg::Vec4(0, 0, 1, 1));

    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    geode->addDrawable(drawable.get());

    geode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    osg::ref_ptr<osg::MatrixTransform> xform =new osg::MatrixTransform();

    xform->addChild(geode.get());

    xform->setMatrix(osg::Matrix::translate(eye));

    return xform.release();
}

void  VisibilityTestArea::generateTestSphere(osg::ref_ptr<osg::TextureCubeMap> depthMap, osg::ref_ptr<osg::TextureCubeMap> colorMap)
{
    osg::ref_ptr<osg::Program>  depthVisualizer = generateShader(
        depthVisualizerVertShader,
        depthVisualizerFragShader);

    debugNode = new osg::PositionAttitudeTransform;
    debugNode->setPosition(_lightSource);
    debugNode->setCullingActive(false);

    osg::StateSet *ss = debugNode->getOrCreateStateSet();

    ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    ss->setTextureAttributeAndModes(0, depthMap, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
    ss->setTextureAttributeAndModes(1, colorMap, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
    ss->setAttribute(depthVisualizer, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
    ss->addUniform(new osg::Uniform("center", _lightSource + osg::Vec3 { 0, 0, 70 }), osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
    ss->addUniform(new osg::Uniform("depthMap", 0), osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
    ss->addUniform(new osg::Uniform("colorMap", 1), osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);

    osg::ref_ptr<osg::TessellationHints>  tsHint = new osg::TessellationHints;
    tsHint->setDetailRatio(8.0);
    osg::ref_ptr<osg::Geode>  geode = new osg::Geode;
    geode->addDrawable(new osg::ShapeDrawable(new osg::Sphere(_lightSource + osg::Vec3 { 0, 0, 70 }, 50), tsHint));

    debugNode->addChild(geode);
    _parentScene->addChild(debugNode);
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
           _lightIndicator->setMatrix(
               osg::Matrix::translate(position)
           );
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
    depthShader = generateShader(depthMapVertShader, depthMapFragShader);
    if (!depthShader.valid())
    {
        osg::notify(osg::WARN) << "Failed to load depth shader" << std::endl;
        // clear();
        return;
    }

    // Render the result in shader
    _visibilityShader = generateShader(visibilityVertShader, visibilityFragShader);
    if (!_visibilityShader.valid())
    {
        osg::notify(osg::WARN) << "Failed to load visibility shader" << std::endl;
        clear();

        return;
    }

    // Light source in shader
    float                     near_plane = 0.5f;
    float                     far_plane  = _lightSource.z()+(float)_viweingRadius;

    _lightPosUniform = new osg::Uniform("lightPos", _lightSource);
    _viewRadiusUniform = new osg::Uniform("user_area", (float)_viweingRadius);
    _farPlaneUniform = new osg::Uniform("far_plane",far_plane);
    _nearPlaneUniform = new osg::Uniform("near_plane", near_plane);
    osg::Matrix               shadowProj = osg::Matrix::perspective(90.0, SM_TEXTURE_WIDTH / SM_TEXTURE_WIDTH, near_plane, far_plane);


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
    _parentScene->getOrCreateStateSet()->addUniform(new osg::Uniform("baseTexture", 1));
    _parentScene->getOrCreateStateSet()->addUniform(new osg::Uniform("shadowMap", 1));
    _parentScene->getOrCreateStateSet()->addUniform(new osg::Uniform("visibleColor", colorToVec(visibleColor)));
    _parentScene->getOrCreateStateSet()->addUniform(new osg::Uniform("invisibleColor", colorToVec(invisibleColor)));

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




