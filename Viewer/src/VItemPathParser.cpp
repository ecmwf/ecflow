//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VItemPathParser.hpp"

VItemPathParser::VItemPathParser(const std::string& path) : itemType_(NoType)
{
    if(path.empty())
        return;

    size_t pos;
    std::string p=path;
    if(p.find("[") == 0)
    {
       pos=p.find("]");
       if(pos != std::string::npos)
       {
           type_=path.substr(1,pos-1);
           p=path.substr(pos+1);
       }
       else
           return;
    }

    pos=p.find("://");

    if(pos != std::string::npos)
    {
        server_=p.substr(0,pos);
        p=p.substr(pos+2);
    }

    if(p.size() == 1 && p.find("/") == 0)
    {
        itemType_=ServerType;
        return;
    }

    pos=p.find_last_of(":");
    if(pos != std::string::npos)
    {
        node_=p.substr(0,pos);
        attribute_=p.substr(pos+1);
        itemType_=AttributeType;
    }
    else
    {
        node_=p;
        itemType_=NodeType;
    }
}

std::string VItemPathParser::encode(const std::string& path,const std::string& type)
{
    return "[" + type + "]" + path;
}

