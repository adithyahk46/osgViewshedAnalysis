#include "VisibilityTestArea.h"

#include <QDockWidget>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>
#include <QSlider>
#include <QPushButton>
#include <QMenu>
#include <QToolBar>
#include <QToolButton>
#include <QAction>
#include <QObject>

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
static const bool  SHOW_DEBUG_CAMERA = true;
static const QString SHADER_DIR =
    "C:/Users/pnmt1054/Adithya-working-directory/QT_PROJECTS/osgViewshedAnalysis/OsgQgis/VisibilityTestArea/";

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

VisibilityTestArea::VisibilityTestArea(osg::Group* sceneRoot, osgViewer::Viewer* viewer,osg::Vec3 userPosition):
	_shadowedScene(sceneRoot),
	_mainViewer(viewer),
	_userPosition(userPosition),
    _viweingRadius(200),
    _userHeight(3)
{
		_parentScene   = _shadowedScene->getParent(0);
        _currentWorldPos = _userPosition;
        _anchoredWorldPos = _userPosition;
        _anchoredOffset = _userPosition + osg::Vec3(2.0,2.0,2.0);

        _currentAnchor =         new osg::PositionAttitudeTransform;
        _currentAnchor->setPosition(_userPosition);
        _parentScene->addChild(_currentAnchor);


}

VisibilityTestArea::~VisibilityTestArea()
{
    clear();
}

osg::Geode* makeIndicator(osg::Vec3 eye)
{
	osg::ref_ptr<osg::Sphere>         pSphereShape   = new osg::Sphere(eye, 1.0f);
	osg::ref_ptr<osg::ShapeDrawable>  pShapeDrawable = new osg::ShapeDrawable(pSphereShape.get());

	pShapeDrawable->setColor(osg::Vec4(1.0, 1.0, 1.0, 1.0));

	osg::ref_ptr<osg::Geode>  geode = new osg::Geode();
	geode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
	geode->getOrCreateStateSet()->setAttribute(new osg::LineWidth(1.0), osg::StateAttribute::ON);

	geode->addDrawable(pShapeDrawable.get());

	return geode.release();
}

void  VisibilityTestArea::generateTestSphere(osg::ref_ptr<osg::TextureCubeMap> depthMap, osg::ref_ptr<osg::TextureCubeMap> colorMap)
{
	osg::ref_ptr<osg::Program>  depthVisualizer = generateShader(
		depthVisualizerVertShader,
		depthVisualizerFragShader);

	debugNode = new osg::PositionAttitudeTransform;
	debugNode->setPosition(_anchoredOffset);
	debugNode->setCullingActive(false);

	osg::StateSet *ss = debugNode->getOrCreateStateSet();

	ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
	ss->setTextureAttributeAndModes(0, depthMap, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
	ss->setTextureAttributeAndModes(1, colorMap, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
	ss->setAttribute(depthVisualizer, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
	ss->addUniform(new osg::Uniform("center", _currentWorldPos + osg::Vec3 { 0, 0, 70 }), osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
	ss->addUniform(new osg::Uniform("depthMap", 0), osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
	ss->addUniform(new osg::Uniform("colorMap", 1), osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);

	osg::ref_ptr<osg::TessellationHints>  tsHint = new osg::TessellationHints;
	tsHint->setDetailRatio(8.0);
	osg::ref_ptr<osg::Geode>  geode = new osg::Geode;
	geode->addDrawable(new osg::ShapeDrawable(new osg::Sphere(_anchoredWorldPos + osg::Vec3 { 0, 0, 70 }, 50), tsHint));

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
    camera->addChild(_shadowedScene);
	_parentScene->addChild(camera);

	return camera.release();
}


void VisibilityTestArea::setViwerPosition(const osg::Vec3d position)
{ 
	_userPosition = position;
}

void  VisibilityTestArea::updateAttributes()
{
	// Light source info
	osg::Vec3  lightPos = _userPosition;

	lightPos.z() += _userHeight;

	// Light source in scene
	if (_currentDrawNode.valid())
	{
        _currentAnchor->removeChild(_currentDrawNode);
	}


    _currentDrawNode = makeIndicator(lightPos + _anchoredOffset);
	_currentAnchor->addChild(_currentDrawNode);

	// Light source in shader
	float                     near_plane = 0.1f;
	float                     far_plane  = 500.0f;
	osg::Matrix               shadowProj = osg::Matrix::perspective(90.0, SM_TEXTURE_WIDTH / SM_TEXTURE_WIDTH, near_plane, far_plane);
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
		depthCamera->setProjectionMatrix(shadowProj);
		depthCamera->getOrCreateStateSet()->addUniform(new osg::Uniform("lightPos", lightPos));
		depthCamera->getOrCreateStateSet()->addUniform(new osg::Uniform("far_plane", far_plane));
		depthCamera->getOrCreateStateSet()->addUniform(new osg::Uniform("near_plane", near_plane));
		depthCamera->getOrCreateStateSet()->addUniform(new osg::Uniform("inverse_view", osg::Matrixf::inverse(shadowViews[i])));

		if (SHOW_DEBUG_CAMERA)
		{
			auto  colorCamera = _colorCameras[i];
			colorCamera->setViewMatrix(shadowViews[i]);
			colorCamera->setProjectionMatrix(shadowProj);
		}
	}

	// Update light source info for shadowing scene
	_parentScene->getOrCreateStateSet()->addUniform(new osg::Uniform("lightPos", lightPos));
	_parentScene->getOrCreateStateSet()->addUniform(new osg::Uniform("far_plane", far_plane));
	_parentScene->getOrCreateStateSet()->addUniform(new osg::Uniform("near_plane", near_plane));
    _parentScene->getOrCreateStateSet()->addUniform(new osg::Uniform("user_area", (float)_viweingRadius));
}


void  VisibilityTestArea::buildModel()
{
	// If shadow exist
    if (!_shadowedScene.valid())
	{
        osg::notify(osg::WARN) << "shadow scene is not valid" << std::endl;
        return;
	}
	// beginDrawing();
	// Parameters

	_mainViewer->getCamera()->getGraphicsContext()->getState()->setUseModelViewAndProjectionUniforms(true);

	// Keep normals updated
	osgUtil::SmoothingVisitor  smoothing;
	_shadowedScene->accept(smoothing);
	osgDB::Registry::instance()->setReadFileCallback(new SmoothingCallback);

	// Generate a shadow map
	osg::ref_ptr<osg::TextureCubeMap>  depthMap = new osg::TextureCubeMap;
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
	osg::ref_ptr<osg::Program>  depthShader = generateShader(
		depthMapVertShader, depthMapFragShader);
    if (!depthShader.valid())
    {
        osg::notify(osg::WARN) << "Failed to load depth shader" << std::endl;
        // clear();
        return;
    }

	// Generate one camera for each side of the shadow cubemap
	for (int i = 0; i < 6; i++)
	{
		_depthCameras[i] = generateCubeCamera(depthMap, i, osg::Camera::DEPTH_BUFFER);
		_depthCameras[i]->getOrCreateStateSet()->setAttribute(depthShader, osg::StateAttribute::ON);
	}

	if (SHOW_DEBUG_CAMERA)
	{
		osg::ref_ptr<osg::TextureCubeMap>  colorMap = new osg::TextureCubeMap;
		colorMap->setTextureSize(SM_TEXTURE_WIDTH, SM_TEXTURE_WIDTH);
		colorMap->setInternalFormat(GL_RGB);
		colorMap->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
		colorMap->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
		colorMap->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
		colorMap->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
		colorMap->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);

		for (int i = 0; i < 6; i++)
		{
			_colorCameras[i] = generateCubeCamera(colorMap, i, osg::Camera::COLOR_BUFFER);
		}

		generateTestSphere(depthMap, colorMap);
	}

	// Render the result in shader
	_renderProgram = generateShader(
		visibilityVertShader,
		visibilityFragShader);

	if (!_renderProgram.valid())
	{
		osg::notify(osg::WARN) << "Failed to load visibility shader" << std::endl;
		clear();

		return;
	}

	_parentScene->getOrCreateStateSet()->setTextureAttributeAndModes(1, depthMap, osg::StateAttribute::ON);
	_parentScene->getOrCreateStateSet()->setAttribute(_renderProgram, osg::StateAttribute::ON);
	_parentScene->getOrCreateStateSet()->addUniform(new osg::Uniform("baseTexture", 0));
	_parentScene->getOrCreateStateSet()->addUniform(new osg::Uniform("shadowMap", 1));

    QColor  visibleColor   = QColor { 159, 255, 61 };
    QColor  invisibleColor = QColor { 255, 87, 61 };
	_parentScene->getOrCreateStateSet()->addUniform(new osg::Uniform("visibleColor", colorToVec(visibleColor)));
	_parentScene->getOrCreateStateSet()->addUniform(new osg::Uniform("invisibleColor", colorToVec(invisibleColor)));

	updateAttributes();
}

void  VisibilityTestArea::clear()
{
	if (!_shadowedScene.valid())
	{
		return;
	}

	_currentAnchor->removeChild(_currentDrawNode);
	_currentDrawNode = NULL;

	for (auto camera : _depthCameras)
	{
		_parentScene->removeChild(camera);
		camera = NULL;
	}

	if (SHOW_DEBUG_CAMERA)
	{
		for (auto camera : _colorCameras)
		{
			_parentScene->removeChild(camera);
			camera = NULL;
		}

		_parentScene->removeChild(debugNode);
	}

	_parentScene->getOrCreateStateSet()->setAttribute(_renderProgram, osg::StateAttribute::OFF);
	_parentScene->getOrCreateStateSet()->removeAttribute(osg::StateAttribute::PROGRAM);
	_parentScene->getOrCreateStateSet()->removeTextureAttribute(1, osg::StateAttribute::TEXTURE);

	osgDB::Registry::instance()->setReadFileCallback(NULL);

	_mainViewer->getCamera()->getGraphicsContext()->getState()->setUseModelViewAndProjectionUniforms(false);
	
	_shadowedScene = NULL;

}




