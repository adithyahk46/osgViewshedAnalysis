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



void VisibilityTestArea::setupDebugHUD()
{
    osg::ref_ptr<osg::Camera> hudCamera = new osg::Camera;
    hudCamera->setProjectionMatrix(osg::Matrix::ortho2D(0, 1280, 0, 720));
    hudCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    hudCamera->setViewMatrix(osg::Matrix::identity());
    hudCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    hudCamera->setRenderOrder(osg::Camera::POST_RENDER);
    hudCamera->setAllowEventFocus(false);
    hudCamera->setNodeMask(0xffffffff);

    osg::ref_ptr<osg::Geode> quadGeode = new osg::Geode;
    osg::ref_ptr<osg::Geometry> quadBoard = osg::createTexturedQuadGeometry(
        osg::Vec3(50.0f, 50.0f, 0.0f),
        osg::Vec3(400.0f, 0.0f, 0.0f),
        osg::Vec3(0.0f, 400.0f, 0.0f)
    );
    quadGeode->addDrawable(quadBoard);
    hudCamera->addChild(quadGeode);

    osg::StateSet* stateset = quadBoard->getOrCreateStateSet();
    stateset->setTextureAttributeAndModes(0, depthMap.get(), osg::StateAttribute::ON);
    stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateset->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);

    osg::ref_ptr<osg::Program> visualProgram = new osg::Program;

    // Vertex Shader (Core Profile)
    // Note: osg_Vertex and osg_MultiTexCoord0 are automatically
    // provided by OSG when using setUseModelViewAndProjectionUniforms(true)
    const char* vertSource =
        "#version 150\n"
        "in vec4 osg_Vertex;\n"
        "in vec4 osg_MultiTexCoord0;\n"
        "uniform mat4 osg_ModelViewProjectionMatrix;\n"
        "out vec2 texCoord;\n"
        "void main() {\n"
        "  texCoord = osg_MultiTexCoord0.xy;\n"
        "  gl_Position = osg_ModelViewProjectionMatrix * osg_Vertex;\n"
        "}\n";

    // Fragment Shader (Core Profile)
    const char* fragSource =
        "#version 150\n"
        "uniform sampler2D depthMap;\n"
        "uniform float near;\n"
        "uniform float far;\n"
        "in vec2 texCoord;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "  float z = texture(depthMap, texCoord).r;\n"
        "  // Linearization for better visibility\n"
        "  float lz = (2.0 * near) / (far + near - z * (far - near));\n"
        "  fragColor = vec4(vec3(lz), 1.0);\n"
        "}\n";

    visualProgram->addShader(new osg::Shader(osg::Shader::VERTEX, vertSource));
    visualProgram->addShader(new osg::Shader(osg::Shader::FRAGMENT, fragSource));

    stateset->setAttributeAndModes(visualProgram, osg::StateAttribute::ON);
    stateset->addUniform(new osg::Uniform("depthMap", 0));
    stateset->addUniform(new osg::Uniform("near", (float)near_plane));
    stateset->addUniform(new osg::Uniform("far", (float)far_plane));

    _parentScene->addChild(hudCamera);
}

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

osg::ref_ptr<osg::MatrixTransform> createFrustumNode(osg::Camera* camera, float near_p, float far_p) {
    // 1. Extract 'top' and 'right' from the camera's existing projection matrix
    // Since you set it using osg::Matrix::frustum(-right, right, -top, top, near, far)
    osg::Matrix proj = camera->getProjectionMatrix();

    // In a standard frustum matrix:
    // proj(0,0) = near / right
    // proj(1,1) = near / top
    double right = near_p / proj(0,0);
    double top = near_p / proj(1,1);
    double ratio = (double)far_p / (double)near_p;
    double fTop = top * ratio;
    double fRight = right * ratio;

    // 2. Create Vertices in Local Space (Camera at 0,0,0 looking at -Z)
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array(8);
    // Near Plane
    (*vertices)[0].set(-right, -top, -near_p);
    (*vertices)[1].set( right, -top, -near_p);
    (*vertices)[2].set( right,  top, -near_p);
    (*vertices)[3].set(-right,  top, -near_p);
    // Far Plane
    (*vertices)[4].set(-fRight, -fTop, -far_p);
    (*vertices)[5].set( fRight, -fTop, -far_p);
    (*vertices)[6].set( fRight,  fTop, -far_p);
    (*vertices)[7].set(-fRight,  fTop, -far_p);

    // 3. Create Geometry
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    geom->setVertexArray(vertices);

    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_LINES);
    // Near Square
    indices->push_back(0); indices->push_back(1); indices->push_back(1); indices->push_back(2);
    indices->push_back(2); indices->push_back(3); indices->push_back(3); indices->push_back(0);
    // Far Square
    indices->push_back(4); indices->push_back(5); indices->push_back(5); indices->push_back(6);
    indices->push_back(6); indices->push_back(7); indices->push_back(7); indices->push_back(4);
    // Connecting Lines
    indices->push_back(0); indices->push_back(4); indices->push_back(1); indices->push_back(5);
    indices->push_back(2); indices->push_back(6); indices->push_back(3); indices->push_back(7);
    geom->addPrimitiveSet(indices);

    // 4. Wrap in Geode and MatrixTransform
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(geom);
    geode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    osg::ref_ptr<osg::MatrixTransform> xform = new osg::MatrixTransform;
    xform->addChild(geode);

    // Set initial position based on camera's current view
    xform->setMatrix(osg::Matrix::inverse(camera->getViewMatrix()));

    return xform;
}

void VisibilityTestArea::setUpCamera(){

    _depthCamera = new osg::Camera;

    _depthCamera->setClearMask(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    _depthCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    _depthCamera->setRenderOrder(osg::Camera::PRE_RENDER);
    _depthCamera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
    _depthCamera->setViewport(0, 0, SM_TEXTURE_WIDTH, SM_TEXTURE_WIDTH);
    _depthCamera->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    _depthCamera->attach(osg::Camera::DEPTH_BUFFER, depthMap);

    _depthCamera->setNodeMask(0xffffffff & (~INTERSECT_IGNORE));
    // Generate shadow map cameras and corresponding textures
   // osg::Matrix  shadowProj = osg::Matrix::perspective(_verticalFOV, SM_TEXTURE_WIDTH / SM_TEXTURE_WIDTH, near_plane, far_plane);

   double vFovRad = osg::DegreesToRadians((double)_verticalFOV);
   double hFovRad = osg::DegreesToRadians((double)_horizontalFOV);

   double top = near_plane * tan(vFovRad / 2.0);
   double right = near_plane * tan(hFovRad / 2.0);

   osg::Matrix shadowProj = osg::Matrix::frustum(-right, right,-top, top, near_plane, far_plane);

   _depthCamera->setProjectionMatrix(shadowProj);
   _depthCamera->getOrCreateStateSet()->setAttribute(depthShader, osg::StateAttribute::ON);
   _depthCamera->getOrCreateStateSet()->addUniform(_lightPosUniform);
   _depthCamera->getOrCreateStateSet()->addUniform(_farPlaneUniform);
   _depthCamera->getOrCreateStateSet()->addUniform(_nearPlaneUniform);

   _depthCamera->addChild(_shadowedScene);
   _parentScene->addChild(_depthCamera);

   _lightIndicator = makeIndicator(_lightSource);
   _parentScene->getParent(0)->addChild(_lightIndicator);

   frustumVisual = createFrustumNode(_depthCamera.get(), near_plane, far_plane);
   _parentScene->getParent(0)->addChild(frustumVisual);


   osg::Vec3  lightPos = _lightSource;

   osg::Matrix view =
       osg::Matrix::lookAt(lightPos, lightPos + osg::Vec3(1.0, 0.0, 0.0), osg::Vec3(0.0, 0.0, 1.0));

   _depthCamera->setViewMatrix(view);

   _inverseViewUniform = new osg::Uniform("inverse_view", osg::Matrixf::inverse(view));
   _depthCamera->getOrCreateStateSet()->addUniform(_inverseViewUniform);

   _inverseViewUniform->set(osg::Matrixf::inverse(view));

   osg::Matrix worldMatrix = osg::Matrix::inverse(_depthCamera->getViewMatrix());
   frustumVisual->setMatrix(worldMatrix);


   // Create a Bias Matrix (maps -1->1 to 0->1)
   osg::Matrix biasMatrix = osg::Matrix::translate(1.0, 1.0, 1.0) * osg::Matrix::scale(0.5, 0.5, 0.5);

   // Get the Camera's View and Projection
   osg::Matrix view1 = _depthCamera->getViewMatrix();
   osg::Matrix proj = _depthCamera->getProjectionMatrix();

   // Combine them: VP = View * Projection * Bias
   osg::Matrix viewProjectionBias = view1 * proj * biasMatrix;

   _cameraVPUniform->set(viewProjectionBias);
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

    depthMap = new osg::Texture2D;
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
    far_plane  = (float)_viweingRadius + farPlaneOffset;

    _lightPosUniform = new osg::Uniform("lightPos", _lightSource);
    _viewRadiusUniform = new osg::Uniform("user_area", (float)_viweingRadius);
    _farPlaneUniform = new osg::Uniform("far_plane",far_plane);
    _nearPlaneUniform = new osg::Uniform("near_plane", near_plane);

    _visibleColorUniform = new osg::Uniform("visibleColor", visibleColor);
    _invisibleColorUniform = new osg::Uniform("invisibleColor", invisibleColor);

    _baseTextureUniform = new osg::Uniform("baseTexture", 0);
    _shadowMapUniform = new osg::Uniform("shadowMap", 1);

    _cameraVPUniform = new osg::Uniform("cameraVP", osg::Matrixf());


    setUpCamera();


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
    _parentScene->getOrCreateStateSet()->addUniform(_cameraVPUniform);


    setupDebugHUD();

}

void VisibilityTestArea::setCameraPosition(const osg::Vec3 pos)
{
    if(!_depthCamera.valid()) return;

    _lightSource = pos;
    _lightPosUniform->set(_lightSource);

    if (_lightIndicator.valid()) _lightIndicator->setPosition(_lightSource);

    osg::Vec3  lightPos = _lightSource;

    {
        osg::Vec3f pos, dir, up;
        _depthCamera->getViewMatrix().getLookAt(pos,dir,up);
        osg::Matrix view =
            osg::Matrix::lookAt(lightPos, lightPos + (dir-pos), osg::Vec3(0.0, 0.0, 1.0));
        _depthCamera->setViewMatrix(view);

        if(!_inverseViewUniform.valid())
        {
                _inverseViewUniform = new osg::Uniform("inverse_view", osg::Matrixf::inverse(view));
                _depthCamera->getOrCreateStateSet()->addUniform(_inverseViewUniform);
        }
        else{
                _inverseViewUniform->set(osg::Matrixf::inverse(view));
        }

    }


    osg::Matrix worldMatrix = osg::Matrix::inverse(_depthCamera->getViewMatrix());
    frustumVisual->setMatrix(worldMatrix);


    // Create a Bias Matrix (maps -1->1 to 0->1)
    osg::Matrix biasMatrix = osg::Matrix::translate(1.0, 1.0, 1.0) * osg::Matrix::scale(0.5, 0.5, 0.5);

    // Get the Camera's View and Projection
    osg::Matrix view1 = _depthCamera->getViewMatrix();
    osg::Matrix proj = _depthCamera->getProjectionMatrix();

    // Combine them: VP = View * Projection * Bias
    osg::Matrix viewProjectionBias = view1 * proj * biasMatrix;

    _cameraVPUniform->set(viewProjectionBias);

    // osg::Vec3f pos, dir, up;
    // _depthCamera->getViewMatrix().getLookAt(pos,dir,up);

    // // qDebug()<<"Position = " << pos.x() <<" " <<pos.y()<<" "<<pos.z();
    // qDebug()<<"Direction = " << dir.x() <<" " <<dir.y()<<" "<<dir.z();
    // qDebug()<<"Up = " << up.x() <<" " <<up.y()<<" "<<up.z();
}

void VisibilityTestArea::setRotation(double angle, const osg::Vec3 axis)
{
    if (!_depthCamera) return;

    // 1. Convert to radians
    double radians = osg::DegreesToRadians(angle);

    // 2. Define your CONSTANT base vectors (Forward: +X, Up: +Z)
    osg::Vec3 eye = _lightSource;
    osg::Vec3 lookDir(1.0, 0.0, 0.0);
    osg::Vec3 upVec(0.0, 0.0, 1.0);

    // 3. Create the rotation matrix
    osg::Matrixd rotationMatrix = osg::Matrixd::rotate(radians, axis);

    // 4. Transform the LOOK and UP vectors by the rotation
    // This rotates the "eyesight" without moving the "eye"
    osg::Vec3 rotatedLookDir = rotationMatrix.postMult(lookDir);
    osg::Vec3 rotatedUpVec = rotationMatrix.postMult(upVec);

    // 5. Rebuild the View Matrix at the same position
    osg::Matrixd newView = osg::Matrixd::lookAt(eye, eye + rotatedLookDir, rotatedUpVec);
    _depthCamera->setViewMatrix(newView);

    // 6. Update the frustum visualizer
    osg::Matrix worldMatrix = osg::Matrix::inverse(newView);
    frustumVisual->setMatrix(worldMatrix);

    // 7. Sync the shader uniform
    if(_inverseViewUniform.valid()) {
        _inverseViewUniform->set(osg::Matrixf(worldMatrix));
    }


    // Create a Bias Matrix (maps -1->1 to 0->1)
    osg::Matrix biasMatrix = osg::Matrix::translate(1.0, 1.0, 1.0) * osg::Matrix::scale(0.5, 0.5, 0.5);

    // Get the Camera's View and Projection
    osg::Matrix view1 = _depthCamera->getViewMatrix();
    osg::Matrix proj = _depthCamera->getProjectionMatrix();

    // Combine them: VP = View * Projection * Bias
    osg::Matrix viewProjectionBias = view1 * proj * biasMatrix;

    _cameraVPUniform->set(viewProjectionBias);

    // osg::Vec3f pos, dir, up;
    // _depthCamera->getViewMatrix().getLookAt(pos,dir,up);

    // qDebug()<<"Position = " << pos.x() <<" " <<pos.y()<<" "<<pos.z();
    // qDebug()<<"Direction = " << dir.x() <<" " <<dir.y()<<" "<<dir.z();
    // qDebug()<<"Up = " << up.x() <<" " <<up.y()<<" "<<up.z();



}

void VisibilityTestArea::setDistance(int distance)
{
    _viweingRadius = distance;
    far_plane= farPlaneOffset+(float)_viweingRadius;

    if(_viewRadiusUniform.valid()){
        _viewRadiusUniform->set((float)_viweingRadius);
        _farPlaneUniform->set(far_plane);
    }

    updateProjectionMatrix();
}

void VisibilityTestArea::setVerticalFOV(int angle)
{
    _verticalFOV = angle;
    updateProjectionMatrix();

}

void VisibilityTestArea::setHorizontalFOV(int angle)
{
    _horizontalFOV = angle;
    updateProjectionMatrix();
}

void VisibilityTestArea::setVisibleAreaColor(const osg::Vec4 color)
{
    visibleColor = color;

    if(_visibleColorUniform.valid()){
        _visibleColorUniform->set(color);
    }
}

void VisibilityTestArea::setInvisibleAreaColor(const osg::Vec4 color)
{
    invisibleColor = color;

    if(_invisibleColorUniform.valid()){
        _invisibleColorUniform->set(color);
    }
}

void VisibilityTestArea::updateProjectionMatrix()
{
    if(!_depthCamera.valid()) return;

    //Updating Projection matrix of camera
    {
        double vFovRad = osg::DegreesToRadians((double)_verticalFOV);
        double hFovRad = osg::DegreesToRadians((double)_horizontalFOV);

        double top = near_plane * tan(vFovRad / 2.0);
        double right = near_plane * tan(hFovRad / 2.0);

        osg::Matrix shadowProj = osg::Matrix::frustum(-right, right,-top, top, near_plane, far_plane);

        _depthCamera->setProjectionMatrix(shadowProj);
    }

    //updating view matrix of uniform
    {
        // Create a Bias Matrix (maps -1->1 to 0->1)
        osg::Matrix biasMatrix = osg::Matrix::translate(1.0, 1.0, 1.0) * osg::Matrix::scale(0.5, 0.5, 0.5);

        // Combine them: VP = View * Projection * Bias
        osg::Matrix viewProjectionBias = _depthCamera->getViewMatrix() * _depthCamera->getProjectionMatrix() * biasMatrix;

        _cameraVPUniform->set(viewProjectionBias);
    }

    //updating projection of frustrum outline
    _parentScene->getParent(0)->removeChild(frustumVisual);

    frustumVisual = createFrustumNode(_depthCamera.get(), near_plane, far_plane);
    _parentScene->getParent(0)->addChild(frustumVisual);


}

void  VisibilityTestArea::clear()
{
    for (int i = 0; i < 6; i++)
    {
        _depthCamera->getOrCreateStateSet()->setAttribute(depthShader, osg::StateAttribute::OFF);
        _depthCamera->getOrCreateStateSet()->removeUniform(_lightPosUniform);
        _depthCamera->getOrCreateStateSet()->removeUniform(_farPlaneUniform);
        _depthCamera->getOrCreateStateSet()->removeUniform(_nearPlaneUniform);
        _depthCamera->getOrCreateStateSet()->removeUniform(_inverseViewUniform);
        _depthCamera->removeChild(_shadowedScene);
        _parentScene->removeChild(_depthCamera);
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




