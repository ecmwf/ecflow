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

class TextFilterItem
{
public:
    TextFilterItem(const std::string& name,const std::string& filter) : name_(name), filter_(filter) {}
    const std::string& name() const {return name_;}
    const std::string& filter() const {return filter_;}

public:
    std::string name_;
    std::string filter_;
};

class TextFilterHandler
{
public:
    static TextFilterHandler* Instance();

    void add(const std::string& name,const std::string& filter);
    void update(const std::string& name,const std::string& filter);
    const std::vector<TextFilterItem>& items() const {return items_;}

protected:
    TextFilterHandler();

    static TextFilterHandler* instance_;
    std::vector<TextFilterItem> items_;
    std::string latest_;
};


#endif // TEXTFILTERHANDLER_HPP
