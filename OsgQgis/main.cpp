#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>


int usage(const char* name, const char* message)
{
    std::cerr << "Error: " << message << std::endl;
    std::cerr << "Usage: " << name << " file.earth" << std::endl;
    return -1;
}


int main(int argc, char *argv[])
{

    osgEarth::initialize();

   QApplication a(argc, argv);

    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    // QTextCodec *codec = QTextCodec::codecForLocale();
    #ifdef OSG_GL3_AVAILABLE
    format.setVersion(3, 2);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setOption(QSurfaceFormat::DebugContext);
    #else
    format.setVersion(2, 0);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setOption(QSurfaceFormat::DebugContext);
    #endif
    format.setDepthBufferSize(24);
    //format.setAlphaBufferSize(8);
    format.setSamples(8);
    format.setStencilBufferSize(8);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    QSurfaceFormat::setDefaultFormat(format);


    // Set the Fusion style
      a.setStyle(QStyleFactory::create("Fusion"));

      // Optional: Set a light palette for the Fusion style
      QPalette lightPalette;
      lightPalette.setColor(QPalette::Window, QColor(240, 240, 240));  // Light grey
      lightPalette.setColor(QPalette::WindowText, Qt::black);
      lightPalette.setColor(QPalette::Base, QColor(255, 255, 255));   // White
      lightPalette.setColor(QPalette::AlternateBase, QColor(240, 240, 240));  // Light grey
      lightPalette.setColor(QPalette::ToolTipBase, Qt::white);
      lightPalette.setColor(QPalette::ToolTipText, Qt::black);
      lightPalette.setColor(QPalette::Text, Qt::black);
      lightPalette.setColor(QPalette::Button, QColor(240, 240, 240));  // Light grey
      lightPalette.setColor(QPalette::ButtonText, Qt::black);
      lightPalette.setColor(QPalette::BrightText, Qt::red);

      lightPalette.setColor(QPalette::Highlight, QColor(76, 163, 224));  // Blue highlight
      lightPalette.setColor(QPalette::HighlightedText, Qt::white);

      a.setPalette(lightPalette);

    MainWindow w;
    w.show();
    return a.exec();
}
