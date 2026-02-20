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

 osg::ref_ptr<osg::Geode> createElevatedSquare()
{
    std::string dt2Path =
        "C:/Users/pnmt1054/Adithya_working_directory/Data/Elevation/43J11.dt2";

    std::string texturePath =
        "C:/Users/pnmt1054/Videos/Screen Recordings/Screenshot 2026-02-19 115004.png";

    std::ifstream file(dt2Path, std::ios::binary);
    if (!file)
    {
        std::cout << "Failed to open DT2 file!" << std::endl;
    }

    // ===============================
    // 1. Read DT2 Dimensions
    // ===============================
    char uhl[80];
    file.read(uhl, 80);

    std::string colStr(uhl + 47, 4);
    std::string rowStr(uhl + 51, 4);

    int width  = std::stoi(colStr);
    int height = std::stoi(rowStr);

    file.seekg(648 + 2700, std::ios::cur);

    std::vector<std::vector<float>> elevation(height,
        std::vector<float>(width, 0.0f));

    for (int col = 0; col < width; ++col)
    {
        file.seekg(8, std::ios::cur);

        for (int row = 0; row < height; ++row)
        {
            unsigned char bytes[2];
            file.read((char*)bytes, 2);

            short value = (short)((bytes[0] << 8) | bytes[1]);

            if (value == -32767)
                value = 0;

            elevation[row][col] = (float)value;
        }

        file.seekg(4, std::ios::cur);
    }

    file.close();

    // ===============================
    // 2. Create Geometry
    // ===============================
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec3Array> normals  = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec2Array> texcoords = new osg::Vec2Array();
    osg::ref_ptr<osg::DrawElementsUInt> indices =
        new osg::DrawElementsUInt(GL_TRIANGLES);

    float spacing = 1.0f;
    float heightScale = 0.05f;

    float halfWidth  = (width  - 1) * spacing * 0.5f;
    float halfHeight = (height - 1) * spacing * 0.5f;

    // Create vertices + texcoords
    for (int r = 0; r < height; ++r)
    {
        for (int c = 0; c < width; ++c)
        {
            float x = c * spacing - halfWidth;
            float y = r * spacing - halfHeight;
            float z = elevation[r][c] * heightScale;

            vertices->push_back(osg::Vec3(x, y, z));

            texcoords->push_back(osg::Vec2(
                (float)c / (width - 1),
                1.0f - (float)r / (height - 1)
            ));

            normals->push_back(osg::Vec3(0.0f, 0.0f, 0.0f)); // initialize
        }
    }

    // Create indices
    for (int r = 0; r < height - 1; ++r)
    {
        for (int c = 0; c < width - 1; ++c)
        {
            int i0 = r * width + c;
            int i1 = r * width + c + 1;
            int i2 = (r + 1) * width + c;
            int i3 = (r + 1) * width + c + 1;

            indices->push_back(i0);
            indices->push_back(i1);
            indices->push_back(i2);

            indices->push_back(i1);
            indices->push_back(i3);
            indices->push_back(i2);

            // ----- NORMAL ACCUMULATION -----

            osg::Vec3 v0 = (*vertices)[i0];
            osg::Vec3 v1 = (*vertices)[i1];
            osg::Vec3 v2 = (*vertices)[i2];
            osg::Vec3 v3 = (*vertices)[i3];

            osg::Vec3 n0 = (v1 - v0) ^ (v2 - v0);
            n0.normalize();

            (*normals)[i0] += n0;
            (*normals)[i1] += n0;
            (*normals)[i2] += n0;

            osg::Vec3 n1 = (v3 - v1) ^ (v2 - v1);
            n1.normalize();

            (*normals)[i1] += n1;
            (*normals)[i3] += n1;
            (*normals)[i2] += n1;
        }
    }

    // Normalize accumulated normals
    for (unsigned int i = 0; i < normals->size(); ++i)
    {
        (*normals)[i].normalize();
    }

    // ===============================
    // 3. Assign Geometry Arrays
    // ===============================
    geometry->setVertexArray(vertices.get());
    geometry->setNormalArray(normals.get(), osg::Array::BIND_PER_VERTEX);
    geometry->setTexCoordArray(0, texcoords.get());
    geometry->addPrimitiveSet(indices.get());

    // ===============================
    // 4. Apply Texture
    // ===============================
    osg::ref_ptr<osg::Image> textureImage =
        osgDB::readImageFile(texturePath);

    if (!textureImage)
    {
        std::cout << "Failed to load texture!" << std::endl;
    }

    osg::ref_ptr<osg::Texture2D> texture =
        new osg::Texture2D(textureImage.get());

    texture->setFilter(osg::Texture::MIN_FILTER,
                       osg::Texture::LINEAR_MIPMAP_LINEAR);
    texture->setFilter(osg::Texture::MAG_FILTER,
                       osg::Texture::LINEAR);
    texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);

    // ===============================
    // 5. Create Geode + State
    // ===============================
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    geode->addDrawable(geometry.get());\


    osg::ref_ptr<osg::Light> light = new osg::Light;
    light->setLightNum(0);
    light->setPosition(osg::Vec4(0.0f, 0.0f, 1000.0f, 1.0f));

    osg::ref_ptr<osg::LightSource> lightSource = new osg::LightSource;
    lightSource->setLight(light.get());

    lightSource->addChild(geode.get());

    osg::StateSet* stateSet = geode->getOrCreateStateSet();
    stateSet->setTextureAttributeAndModes(
        0, texture.get(), osg::StateAttribute::ON);


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

    // testgroup->addChild(cubeGeode.get());
    }

    //sphere
    {
    osg::ref_ptr<osg::Geode> sphereGeode = new osg::Geode;
    osg::ref_ptr<osg::ShapeDrawable> sphere =
        new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(3,2,30), 1.0f));
    sphere->setColor(osg::Vec4(1,1,0,1));
    sphereGeode->addDrawable(sphere.get());

    sphereGeode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    // testgroup->addChild(sphereGeode.get());
    }


     osg::ref_ptr<osg::Geode> geode = createElevatedSquare();
    osg::ref_ptr<osg::MatrixTransform> xform =new osg::MatrixTransform();

    xform->addChild(geode.get());

    xform->setMatrix(osg::Matrix::translate(osg::Vec3(0,0,-78)));

    testgroup->addChild(xform);



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

