//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VItemPathParser.hpp"
#include "VAttributeType.hpp"

//format:
// [type]server://nodepath:atttibutename

VItemPathParser::VItemPathParser(const std::string& path,PathFormat format) : itemType_(NoType)
{
    if(path.empty())
        return;

    size_t pos;
    std::string p=path;

    if(format== DefaultFormat)
    {
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

        //Here we suppose that the node name cannot contain ":"
        pos=p.find_first_of(":");
        if(pos != std::string::npos)
        {
            node_=p.substr(0,pos);
            attribute_=p.substr(pos+1);
            itemType_=AttributeType;

            if(type_ == "gen-variable")
                type_="genvar";
            else if(type_ == "user-variable")
                type_="var";

            if(!VAttributeType::find(type_))
            {
                itemType_=NoType;
                type_.clear();
            }
            else
            {
                node_=p;
                itemType_=NodeType;
            }
        }
    }
    else if(format == DiagFormat)
    {
        pos=p.find(":/");
        if(pos != std::string::npos)
        {
            server_=p.substr(0,pos);
            extractHostPort(server_);
            if(p.length() > pos+1)
            {
                node_=p.substr(pos+1);
                itemType_=NodeType;
            }
            else
            {
                itemType_=ServerType;
            }
        }
    }
}

std::string VItemPathParser::encode(const std::string& path,const std::string& type)
{
    if(type.empty())
        return path;

    return "[" + type + "]" + path;
}

std::string VItemPathParser::encodeWithServer(const std::string& server,const std::string& path,const std::string& type)
{
    if(type.empty())
        return path;

    return "[" + type + "]" + server + ":/" + path;
}

std::string VItemPathParser::encodeAttribute(const std::string& parentPath,const std::string& attrName,const std::string& attrType)
{
    if(attrType.empty())
        return std::string();

    VItemPathParser parent(parentPath);

    return "[" + attrType + "]" + parent.server() + ":/" + parent.node() + ":" + attrName;
}

std::string VItemPathParser::parent() const
{
    switch(itemType_)
    {
    case ServerType:
        return std::string();
        break;
    case NodeType:
        {
            std::size_t pos=node_.find_last_of("/");
            if(pos !=  std::string::npos)
            {
                std::string n=node_.substr(0,pos);
                if(!n.empty())
                    return encodeWithServer(server_,n,type_);
                else
                    return encodeWithServer(server_,"/","server");
            }
        }
        break;
    case AttributeType:
        return encodeWithServer(server_,node_,"node");
        break;
    default:
        break;
    }

    return std::string();
}

void VItemPathParser::extractHostPort(const std::string& server)
{
    size_t pos=server.find("@");
    if(pos !=  std::string::npos)
    {
        host_=server.substr(0,pos);
        if(server.length() > pos +1)
            port_=server.substr(pos+1);
    }
}
