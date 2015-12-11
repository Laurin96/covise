/* This file is part of COVISE.

You can use it under the terms of the GNU Lesser General Public License
version 2.1 or later, see lgpl-2.1.txt.

* License: LGPL 2+ */

#include "oscSourceFile.h"
#include "OpenScenarioBase.h"
#include "oscObjectBase.h"

#include <iostream>

#include <xercesc/dom/DOMDocument.hpp>


using namespace OpenScenario;

oscSourceFile::oscSourceFile()
{
    base = NULL;
    xmlDoc = NULL;
    includeParentElem = NULL;
}

oscSourceFile::~oscSourceFile()
{

}



/*****
 * public functions
 *****/

void oscSourceFile::initialize(OpenScenarioBase *b)
{
    base = b;
}

void oscSourceFile::setVariables(const std::string &ren, const std::string &sf)
{
    srcFileName = sf;
    rootElementName = ren;
}

void oscSourceFile::setXmlDoc(xercesc::DOMDocument *xD)
{
    xmlDoc = xD;
}

void oscSourceFile::setIncludeParentElem(xercesc::DOMElement *inclParentElem)
{
    includeParentElem = inclParentElem;
}


std::string oscSourceFile::getSrcFileName() const
{
    return srcFileName;
}

std::string oscSourceFile::getRootElementName() const
{
    return rootElementName;
}

xercesc::DOMDocument *oscSourceFile::getXmlDoc() const
{
    return xmlDoc;
}

xercesc::DOMElement *oscSourceFile::getIncludeParentElem() const
{
    return includeParentElem;
}



/*****
 * private functions
 *****/
