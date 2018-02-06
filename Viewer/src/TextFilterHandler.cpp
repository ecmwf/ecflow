//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "TextFilterHandler.hpp"

#include <algorithm>

#include "SessionHandler.hpp"
#include "VSettings.hpp"

TextFilterHandler* TextFilterHandler::instance_=0;

//==============================================
//
// TextFilterItem
//
//==============================================

void TextFilterItem::save(VSettings *vs) const
{
    vs->put("name",    name_);
    vs->put("filter",  filter_);
}

//==============================================
//
// TextFilterHandler
//
//==============================================

TextFilterHandler* TextFilterHandler::Instance()
{
    if(!instance_)
        instance_=new TextFilterHandler();

    return instance_;
}

TextFilterHandler::TextFilterHandler() :
    maxLatestNum_(10)
{
    readSettings();

    //items_.push_back(TextFilterItem("first","abc+++"));
    //items_.push_back(TextFilterItem("second","//S+..."));
}

bool TextFilterHandler::contains(const std::string& name,const std::string& filter) const
{
    if(filter.empty())
        return false;

    for(std::vector<TextFilterItem>::const_iterator it=items_.begin(); it !=items_.end(); ++it)
    {
        if((*it).name() == name && (*it).filter() == filter)
        {
            return true;
        }
    }

    return false;
}


void TextFilterHandler::add(const std::string& name,const std::string& filter)
{
    if(filter.empty())
        return;

    //remove if exists
    for(std::vector<TextFilterItem>::iterator it=items_.begin(); it !=items_.end(); ++it)
    {
        if((*it).name() == name)
        {
            return;
        }
    }

    //add item
    items_.push_back(TextFilterItem(name,filter));
}


void TextFilterHandler::update(const std::string& name,const std::string& filter)
{
}

void TextFilterHandler::addLatest(const std::string& name,const std::string& filter)
{
    if(filter.empty())
        return;

    //remove if exists
    for(std::vector<TextFilterItem>::iterator it=latest_.begin(); it !=latest_.end(); ++it)
    {
        if((*it).filter() == filter)
        {
            latest_.erase(it);
            break;
        }
    }

    //trim size
    while(latest_.size() >= maxLatestNum_)
    {
        latest_.pop_back();
    }

    //add item
    latest_.insert(latest_.begin(),TextFilterItem(name,filter));
}

void TextFilterHandler::remove(int index)
{
    if(index < 0 || index >= static_cast<int>(items_.size()))
        return;

    items_.erase(items_.begin()+index);
}

std::string TextFilterHandler::settingsFile()
{
    SessionItem* cs=SessionHandler::instance()->current();
    return cs->textFilterFile();
}

void TextFilterHandler::writeSettings()
{
    std::string dummyFileName="dummy";
    std::string settingsFilePath = settingsFile();
    VSettings vs(settingsFilePath);

    std::vector<VSettings> vsItems;
    for(std::size_t i = 0; i < items_.size() ; i++)
    {
        VSettings vsThisItem(dummyFileName);
        items_[i].save(&vsThisItem);
        vsItems.push_back(vsThisItem);
    }
    vs.put("saved",vsItems);

    vsItems.clear();
    for(std::size_t i = 0; i < latest_.size() ; i++)
    {
        VSettings vsThisItem(dummyFileName);
        latest_[i].save(&vsThisItem);
        vsItems.push_back(vsThisItem);
    }
    vs.put("latest",vsItems);

    vs.write();
}

void TextFilterHandler::readSettings()
{
    std::string settingsFilePath = settingsFile();
    VSettings vs(settingsFilePath);

    bool ok = vs.read(false);  // false means we don't abort if the file is not there

    if(ok)
    {
        std::vector<VSettings> vsItems;
        vs.get("saved",vsItems);
        for (std::size_t i = 0; i < vsItems.size(); i++)
        {
            std::string emptyDefault="";
            std::string name    = vsItems[i].get("name",  emptyDefault);
            std::string filter = vsItems[i].get("filter", emptyDefault);
            add(name,filter);  // add it to our in-memory list
        }

        vsItems.clear();
        vs.get("latest",vsItems);
        for (std::size_t i = 0; i < vsItems.size(); i++)
        {
            std::string emptyDefault="";
            std::string name    = vsItems[i].get("name",  emptyDefault);
            std::string filter = vsItems[i].get("filter", emptyDefault);
            addLatest(name,filter);  // add it to our in-memory list
        }
    }
}

