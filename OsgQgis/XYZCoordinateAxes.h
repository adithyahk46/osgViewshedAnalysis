#ifndef XYZCOORDINATEAXES_H
#define XYZCOORDINATEAXES_H


// XYZCoordinateAxes.h
#pragma once
#include <osg/Group>
#include <osg/Vec4>

class XYZCoordinateAxes : public osg::Group
{
public:
    XYZCoordinateAxes();

    void setAxisLength(float length);
    void setGridUnit(float unit);
    void setGridColor(const osg::Vec4& color);
    void setAxisColors(const osg::Vec4& x,
                       const osg::Vec4& y,
                       const osg::Vec4& z);

    void build();

private:
    void createAxes();
    void createXYGrid();

private:
    float m_axisLength;
    float m_gridUnit;

    osg::Vec4 m_gridColor;
    osg::Vec4 m_xColor;
    osg::Vec4 m_yColor;
    osg::Vec4 m_zColor;
};

#endif // XYZCOORDINATEAXES_H
