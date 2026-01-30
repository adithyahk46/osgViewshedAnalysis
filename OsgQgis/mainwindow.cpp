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

osg::Vec3 projectPointOnPlane(
    const osg::Vec3& eye,
    const osg::Vec3& point,
    float planeX)
{
    osg::Vec3 dir = point - eye;

    // Ray: eye + t * dir
    // Plane: x = planeX
    float t = (planeX - eye.x()) / dir.x();

    return eye + dir * t;
}


void MainWindow::updateViewer()
{
    /* ===================== HELPERS ===================== */
    // Viewshed3DAreaAnalysis* viewshed = new Viewshed3DAreaAnalysis();
    // root->addChild(viewshed->getCameraNode().get());

    osg::ref_ptr<XYZCoordinateAxes> axes = new XYZCoordinateAxes();
    axes->setAxisLength(200.0f);
    axes->setGridUnit(10.0f);
    axes->setGridColor(osg::Vec4(0.4f, 0.4f, 0.4f, 1.0f));
    axes->build();

    root->addChild(axes);



    // float size = 10.0f;

    // osg::ref_ptr<osg::Vec3Array> planeVerts = new osg::Vec3Array;
    // planeVerts->push_back(osg::Vec3( 20.0f, -size,-size)); // Changed to z=0
    // planeVerts->push_back(osg::Vec3(20.0f, -size, size));
    // planeVerts->push_back(osg::Vec3(20.0f,  size, size));
    // planeVerts->push_back(osg::Vec3(20.0f,  size, -size));

    // osg::ref_ptr<osg::Vec3Array> planedNorms = new osg::Vec3Array;
    // planedNorms->push_back(osg::Vec3(0,0,1));

    // osg::ref_ptr<osg::Vec4Array> planeColors = new osg::Vec4Array;
    // planeColors->push_back(osg::Vec4(0.8f,0.8f,0.8f,1.0f));

    // osg::ref_ptr<osg::Geometry> planeGeom = new osg::Geometry;
    // planeGeom->setVertexArray(planeVerts.get());
    // planeGeom->setNormalArray(planedNorms.get(), osg::Array::BIND_OVERALL);
    // planeGeom->setColorArray(planeColors.get(), osg::Array::BIND_OVERALL);

    // osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
    // indices->push_back(0); indices->push_back(1); indices->push_back(2);
    // indices->push_back(2); indices->push_back(3); indices->push_back(0);
    // planeGeom->addPrimitiveSet(indices.get());

    // osg::ref_ptr<osg::Geode> planeGeode = new osg::Geode;
    // planeGeode->addDrawable(planeGeom.get());
    // root->addChild(planeGeode.get());

    // osg::ref_ptr<osg::Geode> cubeGeode = new osg::Geode;
    // osg::ref_ptr<osg::ShapeDrawable> cubeDrawable = new osg::ShapeDrawable(new osg::Box(osg::Vec3(10,0,2), 4.0f)); // Smaller cube at z=3
    // cubeDrawable->setColor(osg::Vec4(0.2f, 0.5f, 0.8f, 1.0f));
    // cubeGeode->addDrawable(cubeDrawable.get());
    // root->addChild(cubeGeode.get());

    // osg::ref_ptr<osg::Geode> spearGeod = new osg::Geode;
    // osg::ref_ptr<osg::ShapeDrawable> Sphere = new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(3,3,3), 1.0f));
    // Sphere->setColor(osg::Vec4(1,1,0,1));  // Set color on the ShapeDrawable
    // spearGeod->addDrawable(Sphere.get());
    // root->addChild(spearGeod.get());

    osg::Vec3 eye(0,0,0);
       float planeX = 20.0f;
       float size = 10.0f;
       int resolution = 60;   // plane pixels per side

       /* =================== OCCLUDERS =================== */
       /* =================== OCCLUDERS (UNCHANGED COLORS) =================== */

       osg::ref_ptr<osg::Geode> cubeGeode = new osg::Geode;
       osg::ref_ptr<osg::ShapeDrawable> cube =
           new osg::ShapeDrawable(new osg::Box(osg::Vec3(10,0,2), 4.0f));
       cube->setColor(osg::Vec4(0.2f, 0.5f, 0.8f, 1.0f));   // ORIGINAL COLOR
       cubeGeode->addDrawable(cube.get());
       root->addChild(cubeGeode.get());

       osg::ref_ptr<osg::Geode> sphereGeode = new osg::Geode;
       osg::ref_ptr<osg::ShapeDrawable> sphere =
           new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(3,3,3), 1.0f));
       sphere->setColor(osg::Vec4(1,1,0,1));               // ORIGINAL COLOR
       sphereGeode->addDrawable(sphere.get());
       root->addChild(sphereGeode.get());

       /* =================== PLANE =================== */

       osg::ref_ptr<osg::Geometry> planeGeom = new osg::Geometry;
       osg::ref_ptr<osg::Vec3Array> verts = new osg::Vec3Array;
       osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;

       float step = (2.0f * size) / resolution;

       for (int y = 0; y < resolution; ++y)
       {
           for (int z = 0; z < resolution; ++z)
           {
               float y0 = -size + y * step;
               float y1 = y0 + step;
               float z0 = -size + z * step;
               float z1 = z0 + step;

               // quad vertices
               osg::Vec3 v0(planeX, y0, z0);
               osg::Vec3 v1(planeX, y1, z0);
               osg::Vec3 v2(planeX, y1, z1);
               osg::Vec3 v3(planeX, y0, z1);

               // pixel center (for ray)
               osg::Vec3 center = (v0 + v2) * 0.5f;

               osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector =
                   new osgUtil::LineSegmentIntersector(eye, center);

               osgUtil::IntersectionVisitor iv(intersector.get());
               root->accept(iv);

               bool hit = intersector->containsIntersections();

               osg::Vec4 color = hit ?
                   osg::Vec4(1,0,0,1) :   // RED if hit
                   osg::Vec4(1,1,1,1);    // WHITE otherwise

               // two triangles per quad
               verts->push_back(v0);
               verts->push_back(v1);
               verts->push_back(v2);

               verts->push_back(v2);
               verts->push_back(v3);
               verts->push_back(v0);

               for (int i = 0; i < 6; ++i)
                   colors->push_back(color);
           }
       }

       planeGeom->setVertexArray(verts.get());
       planeGeom->setColorArray(colors.get(), osg::Array::BIND_PER_VERTEX);
       planeGeom->addPrimitiveSet(
           new osg::DrawArrays(GL_TRIANGLES, 0, verts->size()));

       osg::ref_ptr<osg::Geode> planeGeode = new osg::Geode;
       planeGeode->addDrawable(planeGeom.get());

       osg::StateSet* ss = planeGeode->getOrCreateStateSet();
       ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

       root->addChild(planeGeode.get());




       /* =================== CAMERA FOV BORDERS =================== */

       float fovY = osg::DegreesToRadians(60.0f);
       float aspect = 1.0f;
       float d = planeX;

       float halfHeight = d * tanf(fovY * 0.5f);
       float halfWidth  = halfHeight * aspect;

       osg::Vec3 tl(d, -halfWidth,  halfHeight);
       osg::Vec3 tr(d,  halfWidth,  halfHeight);
       osg::Vec3 br(d,  halfWidth, -halfHeight);
       osg::Vec3 bl(d, -halfWidth, -halfHeight);

       osg::ref_ptr<osg::Geometry> fovGeom = new osg::Geometry;
       osg::ref_ptr<osg::Vec3Array> fovVerts = new osg::Vec3Array;

       // osg::Vec3 eye(0,0,0);

       // rays
       fovVerts->push_back(eye); fovVerts->push_back(tl);
       fovVerts->push_back(eye); fovVerts->push_back(tr);
       fovVerts->push_back(eye); fovVerts->push_back(br);
       fovVerts->push_back(eye); fovVerts->push_back(bl);

       // plane rectangle
       fovVerts->push_back(tl); fovVerts->push_back(tr);
       fovVerts->push_back(tr); fovVerts->push_back(br);
       fovVerts->push_back(br); fovVerts->push_back(bl);
       fovVerts->push_back(bl); fovVerts->push_back(tl);

       fovGeom->setVertexArray(fovVerts.get());
       fovGeom->addPrimitiveSet(
           new osg::DrawArrays(GL_LINES, 0, fovVerts->size()));

       osg::ref_ptr<osg::Vec4Array> fovColor = new osg::Vec4Array;
       fovColor->push_back(osg::Vec4(0,1,0,1));   // GREEN FOV
       fovGeom->setColorArray(fovColor.get(), osg::Array::BIND_OVERALL);

       osg::ref_ptr<osg::Geode> fovGeode = new osg::Geode;
       fovGeode->addDrawable(fovGeom.get());

       osg::StateSet* ss2 = fovGeode->getOrCreateStateSet();
       ss2->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

       root->addChild(fovGeode.get());

}


void MainWindow::on_pushButton_clicked()
{
    // create a camera at origin, camera angles towords plane. the plane is recever screen , and project the object on it

    osg::Vec3 eye(0, 0, 0);        // Camera at origin
       float planeX = 20.0f;          // Receiver plane x = 20

       // ---- Object positions (same as you created earlier) ----
       osg::Vec3 cubeCenter(10, 0, 2);
       osg::Vec3 sphereCenter(3, 3, 3);

       // ---- Project points onto plane ----
       osg::Vec3 cubeProj   = projectPointOnPlane(eye, cubeCenter, planeX);
       osg::Vec3 sphereProj = projectPointOnPlane(eye, sphereCenter, planeX);

       // ---- Draw projected points as small spheres ----
       osg::ref_ptr<osg::Geode> projGeode = new osg::Geode;

       osg::ref_ptr<osg::ShapeDrawable> cubeProjDraw =
           new osg::ShapeDrawable(new osg::Sphere(cubeProj, 0.5f));
       cubeProjDraw->setColor(osg::Vec4(0, 1, 0, 1)); // green

       osg::ref_ptr<osg::ShapeDrawable> sphereProjDraw =
           new osg::ShapeDrawable(new osg::Sphere(sphereProj, 0.5f));
       sphereProjDraw->setColor(osg::Vec4(1, 0, 0, 1)); // red

       projGeode->addDrawable(cubeProjDraw.get());
       projGeode->addDrawable(sphereProjDraw.get());

       root->addChild(projGeode.get());

       // ---- Optional: draw rays from eye to projection ----
       osg::ref_ptr<osg::Geometry> rayGeom = new osg::Geometry;
       osg::ref_ptr<osg::Vec3Array> rayVerts = new osg::Vec3Array;

       rayVerts->push_back(eye);
       rayVerts->push_back(cubeProj);

       rayVerts->push_back(eye);
       rayVerts->push_back(sphereProj);

       rayGeom->setVertexArray(rayVerts.get());
       rayGeom->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, rayVerts->size()));

       osg::ref_ptr<osg::Vec4Array> rayColor = new osg::Vec4Array;
       rayColor->push_back(osg::Vec4(1,1,1,1));
       rayGeom->setColorArray(rayColor.get(), osg::Array::BIND_OVERALL);

       osg::ref_ptr<osg::Geode> rayGeode = new osg::Geode;
       rayGeode->addDrawable(rayGeom.get());

       root->addChild(rayGeode.get());
}


void MainWindow::initOsg()
{
    viewer = osgWidget->getOsgViewer();
    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);

    root = new osg::Group();
    updateViewer();

    // // Get the traits for the graphics context
    //    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    //    traits->x = 100;
    //    traits->y = 100;
    //    traits->width = 800;
    //    traits->height = 600;
    //    traits->windowDecoration = true;
    //    traits->doubleBuffer = true;
    //    traits->samples = 4;
    //    traits->sampleBuffers = 1;

    //    // Set OpenGL core profile for GL3+
    //    traits->glContextVersion = "3.3";
    //    // traits->glContextProfileMask = osg::GraphicsContext::;

    //    // Create graphics context
    //    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    //    if (!gc) {
    //        std::cerr << "Failed to create OpenGL core profile context." << std::endl;
    //        return ;
    //    }

       // Configure viewer
       osg::Camera* camera = viewer->getCamera();
       // camera->setGraphicsContext(gc);
       // camera->setViewport(0, 0, traits->width, traits->height);

       // Set initial camera position
       camera->setViewMatrixAsLookAt(
           osg::Vec3(0, 0, 0),  // Camera position (origin)
           osg::Vec3(1, 0, 0),  // Look direction (towards positive X)
           osg::Vec3(0, 0, 1)   // Up vector
       );


    viewer->setSceneData(root.get());
    viewer->setCameraManipulator(new osgGA::TrackballManipulator());
    viewer->getCamera()->setClearColor(osg::Vec4(0.1, 0.1, 0.1, 1));


}



void MainWindow::initializeBaseMap()
{
 //    viewer = osgWidget->getOsgViewer();
 //    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);

 //    /* ================= MAP ================= */
 //    osg::ref_ptr<osgEarth::Map> map = new osgEarth::Map();


 //     osgEarth::XYZImageLayer* remoteTiles = new osgEarth::XYZImageLayer();
 //     remoteTiles->setURL("https://tile.openstreetmap.org/{z}/{x}/{y}.png");
 //     remoteTiles->setName("OSM");
 //     remoteTiles->setProfile(osgEarth::Profile::create(osgEarth::Profile::SPHERICAL_MERCATOR));
 //     remoteTiles->setOpacity(1.0f);
 //    map->addLayer(remoteTiles);

 //    mapNode = new osgEarth::MapNode(map.get());


 //    const osgEarth::SpatialReference* geoSRS =
 //        osgEarth::SpatialReference::get("wgs84");

 //    osg::ref_ptr<osgEarth::GeoTransform> geoXform =
 //        new osgEarth::GeoTransform();

 //    geoXform->setPosition(
 //        osgEarth::GeoPoint(
 //            geoSRS,
 //            77.5946,   // longitude
 //            12.9716,   // latitude
 //             0.0,       // altitude
 //            osgEarth::ALTMODE_ABSOLUTE));

 //    geoXform->addChild(geode.get());

 //    /* ================= SCENE ================= */
 //    osg::ref_ptr<osg::Group> root = new osg::Group();
 //    root->addChild(mapNode.get());
 //    root->addChild(geoXform.get());

 //    viewer->setSceneData(root.get());

 //    /* ================= CAMERA ================= */
 //    osgEarth::EarthManipulator* manip =
 //        new osgEarth::Util::EarthManipulator();

 //    viewer->setCameraManipulator(manip);

 //    osgEarth::Viewpoint vp;
 //    vp.setName("Bengaluru");

 //    vp.setFocalPoint(
 //        osgEarth::GeoPoint(
 //            osgEarth::SpatialReference::get("wgs84"),
 //            77.5946,   // lon
 //            12.9716,   // lat
 //            0.0,
 //            osgEarth::ALTMODE_ABSOLUTE
 //        )
 //    );

 //    vp.setRange(
 //        osgEarth::Distance(800.0, osgEarth::Units::METERS)
 //    );

 //    vp.setHeading(
 //        osgEarth::Angle(0.0, osgEarth::Units::DEGREES)
 //    );

 //    vp.setPitch(
 //        osgEarth::Angle(-45.0, osgEarth::Units::DEGREES)
 //    );
 // viewer->realize();
 //    manip->setViewpoint(vp, 1.0);



    // viewer->home();

}



