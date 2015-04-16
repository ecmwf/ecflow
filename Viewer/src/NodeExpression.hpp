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

    enum NodeType {SERVER, SUITE, FAMILY, TASK, ALIAS, BAD};


    static BaseNodeCondition *parseWholeExpression(std::string);
    static BaseNodeCondition *parseExpression(std::vector<std::string> &tokens);

    static NodeType    nodeType(const std::string &name);
    static std::string typeName(const NodeType);

private:
    static std::vector<BaseNodeCondition *> popLastNOperands(std::vector<BaseNodeCondition *> &inOperands, int n);
};



class BaseNodeCondition
{
public:
    BaseNodeCondition() {};
    virtual ~BaseNodeCondition() {};

    virtual bool execute(VInfo_ptr nodeInfo) = 0;
    virtual int  numOperands() {return 0;};
    virtual std::string print() = 0;

    void setOperands(std::vector<BaseNodeCondition *> ops) {operands_ = ops;};


protected:
    std::vector<BaseNodeCondition *> operands_;
};

// -----------------------------------------------------------------

class AndNodeCondition : public BaseNodeCondition
{
public:
    AndNodeCondition() {};
    ~AndNodeCondition() {};

    bool execute(VInfo_ptr nodeInfo) {return operands_[0]->execute(nodeInfo) && operands_[1]->execute(nodeInfo);};
    int  numOperands() {return 2;};
    std::string print() {return std::string("and") + "(" + operands_[0]->print() + "," + operands_[1]->print() + ")";};
};

// -----------------------------------------------------------------

class OrNodeCondition : public BaseNodeCondition
{
public:
    OrNodeCondition()  {};
    ~OrNodeCondition() {};

    bool execute(VInfo_ptr nodeInfo) {return operands_[0]->execute(nodeInfo) || operands_[1]->execute(nodeInfo);};
    int  numOperands() {return 2;};
    std::string print() {return std::string("or") + "(" + operands_[0]->print() + "," + operands_[1]->print() + ")";};
};

// -----------------------------------------------------------------

class NotNodeCondition : public BaseNodeCondition
{
public:
    NotNodeCondition()  {};
    ~NotNodeCondition() {};

    bool execute(VInfo_ptr nodeInfo) {return !(operands_[0]->execute(nodeInfo));};
    int  numOperands() {return 1;};
    std::string print() {return std::string("not") + "(" + operands_[0]->print() + ")";};
};

// -----------------------------------------------------------------

class TrueNodeCondition : public BaseNodeCondition
{
public:
    TrueNodeCondition()  {};
    ~TrueNodeCondition() {};

    bool execute(VInfo_ptr nodeInfo) {return true;};
    std::string print() {return std::string("true");};
};

// -----------------------------------------------------------------

class FalseNodeCondition : public BaseNodeCondition
{
public:
    FalseNodeCondition()  {};
    ~FalseNodeCondition() {};

    bool execute(VInfo_ptr nodeInfo) {return false;};
    std::string print() {return std::string("false");};
};

// -----------------------------------------------------------------

class TypeNodeCondition : public BaseNodeCondition
{
public:
    TypeNodeCondition(NodeExpressionParser::NodeType type) {type_ = type;};
    ~TypeNodeCondition() {};

    bool execute(VInfo_ptr nodeInfo);
    std::string print() {return NodeExpressionParser::typeName(type_);};

private:
    NodeExpressionParser::NodeType type_;
};

// -----------------------------------------------------------------

class StateNodeCondition : public BaseNodeCondition
{
public:
    StateNodeCondition(QString stateName) {stateName_ = stateName;};
    ~StateNodeCondition() {};

    bool execute(VInfo_ptr nodeInfo);
    std::string print() {return stateName_.toStdString();};

private:
    QString stateName_;
};


#endif
