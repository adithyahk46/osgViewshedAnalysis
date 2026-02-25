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

// #include "ViewshedShaders.h"


const char* depthMapVertVP = R"(
    #version 330 core

    //in vec4 gl_Vertex;
    out float lightDistance;

    uniform mat4 osg_ModelViewProjectionMatrix;
    uniform mat4 osg_ViewMatrixInverse;
    uniform mat4 osg_ModelViewMatrix;
    uniform vec3 lightPos;
    uniform mat4 inverse_view;

    uniform float near_plane;
    uniform float far_plane;

    void depthMapVertex(inout vec4 vertex)
    {
        // get distance between fragment and light source
        vec3 worldPos = (inverse_view * osg_ModelViewMatrix * vertex).xyz;
        lightDistance = length(worldPos - lightPos);
        lightDistance = ((1 / lightDistance) - (1 / near_plane)) / (1 / far_plane - 1 / near_plane);

        gl_Position = osg_ModelViewProjectionMatrix * vertex;
    }
)";

const char* depthMapFragVP = R"(
    #version 330 core
    in float lightDistance;
    uniform float far_plane;

    void depthMapFragment(inout vec4 color)
    {
        // Mapping to [0, 1]
        gl_FragDepth = lightDistance;
    }
)";

const char* visibilityShaderVertVP = R"(
    #version 330 core

    //in vec4 gl_Vertex;
    //in vec3 gl_Normal;
    //in vec2 gl_MultiTexCoord0;

    uniform mat4 osg_ModelViewProjectionMatrix;
    uniform mat4 osg_ViewMatrixInverse;
    uniform mat4 osg_ModelViewMatrix;

    uniform vec3 lightPos;

    out vec3 worldPos;
    out vec3 normal;
    out vec2 texCoords;
    out float lightDistance;

    void visibilityVertex(inout vec4 vertex)
    {
        worldPos = (osg_ViewMatrixInverse * osg_ModelViewMatrix * vertex).xyz;
        lightDistance = length(worldPos - lightPos);

        normal = normalize( osg_Normal );
        texCoords = osg_MultiTexCoord0.xy;
        gl_Position = osg_ModelViewProjectionMatrix * vertex;
    }
)";


const char* visibilityShaderFragVP = R"(
    #version 330 core
    // out vec4 FragColor;

    in vec3 worldPos;
    in vec3 normal;
    in vec2 texCoords;
    in float lightDistance;

    uniform vec3 lightPos;
    uniform vec4 visibleColor;
    uniform vec4 invisibleColor;

    uniform sampler2D baseTexture;
    uniform samplerCube shadowMap;

    uniform float near_plane;
    uniform float far_plane;
    uniform float user_area;

    float linearizeDepth(float z)
    {
        float z_n = 2.0 * z - 1.0;
        return 2.0 * near_plane * far_plane / (far_plane + near_plane - z_n * (far_plane - near_plane));
    };

    // Return 1 for shadowed, 0 visible
    bool isShadowed(vec3 lightDir)
    {
        float bias = max(0.01 * (1.0 - dot(normal, lightDir)), 0.001) * far_plane;

        float z = linearizeDepth(texture(shadowMap, lightDir).r);
        return lightDistance - bias > z;
    }

    void visibilityFragment(inout vec4 FragColor)
    {
        vec3 baseColor = texture(baseTexture, texCoords).rgb;

        if (length(lightPos.xy - worldPos.xy) > user_area)
            FragColor = vec4(baseColor, 1.0);
        else
        {
            vec3 lightDir = normalize(lightPos - worldPos);
            float normDif = max(dot(lightDir, normal), 0.0);

            vec3 lighting;
            // Render as visible only if it can be seen by light source
            if (normDif > 0.0 && isShadowed(-lightDir) == false)
                lighting = visibleColor.rgb * baseColor;
            else
                lighting = invisibleColor.rgb * baseColor;

            FragColor = vec4(lighting, 1.0);
        }
    }
)";



enum TraversalOption
{
    INTERSECT_IGNORE = 0x00000004,
    TRAVERSAL_IGNORE = 0x00000010
};

static const int   SM_TEXTURE_WIDTH  = 1024;



VisibilityTestArea::VisibilityTestArea(osg::Group* sceneRoot, osgViewer::Viewer* viewer,osg::Vec3 lightSource, int radius):
    _shadowedScene(sceneRoot),
    _mainViewer(viewer),
    _lightSource(lightSource),
    _viweingRadius(radius)
{
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
        _farPlaneUniform->set((float)radius+ _farPlanOffSet);
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


    // Light source in shader
    far_plane  = (float)_viweingRadius + _farPlanOffSet;

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

    osgEarth::VirtualProgram* _depthCamerasVP = new osgEarth::VirtualProgram();
    _depthCamerasVP->setFunction("depthMapVertex",depthMapVertVP,osgEarth::VirtualProgram::LOCATION_VERTEX_MODEL);
    _depthCamerasVP->setFunction("depthMapFragment",depthMapFragVP,osgEarth::VirtualProgram::LOCATION_FRAGMENT_COLORING);
    _depthCamerasVP->setShaderLogging(true, "shaders1.txt");

    // Generate one camera for each side of the shadow cubemap
    for (int i = 0; i < 6; i++)
    {
        _depthCameras[i] = generateCubeCamera(depthMap, i, osg::Camera::DEPTH_BUFFER);
        _depthCameras[i]->setProjectionMatrix(shadowProj);
        // _depthCameras[i]->getOrCreateStateSet()->setAttributeAndModes(_depthCamerasVP, osg::StateAttribute::ON);
        _depthCameras[i]->getOrCreateStateSet()->addUniform(_lightPosUniform);
        _depthCameras[i]->getOrCreateStateSet()->addUniform(_farPlaneUniform);
        _depthCameras[i]->getOrCreateStateSet()->addUniform(_nearPlaneUniform);

        _depthCameras[i]->addChild(_shadowedScene);
        _parentScene->addChild(_depthCameras[i]);
    }


    _parentScene->getOrCreateStateSet()->setTextureAttributeAndModes(1, depthMap, osg::StateAttribute::ON);

    osgEarth::VirtualProgram* _visibilityShaderVP = new osgEarth::VirtualProgram();
    _visibilityShaderVP->setFunction("visibilityVertex",visibilityShaderVertVP,osgEarth::VirtualProgram::LOCATION_VERTEX_MODEL);
    _visibilityShaderVP->setFunction("visibilityFragment",visibilityShaderFragVP,osgEarth::VirtualProgram::LOCATION_FRAGMENT_COLORING);
    _visibilityShaderVP->setShaderLogging(true, "shaders2.txt");

    _parentScene->getOrCreateStateSet()->setAttributeAndModes(_visibilityShaderVP, osg::StateAttribute::ON);
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
           // _depthCameras[i]->getOrCreateStateSet()->setAttribute(depthShader, osg::StateAttribute::OFF);
           _depthCameras[i]->getOrCreateStateSet()->removeUniform(_lightPosUniform);
           _depthCameras[i]->getOrCreateStateSet()->removeUniform(_farPlaneUniform);
           _depthCameras[i]->getOrCreateStateSet()->removeUniform(_nearPlaneUniform);
           _depthCameras[i]->getOrCreateStateSet()->removeUniform(_inverseViewUniform[i]);
           _depthCameras[i]->removeChild(_shadowedScene);
           _parentScene->removeChild(_depthCameras[i]);
           _depthCameras[i] = nullptr;
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

       // _parentScene->removeChild(_shadowedScene);
       _parentScene->getParent(0)->removeChild(_lightIndicator);

       _lightIndicator = nullptr;
       _visibilityShader = nullptr;




}



