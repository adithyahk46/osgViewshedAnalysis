QT += core gui widgets
QT += opengl openglwidgets xml


greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

CONFIG += console
CONFIG -= windows

DEFINES += CPL_DEBUG=1
DEFINES += OSG_NOTIFY_LEVEL=DEBUG_INFO
DEFINES += OSGEARTH_NOTIFY_LEVEL=INFO
DEFINES += NOMINMAX

# Define a macro that the C++ code can see
DEFINES += SOURCE_PATH=\\\"$$PWD\\\"

CONFIG += unity_build
UNITY_BUILD_BATCH_SIZE = 12

*msvc* { # visual studio spec filter
     QMAKE_CXXFLAGS += -MP
 }

PRECOMPILED_HEADER = m_precompiled_header.h
CONFIG += precompile_header
# -----------------------------
# Fast & quiet MSVC build
# -----------------------------
QMAKE_CXXFLAGS_WARN_ON =
QMAKE_CFLAGS_WARN_ON =

QMAKE_CXXFLAGS += /w /MP /FS
QMAKE_CFLAGS   += /w
QMAKE_CXXFLAGS_RELEASE += /O2 /Ob2
QMAKE_LFLAGS_RELEASE += /INCREMENTAL:NO



INDIGIS_DEPS = $$PWD/../../INDIGIS_DEPS

#QGIS setup
INCLUDEPATH += "$${INDIGIS_DEPS}/qgis_lib/include"
LIBS += -L"$${INDIGIS_DEPS}/qgis_lib/lib"
LIBS += -lqgis_core -lqgis_gui -lqgis_analysis


#extranal deps
INCLUDEPATH += "$${INDIGIS_DEPS}/external_libs/include"
LIBS += -L"$${INDIGIS_DEPS}/external_libs/lib"
LIBS += -lgdal -lproj -lgsl -lspatialindex-64  -lspatialindex_c-64


#osgEarth setup
INCLUDEPATH += $${INDIGIS_DEPS}/osgearth_libs/include
LIBS += -L"$${INDIGIS_DEPS}/osgearth_libs/lib"
# --- OSG ---
LIBS += -losg -losgDB -losgUtil -losgGA -losgViewer -losgManipulator -losgTerrain -losgText -lOpenThreads
LIBS += -losgShadow
# --- osgEarth ---
LIBS += -losgEarth -losgEarthImGui
# --- Qt OSG bridge ---
LIBS += -losgQOpenGL -lopengl32



SOURCES += \
    ViewshedAreaAnalysisWidget.cpp \
    VisibilityTestArea/VisibilityTestArea.cpp \
    XYZCoordinateAxes.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    ViewshedAreaAnalysisWidget.h \
    VisibilityTestArea/ViewshedShaders.h \
    VisibilityTestArea/VisibilityTestArea.h \
    XYZCoordinateAxes.h \
    mainwindow.h


FORMS += \
    ViewshedAreaAnalysisWidget.ui \
    mainwindow.ui


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
