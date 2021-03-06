/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

//-*-c++-*-
#ifndef CO_ALPHA_PEAK_PIN_H
#define CO_ALPHA_PEAK_PIN_H
#include "coPin.h"
#include "sys/types.h"
#include "util/coTypes.h"

#include <osg/Array>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Group>

class coAlphaHatPin : public coPin
{
public:
    coAlphaHatPin(osg::Group *root, float Height, float Width, vvTFPyramid *myPin);
    virtual ~coAlphaHatPin();
    virtual void setPos(float x);
    void setTopWidth(float s);
    void setMax(float m);
    void setBotWidth(float m);
    float w1, w2, w3; // used in coPinEditor

protected:
    osg::ref_ptr<osg::Vec4Array> graphColor;
    osg::ref_ptr<osg::Vec3Array> graphCoord;
    osg::ref_ptr<osg::Vec3Array> graphNormal;

    osg::ref_ptr<osg::DrawElementsUShort> coordIndex;

    osg::ref_ptr<osg::Geode> graphGeode;
    osg::ref_ptr<osg::Geometry> geometry;

    void adjustGraph();
    void createGraphLists();

    osg::ref_ptr<osg::Geode> createGraphGeode();
};
#endif
