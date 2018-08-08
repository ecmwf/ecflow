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
#include <sstream>

#include "SessionHandler.hpp"
#include "VSettings.hpp"

TextFilterHandler* TextFilterHandler::instance_=nullptr;

//==============================================
//
// TextFilterItem
//
//==============================================

void TextFilterItem::save(VSettings *vs) const
{
    vs->put("filter",  filter_);
    vs->putAsBool("matched",  matched_);
    vs->putAsBool("caseSensitive",caseSensitive_);
    vs->putAsBool("contextMenu",contextMenu_);
}

TextFilterItem TextFilterItem::make(VSettings* vs)
{
    std::string emptyDefault="";
    std::string filter = vs->get("filter", emptyDefault);
    bool matched = vs->getAsBool("matched",true);
    bool caseSensitive = vs->getAsBool("filter",false);
    bool contextMenu = vs->getAsBool("contextMenu",false);
    return TextFilterItem(filter,matched,caseSensitive,contextMenu);
}

bool TextFilterItem::operator ==(const TextFilterItem& o) const
{
    return filter_ == o.filter_ &&
           matched_ == o.matched_ && caseSensitive_ == o.caseSensitive_;
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
    maxLatestNum_(5)
{
    readSettings();
}

int TextFilterHandler::indexOf(const std::string& filter,bool matched,bool caseSensitive) const
{
    if(filter.empty())
        return -1;

    TextFilterItem item(filter,matched,caseSensitive);
    for(size_t i=0; i < items_.size(); i++)
    {
        if(items_[i] == item)
            return i;
    }

    return -1;
}

bool TextFilterHandler::contains(const std::string& filter,bool matched,bool caseSensitive) const
{
    return indexOf(filter,matched,caseSensitive) != -1;
}

bool TextFilterHandler::containsExceptOne(int index,const std::string& filter,bool matched,bool caseSensitive) const
{
    if(filter.empty())
        return false;

    TextFilterItem item(filter,matched,caseSensitive);
    for(int i=0; i < static_cast<int>(items_.size()); i++)
    {
        if(i!= index && items_[i] == item)
            return true;
    }
    return false;
}

bool TextFilterHandler::add(const TextFilterItem& item)
{
    if(item.filter().empty())
        return false;

    items_.push_back(item);
    writeSettings();
    return true;
}


bool TextFilterHandler::add(const std::string& filter,bool matched,bool caseSensitive,bool contextMenu)
{
    TextFilterItem item(filter,matched,caseSensitive,contextMenu);
    return add(item);
}

void TextFilterHandler::addLatest(const TextFilterItem& item)
{
    if(item.filter().empty())
        return;

    //Remove if exists
    auto it=std::find(latest_.begin(),latest_.end(),item) ;
    if(it != latest_.end())
       latest_.erase(it);

    //trim size
    while(static_cast<int>(latest_.size()) >= maxLatestNum_)
    {
        latest_.pop_back();
    }

    //add item to front
    latest_.insert(latest_.begin(),item);

    writeSettings();
}

void TextFilterHandler::addLatest(const std::string& filter,bool matched,bool caseSensitive,bool contextMenu)
{
    TextFilterItem item(filter,matched,caseSensitive,contextMenu);
    addLatest(item);
}

void TextFilterHandler::remove(int index)
{
    if(index < 0 || index >= static_cast<int>(items_.size()))
        return;

    items_.erase(items_.begin()+index);
    writeSettings();
}

void TextFilterHandler::update(int index,const TextFilterItem& item)
{
    if(index < 0 || index >= static_cast<int>(items_.size()))
        return;

    items_[index]=item;
    writeSettings();
}

void TextFilterHandler::allFilters(std::set<std::string>& v)
{
    v.clear();
    for(auto & item : items_)
    {
        v.insert(item.filter());
    }
    for(auto & i : latest_)
    {
        v.insert(i.filter());
    }
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
    for(auto & item : items_)
    {
        VSettings vsThisItem(dummyFileName);
        item.save(&vsThisItem);
        vsItems.push_back(vsThisItem);
    }
    vs.put("saved",vsItems);

    vsItems.clear();
    for(auto & i : latest_)
    {
        VSettings vsThisItem(dummyFileName);
        i.save(&vsThisItem);
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
        for (auto & vsItem : vsItems)
        {
            add(TextFilterItem::make(&vsItem));
        }

        vsItems.clear();
        vs.get("latest",vsItems);
        for (auto & vsItem : vsItems)
        {
            addLatest(TextFilterItem::make(&vsItem));
        }
    }
    //If there is no settings file at all we automatically add this filter
    else if(!vs.fileExists())
    {
        add(TextFilterItem("^\\+\\s",false,false));
    }
}
