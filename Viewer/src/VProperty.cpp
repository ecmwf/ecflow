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
   parent_(0),
   master_(0),
   useMaster_(false),
   type_(StringType),
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
        type_=ColourType;
    }
	//Font
	else if(isFont(val))
	{
	   defaultValue_=toFont(val);
	   type_=FontType;
	}
    //int
    else if(isNumber(val))
    {
       	defaultValue_=toNumber(val);
        type_=IntType;
    }
    //bool
    else if(isBool(val))
    {
        defaultValue_=toBool(val);
        type_=BoolType;
    }
    //text
    else
    {
    	defaultValue_=QString::fromStdString(val);
    	type_=StringType;
    }
    
    if(value_.isNull())
        value_=defaultValue_;
    
    //qDebug() << "Prop:" << name_ << defaultValue_ << value_.value<QColor>();
}

void VProperty::setValue(const std::string& val)
{
    bool changed=false;

	if(isColour(val))
    {
        QColor col=toColour(val);
		changed=(value_.value<QColor>() != col);
		value_=col;
    }
    else if(isFont(val))
    {
    	QFont font=toFont(val);
    	changed=(value_.value<QFont>() != font);
    	value_=font;
    }

    else if(isNumber(val))
    {
    	int num=toNumber(val);
    	changed=(value_.toInt() != num);
    	value_=num;
    }
    else if(isBool(val))
    {
    	bool b=toBool(val);
    	changed=(value_.toBool() != b);
    	value_=b;
    }

    if(!defaultValue_.isNull() &&
            defaultValue_.type() != value_.type())
    {
        changed=true;
    	value_=defaultValue_;
        //An error message should be shown!
    }

    if(changed)
    	dispatchChange();
}

void VProperty::setValue(QVariant val)
{
    if(!defaultValue_.isNull() &&
            defaultValue_.type() != val.type())
    {
        return;
    }

    bool changed=(value_ != val);

    value_=val;

    if(changed)
    	dispatchChange();
}

std::string VProperty::valueAsString() const
{
	QString s;

	switch(type_)
	{
	case StringType:
		s=value_.toString();
		break;
	case IntType:
		s=QString::number(value_.toInt());
		break;
	case BoolType:
		s=(value_.toBool() == true)?"true":"false";
		break;
	case ColourType:
		s=VProperty::toString(value_.value<QColor>());
		break;
	case FontType:
		s=VProperty::toString(value_.value<QFont>());
		break;
	default:
		break;

	}

	return s.toStdString();
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
    prop->setParent(this);
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

void VProperty::collectChildren(std::vector<VProperty*>& chVec) const
{
	Q_FOREACH(VProperty* p,children_)
	{
		chVec.push_back(p);
		p->collectChildren(chVec);
	}
}

bool VProperty:: changed() const
{
	return value_ != defaultValue_;
}

void VProperty::collectLinks(std::vector<VProperty*>& linkVec)
{
	if(link_)
		linkVec.push_back(link_);

	 Q_FOREACH(VProperty* p,children_)
	 {
		 p->collectLinks(linkVec);
	 }
}

std::string VProperty::path()
{
	if(parent_)
		return parent_->path() + "." + strName_;

	return strName_;
}

void VProperty::setMaster(VProperty* m)
{
	if(master_)
		master_->removeObserver(this);

	master_=m;
	master_->addObserver(this);
}

void VProperty::setUseMaster(bool b)
{
	assert(master_);

	if(useMaster_ != b)
	{
		useMaster_=b;

		if(useMaster_)
		{
			value_=master_->value_;
		}
	}
}

VProperty* VProperty::clone(bool addLink,bool setMaster)
{
	VProperty *cp=new VProperty(strName_);

	cp->value_=value_;
	cp->defaultValue_=defaultValue_;
	cp->type_=type_;

	if(addLink)
	{
		cp->link_=link_;
	}

	cp->params_=params_;

	if(setMaster)
	{
		cp->setMaster(this);
	}

	Q_FOREACH(VProperty* p,children_)
	{
		VProperty *ch=p->clone(addLink,setMaster);
		cp->addChild(ch);
	}

	return cp;
}

void VProperty::notifyChange(VProperty* p)
{
	if(master_ && p== master_ && useMaster_)
	{
		value_=master_->value_;
		dispatchChange();
	}
}

//=============================
//
// Static methods
//
//=============================

bool VProperty::isColour(const std::string& val)
{
    return QString::fromStdString(val).simplified().startsWith("rgb(");
}

bool VProperty::isFont(const std::string& val)
{
    return QString::fromStdString(val).simplified().startsWith("font(");
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
    QColor col;
    QRegExp rx("rgb\\((\\d+),(\\d+),(\\d+)");

    if(rx.indexIn(qn) > -1 && rx.captureCount() == 3)
    {
        col=QColor(rx.cap(1).toInt(),
                  rx.cap(2).toInt(),
                  rx.cap(3).toInt());

    }
    return col;
}

QFont VProperty::toFont(const std::string& name)
{
	QString qn=QString::fromStdString(name);
	QFont f;
	QRegExp rx("font\\((.+)\\)");
	if(rx.indexIn(qn) > -1 && rx.captureCount() == 1)
	{
		f.fromString(rx.cap(1));
	}

	return f;
}

int VProperty::toNumber(const std::string& name)
{
	QString qn=QString::fromStdString(name);
	return qn.toInt();
}

bool VProperty::toBool(const std::string& name)
{
	return (name == "true")?true:false;
}

QString VProperty::toString(QColor col)
{
	return "rgb(" + QString::number(col.red()) + "," +
			QString::number(col.green()) + "," +
			QString::number(col.blue()) + ")";
}

QString VProperty::toString(QFont f)
{
	return "font(" + f.toString() + ")";
}

