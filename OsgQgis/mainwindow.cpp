#include "mainwindow.h"
#include "ui_mainwindow.h"

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

#include <osgEarth/LogarithmicDepthBuffer>

#include "ViewshedAreaAnalysisWidget.h"
#include "XYZCoordinateAxes.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    osgWidget = new osgQOpenGLWidget();
    osgWidget->setMouseTracking(true);

    // Set OSG widget as central widget
    setCentralWidget(osgWidget);

    QObject::connect(osgWidget, &osgQOpenGLWidget::initialized, [this] {
        initOsg();
        ui->dockWidget->setWidget(new ViewshedAreaAnalysisWidget(testgroup, viewer) );

    });

}

MainWindow::~MainWindow()
{
    delete ui;
}

#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Vec3>
#include <osg/Array>
#include <cmath>

osg::ref_ptr<osg::Node> createElevatedSquare()
{
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

    return geode;
}


void MainWindow::initializeScene()
{

    testgroup = new osg::Group();

    //axes
    {
    osg::ref_ptr<XYZCoordinateAxes> axes = new XYZCoordinateAxes();
    axes->setAxisLength(200.0f);
    axes->setGridUnit(10.0f);
    axes->setGridColor(osg::Vec4(0.4f, 0.4f, 0.4f, 1.0f));
    axes->build();
    root->addChild(axes);
    }

    //cube
    {
    osg::ref_ptr<osg::Geode> cubeGeode = new osg::Geode;
    osg::ref_ptr<osg::ShapeDrawable> cube =
        new osg::ShapeDrawable(new osg::Box(osg::Vec3(20,10,20), 4.0f));
    cube->setColor(osg::Vec4(0.2f, 0.5f, 0.8f, 1.0f));
    cubeGeode->addDrawable(cube.get());
    cubeGeode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    testgroup->addChild(cubeGeode.get());
    }

    //sphere
    {
    osg::ref_ptr<osg::Geode> sphereGeode = new osg::Geode;
    osg::ref_ptr<osg::ShapeDrawable> sphere =
        new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(3,2,30), 1.0f));
    sphere->setColor(osg::Vec4(1,1,0,1));
    sphereGeode->addDrawable(sphere.get());

    sphereGeode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    testgroup->addChild(sphereGeode.get());
    }


    // testgroup->addChild(createElevatedSquare());



}


void MainWindow::initOsg()
{
    viewer = osgWidget->getOsgViewer();
    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);

    // osgEarth::Util::LogarithmicDepthBuffer ldb;
    // ldb.install(viewer->getCamera());

    root = new osg::Group();
    initializeScene();

    shadowGroup = new osg::Group();
    shadowGroup->addChild(testgroup);

    root->addChild(shadowGroup);

    viewer->setSceneData(root.get());
    viewer->setCameraManipulator(new osgGA::TrackballManipulator());
    viewer->getCamera()->setClearColor(osg::Vec4(0.1, 0.1, 0.1, 1));


}

