//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "Sound.hpp"

#include "DirectoryHandler.hpp"
#include "UiLog.hpp"
#include "VConfig.hpp"
#include "VConfigLoader.hpp"
#include "VProperty.hpp"

#include <assert.h>
#include <stdlib.h>

//#ifdef ECFLOW_QT5
//#include <QSoundEffect>
//#endif


#include <boost/algorithm/string/replace.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

Sound* Sound::instance_=NULL;


/*
    if(sound)
    {
        const char *soundCmd = "play -q /usr/share/xemacs/xemacs-packages/etc/sounds/boing.wav";
        if (system(soundCmd))
            UiLog().dbg() << "ChangeNotify:add() could not play sound alert";
*/
//#ifdef ECFLOW_QT5
//  QSoundEffect effect(dialog_);
//  effect.setSource(QUrl::fromLocalFile("file:/usr/share/xemacs/xemacs-packages/etc/sounds/boing.wav"));
//  effect.setLoopCount(1);
//  effect.setVolume(0.25f);
//  effect.play();
//#endif


Sound::Sound() :
	prevPlayedAt_(0),
	delay_(2),
	prop_(NULL)
{
	formats_=".+\\.(wav|mp3|ogg|oga)";

	sysDir_=DirectoryHandler::concatenate(DirectoryHandler::etcDir(), "sounds");

	std::vector<std::string> res;

	DirectoryHandler::findFiles(sysDir_,formats_,res);

	for(unsigned int i=0; i < res.size(); i++)
	{
		sysSounds_.push_back(res.at(i));
	}
}

Sound* Sound::instance()
{
	if(!instance_)
		instance_=new Sound();

	return instance_;
}

void Sound::playSystem(const std::string& fName,int loopCount)
{
	std::string fullName=DirectoryHandler::concatenate(sysDir_, fName);
	play(fullName,loopCount);
}

void Sound::play(const std::string& fName,int loopCount)
{
	assert(loopCount < 6);

	time_t t=time(NULL);
	if(t < prevPlayedAt_+delay_)
		return;

	if(currentCmd_.empty())
	{

	}
	else
	{
		std::string cmd=currentCmd_;
		boost::replace_first(cmd,"%FILE%",fName);
		boost::replace_first(cmd,"%REPEAT%",boost::lexical_cast<std::string>(loopCount-1));
		if(system(cmd.c_str()))
		{
            UiLog().dbg() << "Sound::play() could not play sound alert. Command: " <<  cmd;
		}
	}

	prevPlayedAt_=time(NULL);
}

void  Sound::setCurrentPlayer(const std::string& current)
{
	std::map<std::string,std::string>::const_iterator it=players_.find(current);
	if(it != players_.end())
	{
		currentPlayer_=it->first;
		currentCmd_=it->second;
	}
	else
		assert(0);
}

bool Sound::isSoundFile(const std::string& fName) const
{
	const boost::regex expr(formats_);
	boost::smatch what;
	if(boost::regex_match(fName, what,expr))
       return true;
    return false;
}

void Sound::load(VProperty* prop)
{
    UiLog().dbg() << "Sound:load() -- > begin";

	if(prop->name() != "sound")
	{
        UiLog().err() << "Sound:load() -- > no property found!";
		return;
	}
    
    Sound::instance_->prop_=prop;

    if(VProperty *pp=prop->findChild("players"))
    {
    	Q_FOREACH(VProperty* p,pp->children())
    	{
    		Sound::instance_->players_[p->strName()]=p->param("command").toStdString();
    	}
    }

    if(VProperty *pp=prop->findChild("player"))
    {
    	Sound::instance_->setCurrentPlayer(pp->valueAsStdString());
    }

    UiLog().dbg() << "Sound:load() -- > end";
}

static SimpleLoader<Sound> loaderSound("sound");
