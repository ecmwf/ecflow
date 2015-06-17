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

#include <boost/algorithm/string.hpp>


VProperty::VProperty(const std::string& name) :
   strName_(name),
   name_(QString::fromStdString(name)),
   master_(0),
   useMaster_(false),
   type_("string"),
   link_(0)
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


void VProperty::setDefaultValue(const std::string& val)
{
    //Colour
	if(isColour(val))
    {
        defaultValue_=toColour(val);
        type_="colour";
    }
	//Font
	else if(isFont(val))
	{
	   defaultValue_=toFont(val);
	   type_="font";
	}
    //int
    else if(isNumber(val))
    {
       	defaultValue_=toNumber(val);
        type_="int";
    }
    //bool
    else if(isBool(val))
    {
        defaultValue_=toBool(val);
        type_="bool";
    }
    //text
    else
    {
    	defaultValue_=QString::fromStdString(val);
    	type_="string";
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
    else if(isNumber(val))
    {
    	value_=toNumber(val);
    }
    else if(isBool(val))
    {
       	value_=toBool(val);
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

void VProperty::setParam(QString name,QString value)
{
	params_[name]=value;
}

QString VProperty::param(QString name)
{
	QMap<QString,QString>::const_iterator it=params_.find(name);
	if(it != params_.end())
			return it.value();

	return QString();
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
	QString str=QString::fromStdString(val);
	QRegExp re("\\d*");
	return (re.exactMatch(str));
}

bool VProperty::isBool(const std::string& val)
{
	return (val == "true" || val == "false");
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
	QString qn=QString::fromStdString(name);

	qDebug() << "int" << qn << qn.toInt();
	return qn.toInt();
}

bool VProperty::toBool(const std::string& name)
{
	return (name == "true")?true:false;
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

VProperty* VProperty::find(const std::string& fullPath)
{
	if(fullPath.empty())
		return NULL;

	if(fullPath == strName_)
		return this;

	std::vector<std::string> pathVec;
	boost::split(pathVec,fullPath,boost::is_any_of("."));

	if(pathVec.size() > 0)
	{
		if(pathVec.at(0) != strName_)
			return NULL;
	}

	return VProperty::find(pathVec);
}

VProperty* VProperty::find(const std::vector<std::string>& pathVec)
{
	if(pathVec.size() == 0)
	{
		return NULL;
	}

	if(pathVec.size() == 1)
	{
		return this;
	}

	//The vec size  >=2

	std::vector<std::string> rest(pathVec.begin()+1,pathVec.end());
	VProperty *n = findChild(QString::fromStdString(pathVec.at(1)));

	return n?n->find(rest):NULL;
}

void VProperty::setMaster(VProperty* m)
{
	if(master_)
		master_->removeObserver(this);

	master_=m;
	master_->addObserver(this);
}

VProperty* VProperty::derive()
{
	 VProperty *cp=new VProperty(strName_);

	 cp->defaultValue_=defaultValue_;
	 cp->value_=value_;

	 cp->setMaster(this);

	 Q_FOREACH(VProperty* p,children_)
	 {
		 VProperty *ch=p->derive();
		 cp->addChild(ch);
	 }
	 return cp;
}


