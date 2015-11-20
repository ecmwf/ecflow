#ifndef NODEEXPRESSION_HPP_
#define NODEEXPRESSION_HPP_

//============================================================================
// Copyright 2015 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//============================================================================

#include "DState.hpp"

#include "VNState.hpp"
#include "VSState.hpp"
#include "VNode.hpp"
#include "VInfo.hpp"


// ----------------------
// Node condition classes
// ----------------------

class BaseNodeCondition;  // forward declaration

class NodeExpressionParser
{
public:
    NodeExpressionParser()  {};
    ~NodeExpressionParser() {};

    enum NodeType {SERVER, SUITE, FAMILY, TASK, ALIAS, NODE, BAD};


    static BaseNodeCondition *parseWholeExpression(std::string);
    static BaseNodeCondition *parseExpression();
    static void               setTokens(std::vector<std::string> &tokens) {tokens_ = tokens; i_ = tokens_.begin();};

    static NodeType    nodeType(const std::string &name);
    static std::string typeName(const NodeType);
    static bool        isUserLevel(const std::string &str);
    static bool        isNodeAttribute(const std::string &str);
    static bool        isWhatToSearchIn(const std::string &str, bool &isAttribute);

private:
    static std::vector<BaseNodeCondition *> popLastNOperands(std::vector<BaseNodeCondition *> &inOperands, int n);
    static std::vector<std::string> tokens_;
    static std::vector<std::string>::const_iterator i_;
};



class BaseNodeCondition
{
public:
    BaseNodeCondition() {};
    virtual ~BaseNodeCondition() {};

    bool execute(VInfo_ptr nodeInfo);
    virtual bool execute(VNode* node) = 0;
    virtual int  numOperands() {return 0;};
    virtual std::string print() = 0;
    virtual bool operand2IsArbitraryString() {return false;};

    void setOperands(std::vector<BaseNodeCondition *> ops) {operands_ = ops;};
    bool containsAttributeSearch();


protected:
    std::vector<BaseNodeCondition *> operands_;
    virtual bool searchInAttributes() {return false;};
};

// -----------------------------------------------------------------

class AndNodeCondition : public BaseNodeCondition
{
public:
    AndNodeCondition() {};
    ~AndNodeCondition() {};

    bool execute(VNode* node);
    int  numOperands() {return 2;};
    std::string print() {return std::string("and") + "(" + operands_[0]->print() + "," + operands_[1]->print() + ")";};
};

// -----------------------------------------------------------------

class OrNodeCondition : public BaseNodeCondition
{
public:
    OrNodeCondition()  {};
    ~OrNodeCondition() {};

    bool execute(VNode* node);
    int  numOperands() {return 2;};
    std::string print() {return std::string("or") + "(" + operands_[0]->print() + "," + operands_[1]->print() + ")";};
};

// -----------------------------------------------------------------

class NotNodeCondition : public BaseNodeCondition
{
public:
    NotNodeCondition()  {};
    ~NotNodeCondition() {};

    bool execute(VNode* node);
    int  numOperands() {return 1;};
    std::string print() {return std::string("not") + "(" + operands_[0]->print() + ")";};
};



// --------------------------------
// String matching utitlity classes
// --------------------------------

// note that it would be ideal for the match() function to take references to strings
// for efficiency, but this is not always possible.

class StringMatchBase
{
public:
    StringMatchBase()  {};
    ~StringMatchBase() {};

    virtual bool match(std::string searchFor, std::string searchIn) = 0;
};

class StringMatchExact : public StringMatchBase
{
public:
    StringMatchExact()  {};
    ~StringMatchExact() {};

    bool match(std::string searchFor, std::string searchIn);
};

class StringMatchContains : public StringMatchBase
{
public:
    StringMatchContains()  {};
    ~StringMatchContains() {};

    bool match(std::string searchFor, std::string searchIn);
};

class StringMatchWildcard : public StringMatchBase
{
public:
    StringMatchWildcard()  {};
    ~StringMatchWildcard() {};

    bool match(std::string searchFor, std::string searchIn);
};

class StringMatchRegexp : public StringMatchBase
{
public:
    StringMatchRegexp()  {};
    ~StringMatchRegexp() {};

    bool match(std::string searchFor, std::string searchIn);
};

// -----------------------------------------------------------------

// -------------------------
// String matching condition
// -------------------------

class StringMatchCondition : public BaseNodeCondition
{
public:
    enum MatchMode {ContainsMatch=0,WildcardMatch=1,RegexpMatch=2};

    StringMatchCondition(StringMatchCondition::MatchMode matchMode);
    ~StringMatchCondition() {if (matcher_) delete matcher_;};

    bool execute(VNode *node);
    int  numOperands() {return 2;};
    std::string print() {return operands_[0]->print() + " = " + operands_[1]->print();};
    bool operand2IsArbitraryString() {return true;};
private:
    StringMatchBase *matcher_;
};

// -----------------------------------------------------------------

// ---------------------------
// Basic true/false conditions
// ---------------------------

class TrueNodeCondition : public BaseNodeCondition
{
public:
    TrueNodeCondition()  {};
    ~TrueNodeCondition() {};

    bool execute(VNode*) {return true;};
    std::string print() {return std::string("true");};
};

class FalseNodeCondition : public BaseNodeCondition
{
public:
    FalseNodeCondition()  {};
    ~FalseNodeCondition() {};

    bool execute(VNode*) {return false;};
    std::string print() {return std::string("false");};
};

// -----------------------------------------------------------------

// -------------------
// Node type condition
// -------------------

class TypeNodeCondition : public BaseNodeCondition
{
public:
    explicit TypeNodeCondition(NodeExpressionParser::NodeType type) {type_ = type;};
    ~TypeNodeCondition() {};

    bool execute(VNode* node);
    std::string print() {return NodeExpressionParser::typeName(type_);};

private:
    NodeExpressionParser::NodeType type_;
};

// -----------------------------------------------------------------

// --------------------
// Node state condition
// --------------------

class StateNodeCondition : public BaseNodeCondition
{
public:
    explicit StateNodeCondition(QString stateName) {stateName_ = stateName;};
    ~StateNodeCondition() {};

    bool execute(VNode* node);
    std::string print() {return stateName_.toStdString();};

private:
    QString stateName_;
};

// -----------------------------------------------------------------

// --------------------
// User level condition
// --------------------

class UserLevelCondition : public BaseNodeCondition
{
public:
    explicit UserLevelCondition(QString userLevelName) {userLevelName_ = userLevelName;};
    ~UserLevelCondition() {};

    bool execute(VNode*);
    std::string print() {return userLevelName_.toStdString();};

private:
    QString userLevelName_;
};

// -----------------------------------------------------------------

// ------------------------
// Node attribute condition
// ------------------------

class NodeAttributeCondition : public BaseNodeCondition
{
public:
    explicit NodeAttributeCondition(QString nodeAttrName) {nodeAttrName_ = nodeAttrName;};
    ~NodeAttributeCondition() {};

    bool execute(VNode*);
    std::string print() {return nodeAttrName_.toStdString();};

private:
    QString nodeAttrName_;
};

// -----------------------------------------------------------------

// -----------------
// Search conditions
// -----------------

class WhatToSearchInOperand : public BaseNodeCondition
{
public:
    explicit WhatToSearchInOperand(std::string what, bool &attr);
    ~WhatToSearchInOperand();

    std::string name() {return what_;};
    bool execute(VNode* node) {return false;}; // not called
    std::string print() {return what_;};
    std::string what() {return what_;};

private:
    std::string what_;  // TODO XXX: optimise - we should store an enum here
    bool searchInAttributes_;

    void searchInAttributes(bool attr) {searchInAttributes_ = attr;};
    bool searchInAttributes() {return searchInAttributes_;};
};

// -----------------------------------------------------------------

class WhatToSearchForOperand : public BaseNodeCondition
{
public:
    explicit WhatToSearchForOperand(std::string what) {what_ = what;};
    ~WhatToSearchForOperand();

    std::string name() {return what_;};
    bool execute(VNode* node) {return false;}; // not called
    std::string print() {return what_;};
    std::string what() {return what_;};

private:
    std::string what_;
};





#endif
