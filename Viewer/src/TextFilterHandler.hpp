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

#include <string>
#include <vector>

class VSettings;

class TextFilterItem
{
public:
    TextFilterItem(const std::string& name,const std::string& filter) : name_(name), filter_(filter) {}
    const std::string& name() const {return name_;}
    const std::string& filter() const {return filter_;}
    void save(VSettings *vs) const;

public:
    std::string name_;
    std::string filter_;
};

class TextFilterHandler
{
public:
    static TextFilterHandler* Instance();

    bool contains(const std::string& name,const std::string& filter) const;
    void add(const std::string& name,const std::string& filter);
    void update(const std::string& name,const std::string& filter);
    const std::vector<TextFilterItem>& items() const {return items_;}
    void addLatest(const std::string& name,const std::string& filter);
    const std::vector<TextFilterItem>& latestItems() const {return latest_;}
    void remove(int);

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
