//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VParam.hpp"

#include <QDebug>
#include <QRegExp>

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


VParam::VParam(QString name,VParam::Type type) :
   name_(name),
   type_(type)
{
}

VParam::VParam(QString name,const std::map<QString,QString>& attr) :
		name_(name),
		type_(NoType)
{
	addAttributes(attr);
}


void VParam::addAttributes(const std::map<QString,QString>& attr)
{
	for(std::map<QString,QString>::const_iterator it=attr.begin(); it != attr.end(); it++)
	{
		QString key=(*it).first;
		QString val=(*it).second;
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

QString VParam::text(QString key) const
{
	std::map<QString,QString>::const_iterator it=textMap_.find(key);
	if(it != textMap_.end())
			return it->second;

	return QString();
}

QColor VParam::colour(QString key) const
{
	std::map<QString,QColor>::const_iterator it=colourMap_.find(key);
	if(it != colourMap_.end())
			return it->second;

	return QColor();
}

int VParam::number(QString key) const
{
	std::map<QString,int>::const_iterator it=numberMap_.find(key);
	if(it != numberMap_.end())
			return it->second;

	return -1;
}

QFont VParam::font(QString key) const
{
	std::map<QString,QFont>::const_iterator it=fontMap_.find(key);
	if(it != fontMap_.end())
			return it->second;

	return QFont();
}

bool VParam::isColour(QString val) const
{
	return val.simplified().startsWith("rgb");
}

bool VParam::isFont(QString val) const
{
	return val.simplified().startsWith("font");
}

bool VParam::isNumber(QString val) const
{
	return false;
}

QColor VParam::toColour(QString name) const
{
	qDebug() << name;
	QColor col;
	QRegExp rx("rgb\\((\\d+),(\\d+),(\\d+)");

	if(rx.indexIn(name) > -1 && rx.captureCount() == 3)
	{
	  	col=QColor(rx.cap(1).toInt(),
			      rx.cap(2).toInt(),
			      rx.cap(3).toInt());

	}

	qDebug() << col;

	return col;
}

QFont VParam::toFont(QString name) const
{
	return QFont();
}

int VParam::toNumber(QString name) const
{
	return 0;
}

int  VParam::toInt(VParam::Type t)
{
	return static_cast<int>(t);
}

VParam::Type VParam::toType(int v)
{
	return static_cast<VParam::Type>(v);
}

