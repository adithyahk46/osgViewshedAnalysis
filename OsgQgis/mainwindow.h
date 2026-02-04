#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <osgViewer/Viewer>
#include <osgQOpenGL/osgQOpenGLWidget>

#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Group>
#include <osg/Camera>
#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>
#include <osg/ShapeDrawable>
#include <osg/LineWidth>

#include <osgEarth/Map>
#include <osgEarth/MapNode>
#include <osgEarth/GeoTransform>
#include <osgEarth/SpatialReference>
#include <osgEarth/XYZ>
#include <osgEarth/Utils>

#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>
#include <osgEarth/EarthManipulator>

#include <osgEarth/ImageLayer>
#include <osgEarth/XYZ>


#include <osgEarth/LocalGeometryNode>
#include <osgEarth/Geometry>
#include <osgEarth/Style>
#include <osgEarth/LineSymbol>

#include <osgEarth/Feature>

using namespace osgEarth;
using namespace osgEarth::Units;


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void initOsg();
    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;

    osgQOpenGLWidget* osgWidget = nullptr;
    osg::ref_ptr<osgViewer::Viewer> viewer;
    osg::ref_ptr<osgEarth::MapNode> mapNode;
    osg::ref_ptr<osg::Group> root ;
    osg::ref_ptr<osg::Group> shadowGroup;
    osg::ref_ptr<osg::Group> testgroup ;

    void initializeScene();
};

#endif // MAINWINDOW_H
