#ifndef VIEWSHEDAREAANALYSISWIDGET_H
#define VIEWSHEDAREAANALYSISWIDGET_H

#include <QDialog>
#include <osg/Group>
#include <osgViewer/Viewer>
#include <osg/Vec3>
#include <QCheckBox>
#include <QColor>


#include "VisibilityTestArea/VisibilityTestArea.h"

namespace Ui {
class ViewshedAreaAnalysisWidget;
}

class ViewshedAreaAnalysisWidget : public QDialog
{
    Q_OBJECT

public:
    explicit ViewshedAreaAnalysisWidget(osg::Group* root, osgViewer::Viewer *viewer,QWidget *parent = nullptr);
    ~ViewshedAreaAnalysisWidget();

private slots:
    void on_pb_pickLocation_clicked();

    void on_pb_runORupdate_clicked();

private:
    Ui::ViewshedAreaAnalysisWidget *ui;
     osgViewer::Viewer *_viewer = nullptr;
     osg::Group* _root = nullptr;
    VisibilityTestArea* viewShed =nullptr;

};


class ColorPickerCheckBox : public QCheckBox
{
    Q_OBJECT

public:
    explicit ColorPickerCheckBox(const QColor& color,
                                 QWidget* parent = nullptr);

    QColor color() const;

signals:
    void colorChanged(const QColor& color);

private slots:
    void onClicked();

private:

    QColor m_color;

    void updateAppearance();
};


#endif // VIEWSHEDAREAANALYSISWIDGET_H
