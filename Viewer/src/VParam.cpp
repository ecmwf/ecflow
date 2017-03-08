//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VParam.hpp"

#include "VProperty.hpp"

static uint idCounter=0;

VParam::VParam(const std::string& name) :
   name_(name),
   qName_(QString::fromStdString(name)),
   prop_(0),
   colourPropName_("fill_colour"),
   fontColourPropName_("font_colour"),
   typeColourPropName_("type_colour"),
   id_(idCounter++)
{
}

VParam::~VParam()
{
    if(prop_)
        prop_->removeObserver(this);
}

void VParam::setProperty(VProperty* prop)
{
	prop_=prop;

	label_=prop->param("label");

	if(!colourPropName_.isEmpty())
	{
		if(VProperty* p=prop->findChild(colourPropName_))
		{
			p->addObserver(this);
			colour_=p->value().value<QColor>();
		}
	}

	if(!fontColourPropName_.isEmpty())
	{
		if(VProperty* p=prop->findChild(fontColourPropName_))
		{
			p->addObserver(this);
			fontColour_=p->value().value<QColor>();
		}
    }
    if(!typeColourPropName_.isEmpty())
    {
        if(VProperty* p=prop->findChild(typeColourPropName_))
        {
            p->addObserver(this);
            typeColour_=p->value().value<QColor>();
        }
    }
}

void VParam::notifyChange(VProperty* prop)
{
	if(prop->name() == colourPropName_)
		colour_=prop->value().value<QColor>();

	else if(prop->name() == fontColourPropName_)
        fontColour_=prop->value().value<QColor>();

    else if(prop->name() == typeColourPropName_)
        typeColour_=prop->value().value<QColor>();
}
