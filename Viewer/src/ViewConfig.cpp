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

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

ViewConfig* ViewConfig::instance_=0;

ViewConfig::ViewConfig()
{
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

	//Colour params
	colour_[Unknown]= new VParameter("unknown","Un","","unknown",QColor(220,220,220));
	colour_[Complete]= new VParameter("complete","C","","complete",QColor(255,255,0));
	colour_[Queued]= new VParameter("queued","Q","","queued",QColor(224,240,255));
	colour_[Aborted]= new VParameter("aborted","Ab","","aborted",QColor(255,0,0));
	colour_[Submitted]= new VParameter("submitted","Sb","","submitted",QColor(64,224,208));
	colour_[Active]= new VParameter("active","Ac","","active",QColor(0,255,0));
	colour_[Suspended]= new VParameter("suspended","Sp","","suspended",QColor(255,166,0));
	//colour_[Active]= new VParameter("active","","",QColor(200,200,200));

	colour_[Halted]= new VParameter("suspended","","","",QColor(200,200,200));
	colour_[Shutdown]= new VParameter("suspended","","","",QColor(200,200,200));
	colour_[Meter]= new VParameter("suspended","","","",QColor(200,200,200));
	colour_[Threshold]= new VParameter("suspended","","","",QColor(200,200,200));
	colour_[Event]= new VParameter("suspended","","","",QColor(200,200,200));

	//Font params
	font_[NormalFont]=new VParameter("normal","","","",QFont());
	font_[BoldFont]=new VParameter("normal","","","",QFont());
	font_[SmallFont]=new VParameter("normal","","","",QFont());
	font_[SmallBoldFont]=new VParameter("normal","","","",QFont());

	stateMap_[DState::UNKNOWN]=Unknown;
	stateMap_[DState::COMPLETE]=Complete;
	stateMap_[DState::QUEUED]=Queued;
	stateMap_[DState::ABORTED]=Aborted;
	stateMap_[DState::SUBMITTED]=Submitted;
	stateMap_[DState::ACTIVE]=Active;
	stateMap_[DState::SUSPENDED]=Suspended;

}

ViewConfig* ViewConfig::Instance()
{
		if(!instance_)
				instance_=new ViewConfig();
		return instance_;
}

QColor ViewConfig::stateColour(DState::State st) const
{
		std::map<DState::State,PaletteItem>::const_iterator it=stateMap_.find(st);
		if(it != stateMap_.end())
				return colour(it->second);
		return QColor();
}

QString ViewConfig::stateName(DState::State st) const
{
		std::map<DState::State,PaletteItem>::const_iterator it=stateMap_.find(st);
		if(it != stateMap_.end())
				return name(it->second);
		return QString();
}

QString ViewConfig::stateShortName(DState::State st) const
{
		std::map<DState::State,PaletteItem>::const_iterator it=stateMap_.find(st);
		if(it != stateMap_.end())
				return shortName(it->second);
		return QString();
}

QColor ViewConfig::colour(PaletteItem item) const
{
		std::map<PaletteItem,VParameter*>::const_iterator it=colour_.find(item);
		if(it != colour_.end())
		{
				return it->second->toColour();
		}
		return QColor();
}

QFont ViewConfig::font(FontItem item) const
{
		std::map<FontItem,VParameter*>::const_iterator it=font_.find(item);
		if(it != font_.end())
				return it->second->toFont();
		return QFont();
}

QString ViewConfig::name(PaletteItem item) const
{
		std::map<PaletteItem,VParameter*>::const_iterator it=colour_.find(item);
		if(it != colour_.end())
		{
				return it->second->name();
		}
		return QString();
}

QString ViewConfig::shortName(PaletteItem item) const
{
		std::map<PaletteItem,VParameter*>::const_iterator it=colour_.find(item);
		if(it != colour_.end())
		{
				return it->second->shortName();
		}
		return QString();
}
