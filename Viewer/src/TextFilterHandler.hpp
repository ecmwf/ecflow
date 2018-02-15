//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef TEXTFILTERHANDLER_HPP
#define TEXTFILTERHANDLER_HPP

#include <set>
#include <string>
#include <vector>

class VSettings;

class TextFilterItem
{
public:
    TextFilterItem(const std::string& filter,bool matched=true,bool caseSensitive=false,bool contextMenu=true) :
        filter_(filter), matched_(matched), caseSensitive_(caseSensitive), contextMenu_(contextMenu) {}

    const std::string& filter() const {return filter_;}
    bool caseSensitive() const {return caseSensitive_;}
    bool matched() const {return matched_;}
    bool contextMenu() const {return contextMenu_;}
    bool setContextMenu(bool cm) {contextMenu_=cm;}
    void save(VSettings *vs) const;


    static TextFilterItem make(VSettings* vs);
    bool operator ==(const TextFilterItem& o) const;

public:
    std::string filter_;
    bool matched_;
    bool caseSensitive_;
    bool contextMenu_;
};

class TextFilterHandler
{
public:
    static TextFilterHandler* Instance();

    bool contains(const std::string& filter,bool matched,bool caseSensitive) const;
    bool containsExceptOne(int index,const std::string& filter,bool matched,bool caseSensitive) const;
    bool add(const TextFilterItem&);
    bool add(const std::string& filter,bool matched,bool caseSensitive,bool contextMenu);
    void addLatest(const TextFilterItem&);
    void addLatest(const std::string& filter,bool matched,bool caseSensitive,bool contextMenu);
    const std::vector<TextFilterItem>& items() const {return items_;}    
    const std::vector<TextFilterItem>& latestItems() const {return latest_;}
    void update(int,const TextFilterItem&);
    void remove(int);
    void allFilters(std::set<std::string>&);
    int indexOf(const std::string& filter,bool matched,bool caseSensitive) const;

protected:
    TextFilterHandler();

    std::string settingsFile();
    void readSettings() ;
    void writeSettings();

    static TextFilterHandler* instance_;
    const int maxLatestNum_;
    std::vector<TextFilterItem> items_;
    std::vector<TextFilterItem> latest_;
};


#endif // TEXTFILTERHANDLER_HPP
