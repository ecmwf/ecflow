//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VITEM_HPP_
#define VITEM_HPP_

class ServerHandler;
class VNode;

class VItem
{
public:
    VItem(VNode* parent) : parent_(parent) {}

    VNode* parent() const {return parent_;}
    //virtual ServerHandler* server() const;
    //node_ptr node() const {return node_;}
    virtual bool isNode() const {return false;}
    virtual bool isTopLevel() const {false;}
    virtual bool isServer() const {return false;}
    virtual bool isSuite() const {return false;}
    virtual bool isFamily() const {return false;}
    virtual bool isTask() const {return false;}
    virtual bool isAlias() const {return false;}
    virtual bool isAttribute() const {return false;}

protected:
    VNode* parent_;
};


#endif // VITEM_HPP_

