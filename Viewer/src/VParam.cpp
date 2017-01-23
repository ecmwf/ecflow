//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VParam.hpp"

#include <QDebug>

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

/*
void VParam::addAttributes(const std::map<std::string,std::string>& attr)
{
	for(std::map<std::string,std::string>::const_iterator it=attr.begin(); it != attr.end(); it++)
	{
		std::string key=it->first;
		std::string val=it->second;
		if(isColour(val))
		{
			colourMap_[key]=toColour(val);
		}
		else if(isFont(val))
		{
			fontMap_[key]=toFont(val);
		}
		else if(isNumber(val))
		{
			numberMap_[key]=1;
		}

		textMap_[key]=val;
	}
}

std::string VParam::text(const std::string& key) const
{
	std::map<std::string,std::string>::const_iterator it=textMap_.find(key);
	if(it != textMap_.end())
			return it->second;

	return std::string();
}

QColor VParam::colour(const std::string& key) const
{
	std::map<std::string,QColor>::const_iterator it=colourMap_.find(key);
	if(it != colourMap_.end())
			return it->second;

	return QColor();
}

int VParam::number(const std::string& key) const
{
	std::map<std::string,int>::const_iterator it=numberMap_.find(key);
	if(it != numberMap_.end())
			return it->second;

	return -1;
}

QFont VParam::font(const std::string& key) const
{
	std::map<std::string,QFont>::const_iterator it=fontMap_.find(key);
	if(it != fontMap_.end())
			return it->second;

	return QFont();
}

bool VParam::isColour(const std::string& val)
{
	return QString::fromStdString(val).simplified().startsWith("rgb");
}

bool VParam::isFont(const std::string& val)
{
	return QString::fromStdString(val).simplified().startsWith("font");
}

bool VParam::isNumber(const std::string& val)
{
	return false;
}

QColor VParam::toColour(const std::string& name)
{
	QString qn=QString::fromStdString(name);
	qDebug() << qn;
	QColor col;
	QRegExp rx("rgb\\((\\d+),(\\d+),(\\d+)");

	if(rx.indexIn(qn) > -1 && rx.captureCount() == 3)
	{
	  	col=QColor(rx.cap(1).toInt(),
			      rx.cap(2).toInt(),
			      rx.cap(3).toInt());

	}

	qDebug() << col;

	return col;
}

QFont VParam::toFont(const std::string& name)
{
	return QFont();
}

int VParam::toNumber(const std::string& name)
{
	return 0;
}


void VParam::init(const std::string& parFile,const std::string id,std::map<std::string,std::map<std::string,std::string> >& vals)
{
	//Parse param file using the boost JSON property tree parser
	using boost::property_tree::ptree;
	ptree pt;

	try
	{
		read_json(parFile,pt);
	}
	catch (const boost::property_tree::json_parser::json_parser_error& e)
	{
		 std::string errorMessage = e.what();
		 UserMessage::message(UserMessage::ERROR, true, std::string("Error, unable to parse JSON attributes file : " + errorMessage));
		 return;
	}

	ptree::const_iterator itTop = pt.begin();
	if(itTop == pt.end() || itTop->first != id)
	{
		return;
	}

	//For each parameter
	for(ptree::const_iterator itPar = itTop->second.begin(); itPar != itTop->second.end(); ++itPar)
	{
		std::string name=itPar->first;
		ptree const &parPt = itPar->second;

		//std::map<QString,QString> vals;
		for(ptree::const_iterator itV = parPt.begin(); itV != parPt.end(); ++itV)
		{
			vals[name].insert(make_pair(itV->first,itV->second.get_value<std::string>()));
		}
	}
}
*/

/*QStringList VParam::toList(const boost::property_tree::ptree& array)
{
	QStringList lst;
	for(boost::property_tree::ptree::const_iterator it = array.begin(); it != array.end(); ++it)
	{
			std::string name=it->second.get_value<std::string>();
			lst << QString::fromStdString(name);
	}

	return lst;
}
*/


