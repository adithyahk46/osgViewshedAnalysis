// XYZCoordinateAxes.cpp
#include "XYZCoordinateAxes.h"
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/LineWidth>
#include <osg/StateSet>

XYZCoordinateAxes::XYZCoordinateAxes()
{
    m_axisLength = 100.0f;
    m_gridUnit   = 10.0f;

    m_gridColor = osg::Vec4(0.3f, 0.3f, 0.3f, 1.0f);
    m_xColor    = osg::Vec4(1, 0, 0, 1);
    m_yColor    = osg::Vec4(0, 1, 0, 1);
    m_zColor    = osg::Vec4(0, 0, 1, 1);
}

void XYZCoordinateAxes::setAxisLength(float length)
{
    m_axisLength = length;
}

void XYZCoordinateAxes::setGridUnit(float unit)
{
    m_gridUnit = unit;
}

void XYZCoordinateAxes::setGridColor(const osg::Vec4& color)
{
    m_gridColor = color;
}

void XYZCoordinateAxes::setAxisColors(const osg::Vec4& x,
                                      const osg::Vec4& y,
                                      const osg::Vec4& z)
{
    m_xColor = x;
    m_yColor = y;
    m_zColor = z;
}

void XYZCoordinateAxes::build()
{
    removeChildren(0, getNumChildren());
    createXYGrid();
    createAxes();
}

void XYZCoordinateAxes::createAxes()
{
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> verts = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();

    // X axis
    verts->push_back(osg::Vec3(0, 0, 0));
    verts->push_back(osg::Vec3(m_axisLength, 0, 0));
    colors->push_back(m_xColor);
    colors->push_back(m_xColor);

    // Y axis
    verts->push_back(osg::Vec3(0, 0, 0));
    verts->push_back(osg::Vec3(0, m_axisLength, 0));
    colors->push_back(m_yColor);
    colors->push_back(m_yColor);

    // Z axis (intersects XY plane at origin)
    verts->push_back(osg::Vec3(0, 0, -m_axisLength));
    verts->push_back(osg::Vec3(0, 0, m_axisLength));
    colors->push_back(m_zColor);
    colors->push_back(m_zColor);

    geom->setVertexArray(verts);
    geom->setColorArray(colors, osg::Array::BIND_PER_VERTEX);
    geom->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, verts->size()));

    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    geode->addDrawable(geom);

    osg::StateSet* ss = geode->getOrCreateStateSet();
    ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    ss->setAttribute(new osg::LineWidth(1.0f));

    addChild(geode);
}

void XYZCoordinateAxes::createXYGrid()
{
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> verts = new osg::Vec3Array();

    int count = static_cast<int>(m_axisLength / m_gridUnit);

    for (int i = -count; i <= count; ++i)
    {
        float v = i * m_gridUnit;

        // Lines parallel to X
        verts->push_back(osg::Vec3(-m_axisLength, v, 0));
        verts->push_back(osg::Vec3( m_axisLength, v, 0));

        // Lines parallel to Y
        verts->push_back(osg::Vec3(v, -m_axisLength, 0));
        verts->push_back(osg::Vec3(v,  m_axisLength, 0));
    }

    geom->setVertexArray(verts);

    osg::ref_ptr<osg::Vec4Array> color = new osg::Vec4Array();
    color->push_back(m_gridColor);
    geom->setColorArray(color, osg::Array::BIND_OVERALL);

    geom->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, verts->size()));

    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    geode->addDrawable(geom);

    osg::StateSet* ss = geode->getOrCreateStateSet();
    ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    ss->setAttribute(new osg::LineWidth(1.0f));

    addChild(geode);
}
