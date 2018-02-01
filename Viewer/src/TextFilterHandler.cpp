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

TextFilterHandler* TextFilterHandler::instance_=0;

TextFilterHandler* TextFilterHandler::Instance()
{
    if(!instance_)
        instance_=new TextFilterHandler();

    return instance_;
}

TextFilterHandler::TextFilterHandler()
{
    items_.push_back(TextFilterItem("first","abc+++"));
    items_.push_back(TextFilterItem("second","//S+..."));
}

void TextFilterHandler::add(const std::string& name,const std::string& filter)
{
}

void TextFilterHandler::update(const std::string& name,const std::string& filter)
{
}
