#include "ViewshedAreaAnalysisWidget.h"
#include "ui_ViewshedAreaAnalysisWidget.h"



#include <QColorDialog>

ColorPickerCheckBox::ColorPickerCheckBox(const QColor& color, QWidget* parent)
    : QCheckBox(parent),
      m_color(color)
{
    setCheckable(false);
    setTristate(false);
    setFocusPolicy(Qt::NoFocus);
    setCursor(Qt::PointingHandCursor);

    updateAppearance();

    connect(this, &QCheckBox::clicked,
            this, &ColorPickerCheckBox::onClicked);
}

QColor ColorPickerCheckBox::color() const
{
    return m_color;
}

void ColorPickerCheckBox::onClicked()
{
    QColor newColor = QColorDialog::getColor(
        m_color,
        this,
        "Select Color",
        QColorDialog::ShowAlphaChannel
    );

    if (!newColor.isValid())
        return;

    m_color = newColor;
    updateAppearance();
    emit colorChanged(m_color);
}

void ColorPickerCheckBox::updateAppearance()
{
    setText(m_color.name().toUpper());

    setStyleSheet(QString(R"(
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            background-color: %1;
        }
    )").arg(m_color.name()));
}

ViewshedAreaAnalysisWidget::ViewshedAreaAnalysisWidget(osg::Group* root, osgViewer::Viewer *viewer,QWidget *parent) :
    QDialog(parent),
    _viewer(viewer),
    _root(root),
    ui(new Ui::ViewshedAreaAnalysisWidget)
{
    ui->setupUi(this);
    setWindowTitle("3D Viewshed Analysis");
    setAttribute(Qt::WA_DeleteOnClose, true);


    ColorPickerCheckBox* visibleColor = new ColorPickerCheckBox(QColor("#00FF00"), this);
    ColorPickerCheckBox* hiddenColor = new ColorPickerCheckBox(QColor("#ff0000"), this);
    ColorPickerCheckBox* boundaryColor = new ColorPickerCheckBox(QColor("#004cff"), this);

    connect(boundaryColor, &ColorPickerCheckBox::colorChanged,this, [this](const QColor& c)
    {
        qDebug() << "Color changed to:" << c.name();
        osg::Vec4 osgColor(
                c.redF(),   // 0.0 – 1.0
                c.greenF(),
                c.blueF(),
                ui->sb_boundarylineOpacity->value()/100
            );

            // viewShed->setBorderLineColor(osgColor);

    });

    ui->vl_visibleAreaColor->addWidget(visibleColor);
    ui->vl_hiddenAreaColor->addWidget(hiddenColor);
    ui->vl_boundaryColor->addWidget(boundaryColor);

    ui->sb_longitude->setValue(20);
    ui->sb_latitude->setValue(20);
    ui->sb_altitude->setValue(20);

    connect(ui->sb_longitude, &QDoubleSpinBox::valueChanged, this,[this](double value){
        viewShed->setViwerPosition({(float)value,(float)ui->sb_latitude->value(),(float)ui->sb_altitude->value()});
    });

    connect(ui->sb_latitude, &QDoubleSpinBox::valueChanged, this,[this](double value){
        viewShed->setViwerPosition({(float)ui->sb_longitude->value(),(float)value,(float)ui->sb_altitude->value()});
    });
    connect(ui->sb_altitude, &QDoubleSpinBox::valueChanged, this,[this](double value){
        viewShed->setViwerPosition({(float)ui->sb_longitude->value(),(float)ui->sb_latitude->value(),(float)value});
    });

}

ViewshedAreaAnalysisWidget::~ViewshedAreaAnalysisWidget()
{
    delete ui;
}

void ViewshedAreaAnalysisWidget::on_pb_pickLocation_clicked()
{
    // connect(MouseEventHandler::Instance(), &MouseEventHandler::mouseClickEvent, this,
    //         [this](const double lon, const double lat,const double alt,const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    // {
    //     ui->sb_longitude->setValue(lon);
    //     ui->sb_latitude->setValue(lat);
    //     ui->sb_altitude->setValue(alt);

    //     disconnect(MouseEventHandler::Instance(), &MouseEventHandler::mouseClickEvent, this, nullptr);
    // });
}

void ViewshedAreaAnalysisWidget::on_pb_runORupdate_clicked()
{
    osg::Vec3d observationPoint(20.0, 20.0,20.0);  // X, Y, Z position
      viewShed = new VisibilityTestArea(_root, _viewer, observationPoint, 200);
       // visibilityTest->setParameter(observationPoint, visibilityRadius);
       viewShed->buildModel();

}

