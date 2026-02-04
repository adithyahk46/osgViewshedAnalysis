QT += core gui widgets
QT += opengl openglwidgets xml


greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
DEFINES += CPL_DEBUG=1
DEFINES += OSGEARTH_NOTIFY_LEVEL=INFO


#QGIS setup
INCLUDEPATH += "../INDIGIS_DEPS/qgis_lib/include"
LIBS += -L"../INDIGIS_DEPS/qgis_lib/lib"
LIBS += -lqgis_core -lqgis_gui -lqgis_analysis


#extranal deps
INCLUDEPATH += "../INDIGIS_DEPS/external_libs/include"
LIBS += -L"../INDIGIS_DEPS/external_libs/lib"
LIBS += -lgdal -lproj -lgsl -lspatialindex-64  -lspatialindex_c-64


#osgEarth setup
INCLUDEPATH += ../INDIGIS_DEPS/osgearth_libs/include
LIBS += -L"../INDIGIS_DEPS/osgearth_libs/lib"
# --- OSG ---
LIBS += -losg -losgDB -losgUtil -losgGA -losgViewer -losgManipulator -losgTerrain -losgText -lOpenThreads
LIBS += -losgShadow
# --- osgEarth ---
LIBS += -losgEarth -losgEarthImGui
# --- Qt OSG bridge ---
LIBS += -losgQOpenGL -lopengl32



SOURCES += \
    Viewshed3DAreaAnalysis.cpp \
    VisibilityTestArea/VisibilityTestArea.cpp \
    XYZCoordinateAxes.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    Viewshed3DAreaAnalysis.h \
    VisibilityTestArea/VisibilityTestArea.h \
    XYZCoordinateAxes.h \
    mainwindow.h


FORMS += \
    mainwindow.ui


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    VisibilityTestArea/depthMap.frag \
    VisibilityTestArea/depthMap.vert \
    VisibilityTestArea/visibilityShader.frag \
    VisibilityTestArea/visibilityShader.vert
