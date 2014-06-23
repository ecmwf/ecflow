//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "ViewConfig.hpp"

#include <QDebug>
#include <QRegExp>

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

ViewConfig* ViewConfig::instance_=0;

//============================================
//
// VParameter
//
//============================================

VParameter::VParameter(QString name,const std::map<QString,QString>& attr) : name_(name), attr_(attr)
{
	if(attribute("type") == "colour")
	{
		default_=toColour(attribute("default"));
	}
	value_=default_;
}

QString VParameter::attribute(QString name)
{
	std::map<QString,QString>::iterator it=attr_.find(name);
	if(it != attr_.end())
		return it->second;

	return QString();
}

QColor VParameter::toColour(QString name)
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

//============================================
//
// ViewConfig
//
//============================================

ViewConfig::ViewConfig()
{
	//Read parameters into a map
	std::string sysDir("/home/graphics/cgr/ecflowview_config.options");
	readParams(sysDir);

	//Store state params in a map. In order to make it work the state parameter names read
	//from the param file must match the names State::toString() returns!
	std::vector<DState::State> sv=DState::states();
	for(std::vector<DState::State>::const_iterator it=sv.begin(); it != sv.end(); it++)
	{
		std::map<QString,VParameter*>::iterator itP=params_.find(QString::fromStdString(DState::toString(*it)));
		if(itP != params_.end())
			stateParams_[*it]=itP->second;
	}

	//Set configuration directory name and create it.
	if(char *h=getenv("HOME"))
	{
		configDir_=std::string(h);
		configDir_+="/.ecflowview";
		if(access(configDir_.c_str(),F_OK) != 0 )
		{
			if(mkdir(configDir_.c_str(),0777) == -1)
			{
				//error
			}
		}
	}

	//Set rc directory name and create it.
	if(char *h=getenv("HOME"))
	{
		rcDir_=std::string(h);
		rcDir_+="/.ecflowrc";
	}
}

ViewConfig* ViewConfig::Instance()
{
	if(!instance_)
			instance_=new ViewConfig();
	return instance_;
}

/*
bool ViewConfig::load(const std::string& name)
{
	profileName_=name;
	profileDir_=configDir_+ "/" + profileName + ".prof";

	QFile file(QString::fromStdString(profileFile_));
	if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		return false;
	}

	//Local to config
	//read colour
	//read font
	//read server

	//read tabs
	tabFile_=profileDir_+ "/" + "tabs.txt";

	return true;
}

bool ViewConfig::save(const std::string& name)
{
	profileName_=name;
	profileDir_=configDir_+ "/" + profileName + ".prof";

	QFile file(QString::fromStdString(profileFile_));
	if(!file.open(QIODevice::Writeonly | QIODevice::Text))
	{
		return false;
	}

	//Local to config
	//read colour
	//read font
	//read server

	//read tabs
	tabFile_=profileDir_+ "/" + "tabs.txt";

	return true;
}
*/

QColor ViewConfig::stateColour(DState::State st) const
{
	std::map<DState::State,VParameter*>::const_iterator it=stateParams_.find(st);
	if(it != stateParams_.end())
			return it->second->toColour();
	return QColor();
}

QString ViewConfig::stateName(DState::State st) const
{
	std::map<DState::State,VParameter*>::const_iterator it=stateParams_.find(st);
	if(it != stateParams_.end())
			return it->second->name();
	return QString();
}

QString ViewConfig::stateShortName(DState::State st) const
{
	std::map<DState::State,VParameter*>::const_iterator it=stateParams_.find(st);
	if(it != stateParams_.end())
			return it->second->attribute("shortname");
	return QString();
}

QColor ViewConfig::colour(QString parName) const
{
	std::map<QString,VParameter*>::const_iterator it=params_.find(parName);
	if(it != params_.end())
			return it->second->toColour();
	return QColor();
}

void ViewConfig::readParams(const std::string& parFile)
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
		//QMessageBox::warning(0, QString("Metview"),
		//				 tr("Error, unable to parse JSON response from server"));
		return;
	}

	// for each parameter
	for(ptree::const_iterator itPar = pt.begin(); itPar != pt.end(); ++itPar)
	{
		QString name=QString::fromStdString(itPar->first);
		ptree const &parPt = itPar->second;

		std::map<QString,QString> vals;
		for(ptree::const_iterator itV = parPt.begin(); itV != parPt.end(); ++itV)
		{
			vals[QString::fromStdString(itV->first)]=QString::fromStdString(itV->second.get_value<std::string>());
		}
		VParameter *par=new VParameter(name,vals);
		params_[name]=par;
	}
}
