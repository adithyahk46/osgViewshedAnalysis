#include "mainwindow.h"
#include "ui_mainwindow.h"


#include <osg/Geometry>
#include <osg/Geode>
#include <osg/LightModel>
#include <osg/LightSource>

#include <osgViewer/Viewer>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/LineWidth>

#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Group>
#include <osgViewer/Viewer>

#include <osg/Vec4>
#include <osg/Uniform>
#include <osg/Vec3>
#include <osg/Program>
#include <osg/Shader>
#include <osgShadow/ShadowedScene>
#include <osgShadow/ShadowMap>



#include <Viewshed3DAreaAnalysis.h>
#include "XYZCoordinateAxes.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    osgWidget= new osgQOpenGLWidget();
    osgWidget->setMouseTracking(true);
    setCentralWidget(osgWidget);
    QObject::connect(osgWidget, &osgQOpenGLWidget::initialized, [this] { initOsg();
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}



#include <osg/Camera>
#include <osg/ShapeDrawable>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/LineWidth>
#include <osg/MatrixTransform>
#include <osgDB/WriteFile>
#include <osgViewer/Viewer>
#include <osgGA/TrackballManipulator>
#include <osg/StateSet>
#include <osg/PolygonMode>
#include <iostream>
#include <osg/Point>
#include <osg/Math>



void MainWindow::initializeScene()
{

    testgroup = new osg::Group();

    osg::ref_ptr<XYZCoordinateAxes> axes = new XYZCoordinateAxes();
    axes->setAxisLength(200.0f);
    axes->setGridUnit(10.0f);
    axes->setGridColor(osg::Vec4(0.4f, 0.4f, 0.4f, 1.0f));
    axes->build();
    root->addChild(axes);

    osg::ref_ptr<osg::Geode> cubeGeode = new osg::Geode;
    osg::ref_ptr<osg::ShapeDrawable> cube =
        new osg::ShapeDrawable(new osg::Box(osg::Vec3(10,0,2), 4.0f));
    cube->setColor(osg::Vec4(0.2f, 0.5f, 0.8f, 1.0f));
    cubeGeode->addDrawable(cube.get());
    cubeGeode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    testgroup->addChild(cubeGeode.get());


    osg::ref_ptr<osg::Geode> sphereGeode = new osg::Geode;
    osg::ref_ptr<osg::ShapeDrawable> sphere =
        new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(3,2,2), 1.0f));
    sphere->setColor(osg::Vec4(1,1,0,1));
    sphereGeode->addDrawable(sphere.get());

    sphereGeode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    testgroup->addChild(sphereGeode.get());

    osg::ref_ptr<osg::Geometry> planeGeom = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> planeverts = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> planecolors = new osg::Vec4Array;

    float size = 200.0f;
    planeverts->push_back(osg::Vec3(-size , size, -0.02f));
    planeverts->push_back(osg::Vec3(size , size, -0.02f));
    planeverts->push_back(osg::Vec3(size , -size, -0.02f));
    planeverts->push_back(osg::Vec3(-size , -size, -0.02f));

    planecolors->push_back(osg::Vec4(0.8f,0.8f,0.8f, 1.0f));

    planeGeom->setVertexArray(planeverts.get());
    planeGeom->setColorArray(planecolors.get(), osg::Array::BIND_OVERALL);
    osg::ref_ptr<osg::DrawElementsUInt> indices =
        new osg::DrawElementsUInt(GL_TRIANGLES);

    indices->push_back(0);
    indices->push_back(1);
    indices->push_back(2);

    indices->push_back(0);
    indices->push_back(2);
    indices->push_back(3);

    planeGeom->addPrimitiveSet(indices.get());

    osg::ref_ptr<osg::Geode> planeGeode = new osg::Geode;
    planeGeode->addDrawable(planeGeom.get());

    osg::StateSet* planeSS = planeGeode->getOrCreateStateSet();
    planeSS->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    root->addChild(planeGeode.get());


}

#include "VisibilityTestArea/VisibilityTestArea.h"

void MainWindow::on_pushButton_clicked()
{

 osg::Vec3d observationPoint(20.0, 20.0,20.0);  // X, Y, Z position
    VisibilityTestArea* visibilityTest = new VisibilityTestArea(testgroup, viewer, observationPoint);
    // visibilityTest->setParameter(observationPoint, visibilityRadius);
    visibilityTest->buildModel();

}


void MainWindow::initOsg()
{
    viewer = osgWidget->getOsgViewer();
    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    root = new osg::Group();
    initializeScene();

    shadowGroup = new osg::Group();
    shadowGroup->addChild(testgroup);

    root->addChild(shadowGroup);

    viewer->setSceneData(root.get());
    viewer->setCameraManipulator(new osgGA::TrackballManipulator());
    viewer->getCamera()->setClearColor(osg::Vec4(0.1, 0.1, 0.1, 1));


}
