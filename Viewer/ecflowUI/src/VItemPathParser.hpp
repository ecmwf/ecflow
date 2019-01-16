//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VITEMPATHPARSER_HPP
#define VITEMPATHPARSER_HPP

#include <string>

class VItemPathParser
{
public:
    enum PathFormat {DefaultFormat, // [type]server://nodepath:atttibutename
                     DiagFormat //host@port:/nodepath
                    };

    VItemPathParser(const std::string& path,PathFormat format=DefaultFormat);

    enum ItemType {ServerType,NodeType,AttributeType,NoType};

    ItemType  itemType() const {return itemType_;}
    const std::string type() const {return type_;}
    const std::string server() const {return server_;}
    const std::string host() const {return host_;}
    const std::string port() const {return port_;}
    const std::string node() const {return node_;}
    const std::string attribute() const {return attribute_;}
    std::string parent() const;

    static std::string encode(const std::string& path,const std::string& type);    
    static std::string encodeWithServer(const std::string& server,const std::string& path,
                                        const std::string& type);
    static std::string encodeAttribute(const std::string& parentPath,const std::string& attrName,const std::string& attrType);

protected:
    void extractHostPort(const std::string& server);

    ItemType itemType_;
    std::string type_;
    std::string server_;
    std::string host_;
    std::string port_;
    std::string node_;
    std::string attribute_;
};


#endif // VITEMPATHPARSER_HPP

