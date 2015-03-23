//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VProperty.hpp"

#include <QDebug>
#include <QRegExp>

#include "UserMessage.hpp"

VProperty::VProperty(const std::string& name) :
   strName_(name),
   name_(QString::fromStdString(name)),
   editable_(true)
{
}

VProperty::~VProperty()
{
    Q_FOREACH(VProperty *p,children_)
    {
        delete p;
    }

    children_.clear();
}


void VProperty::setLabelText(const std::string& val)
{
    labelText_=QString::fromStdString(val);
}

void VProperty::setToolTip(const std::string& val)
{
    toolTip_=QString::fromStdString(val);
}

void VProperty::setDefaultValue(const std::string& val)
{
    if(isColour(val))
    {
        defaultValue_=toColour(val);
    }
    //text
    else
    {
    	defaultValue_=QString::fromStdString(val);
    }
    
    if(value_.isNull())
        value_=defaultValue_;
    
    qDebug() << "Prop:" << name_ << defaultValue_ << value_.value<QColor>();
}

void VProperty::setValue(const std::string& val)
{
    if(isColour(val))
    {
        value_=toColour(val);
    }

    if(!defaultValue_.isNull() &&
            defaultValue_.type() != value_.type())
    {
        value_=defaultValue_;
        //An error messages should be shown!
    }

    dispatchChange();
}

void VProperty::setValue(QVariant val)
{
    if(!defaultValue_.isNull() &&
            defaultValue_.type() != val.type())
    {
        return;
    }

    value_=val;

    dispatchChange();
}

void VProperty::addChild(VProperty *prop)
{
    children_ << prop;
}

void VProperty::addObserver(VPropertyObserver* obs)
{
	observers_ << obs;
}

void VProperty::removeObserver(VPropertyObserver* obs)
{
	observers_.removeAll(obs);
}

void VProperty::dispatchChange()
{
	Q_FOREACH(VPropertyObserver* obs,observers_)
	{
		obs->notifyChange(this);
	}
}

//=============================
//
// Static methods
//
//=============================

bool VProperty::isColour(const std::string& val)
{
    return QString::fromStdString(val).simplified().startsWith("rgb");
}

bool VProperty::isFont(const std::string& val)
{
    return QString::fromStdString(val).simplified().startsWith("font");
}

bool VProperty::isNumber(const std::string& val)
{
    return false;
}

QColor VProperty::toColour(const std::string& name)
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

QFont VProperty::toFont(const std::string& name)
{
    return QFont();
}

int VProperty::toNumber(const std::string& name)
{
    return 0;
}

VProperty* VProperty::findChild(QString name)
{
    Q_FOREACH(VProperty* p,children_)
    {
        if(p->name() == name)
            return p;
    }
    
    return 0;
}
