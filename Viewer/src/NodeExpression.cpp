//============================================================================
// Copyright 2014 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
//============================================================================

#include <QRegExp>

#include <boost/algorithm/string.hpp>

#include "Str.hpp"
#include "Node.hpp"

#include "NodeExpression.hpp"
#include "UserMessage.hpp"



// -------------------------
// Expression parser classes
// -------------------------

std::vector<std::string>                 NodeExpressionParser::tokens_;
std::vector<std::string>::const_iterator NodeExpressionParser::i_;


NodeExpressionParser::NodeType NodeExpressionParser::nodeType(const std::string &name)
{
    if      (name == "server") return SERVER;
    else if (name == "suite")  return SUITE;
    else if (name == "family") return FAMILY;
    else if (name == "task")   return TASK;
    else if (name == "alias")  return ALIAS;
    else if (name == "node")   return NODE;
    else return BAD;
}

std::string NodeExpressionParser::typeName(const NodeType type)
{
         if (type == SERVER) return std::string("server");
    else if (type == SUITE)  return std::string("suite");
    else if (type == FAMILY) return std::string("family");
    else if (type == TASK)   return std::string("task");
    else if (type == ALIAS)  return std::string("alias");
    else if (type == NODE)   return std::string("node");
    else return std::string("BAD");
}

bool NodeExpressionParser::isUserLevel(const std::string &str)
{
    if (str == "oper" || str == "admin")
        return true;
    else
        return false;
}

bool NodeExpressionParser::isNodeAttribute(const std::string &str)
{
    if (str == "has_triggers" || str == "has_time" || str == "has_date" || str == "locked")
        return true;
    else
        return false;
}


bool NodeExpressionParser::isWhatToSearchIn(const std::string &str, bool &isAttribute)
{
    // list of non-attribute items that we can search in
    if (str == "node_name")
    {
        isAttribute = false;
        return true;
    }

    // list of attributes that we can search in
    else if (str == "label_name" || str == "label_value")
    {
        isAttribute = true;
        return true;
    }

    return false;
}


// NodeExpressionParser::popLastNOperands
// - utility function to remove and return the last n operands from
// - the stack
std::vector<BaseNodeCondition *> NodeExpressionParser::popLastNOperands(std::vector<BaseNodeCondition *> &inOperands, int n)
{
    std::vector<BaseNodeCondition *> resultVec;

    for (int i=0; i<n; i++)
    {
        resultVec.push_back(inOperands.back());
        inOperands.pop_back();
    }

    return resultVec;
}



BaseNodeCondition *NodeExpressionParser::parseWholeExpression(std::string expr)
{
    std::vector<std::string> tokens;
    char delimiter = ' ';
    char insideQuote = '\0';  // \0 if not inside a quote, \' if we are inside a quote
                              // will not handle the case of nested quotes!

    ecf::Str::replace_all(expr, std::string("("), std::string(" ( "));
    ecf::Str::replace_all(expr, std::string(")"), std::string(" ) "));

    //boost::algorithm::to_lower(expr); // convert to lowercase

    int    index  = 0;
    int    length = expr.length();
    std::string token  = "";


    // loop through each character in the string

    while (index < length)
    {
        char c = expr[index];

        if (c == '\'')  // a quote character?
        {
            if (insideQuote == '\'')   // this is the closing quote
                insideQuote = '\0';    // note that we are no longer inside a quote 
            else
                insideQuote = '\'';    // this is an opening quote
        }
        else if (c == delimiter && insideQuote == '\0') // a delimeter but not inside a quote?
        {
            if (token.length()>0)
                tokens.push_back(token);
            token ="";
        }
        else
            token += c;

        index++;
    }

    if(token.length()>0)
        tokens.push_back(token);


    setTokens(tokens);

    return parseExpression();
}


BaseNodeCondition *NodeExpressionParser::parseExpression()
{
    BaseNodeCondition *result = NULL;

    std::vector<BaseNodeCondition *> funcStack;
    std::vector<BaseNodeCondition *> operandStack;


    // short-circuit - if empty, then return a True condition

    if (tokens_.size() == 0)
    {
        result = new TrueNodeCondition();
    }


    while (i_ != tokens_.end() || funcStack.size() > 0)
    {
        bool tokenOk = true;
        bool updatedOperands = false;


        if (i_ != tokens_.end())
        {

            // are we expecting an arbitrary string?
            if ((funcStack.size() > 0) && (funcStack.back()->operand2IsArbitraryString()))
            {
                WhatToSearchForOperand *whatToSearchFor = new WhatToSearchForOperand(*i_);
                operandStack.push_back(whatToSearchFor);
                result = whatToSearchFor;
                updatedOperands = true;
            }
            else
            {
                bool attr = false;

                // node types
                NodeExpressionParser::NodeType type = NodeExpressionParser::nodeType(*i_);
                if (type != BAD)
                {
                    TypeNodeCondition *typeCond = new TypeNodeCondition(type);
                    operandStack.push_back(typeCond);
                    result = typeCond;
                    updatedOperands = true;
                }

                // node/server states
                else if (DState::isValid(*i_) || VSState::find(*i_))
                {
                    StateNodeCondition *stateCond = new StateNodeCondition(QString::fromStdString(*i_));
                    operandStack.push_back(stateCond);
                    result = stateCond;
                    updatedOperands = true;
                }

                // user level
                else if (isUserLevel(*i_))
                {
                    UserLevelCondition *userCond = new UserLevelCondition(QString::fromStdString(*i_));
                    operandStack.push_back(userCond);
                    result = userCond;
                    updatedOperands = true;
                }

                // node attribute
                else if (isNodeAttribute(*i_))
                {
                    NodeAttributeCondition *attrCond = new NodeAttributeCondition(QString::fromStdString(*i_));
                    operandStack.push_back(attrCond);
                    result = attrCond;
                    updatedOperands = true;
                }

                else if (isWhatToSearchIn(*i_, attr))
                {
                    WhatToSearchInOperand *searchCond = new WhatToSearchInOperand(*i_, attr);
                    operandStack.push_back(searchCond);
                    result = searchCond;
                    updatedOperands = true;
                }

                // logical operators
                else if (*i_ == "and")
                {
                    AndNodeCondition *andCond = new AndNodeCondition();
                    funcStack.push_back(andCond);
                    result = andCond;
                }

                else if (*i_ == "or")
                {
                    OrNodeCondition *orCond = new OrNodeCondition();
                    funcStack.push_back(orCond);
                    result = orCond;
                }

                else if (*i_ == "not")
                {
                    NotNodeCondition *notCond = new NotNodeCondition();
                    funcStack.push_back(notCond);
                    result = notCond;
                }

                else if(StringMatchMode::operToMode(*i_) != StringMatchMode::InvalidMatch)
                {
                    StringMatchCondition *stringMatchCond = new StringMatchCondition(StringMatchMode::operToMode(*i_));
                    funcStack.push_back(stringMatchCond);
                    result = stringMatchCond;
                }

                else if (*i_ == "(")
                {
                    ++i_;
                    result = NodeExpressionParser::parseExpression();
                    operandStack.push_back(result);
                }
                else if (*i_ == ")")
                {
                    return result;
                }

                else
                {
                    tokenOk = false;
                }
            }
        }
        
        else
        {
            // got to the end of the tokens, but we may still need to
            // update the condition stacks
            updatedOperands = true;
        }


        if (tokenOk)
        {
            // if there are enough operands on the stack for the last
            // function, pop them off and create a small tree for that function
            if (!funcStack.empty())
            {
                if(updatedOperands && (operandStack.size() >= funcStack.back()->numOperands()))
                {
                    std::vector<BaseNodeCondition *> operands;
                    result = funcStack.back();  // last function is the current result
                    operands = popLastNOperands(operandStack, result->numOperands());  // pop its operands off the stack
                    result->setOperands(operands);
                    funcStack.pop_back(); // remove the last function from the stack
                    //operandStack.clear(); // clear the operand stack
                    operandStack.push_back(result);  // store the current result
                }
            }
        }
        else
        {
            UserMessage::message(UserMessage::ERROR, false, std::string("Error parsing expression " + *i_));
            result = new FalseNodeCondition();
            return result;
        }

        if (i_ != tokens_.end())
            ++i_; // move onto the next token
    }

    UserMessage::message(UserMessage::DBG, false, std::string("    ") + result->print());
    return result;
}


bool BaseNodeCondition::execute(VInfo_ptr nodeInfo)
{
	if(!nodeInfo || !nodeInfo.get())
		return true;

	if(!nodeInfo->isServer() && !nodeInfo->isNode())
			return false;

	return execute(nodeInfo->node());
}
// -----------------------------------------------------------------

bool BaseNodeCondition::containsAttributeSearch()
{
    bool contains = false;

    // check child condition nodes
    for (int i = 0; i < operands_.size(); i++)
    {
        contains = contains | operands_[i]->containsAttributeSearch();
    }

    // check this condition node
    contains = contains | searchInAttributes();

    return contains;
}

//=========================================================================
//
//  AndNodeCondition
//
//=========================================================================

bool AndNodeCondition::execute(VNode* node)
{
	return operands_[0]->execute(node) && operands_[1]->execute(node);
}

//=========================================================================
//
//  OrNodeCondition
//
//=========================================================================

bool OrNodeCondition::execute(VNode* node)
{
	return operands_[0]->execute(node) || operands_[1]->execute(node);
}


//=========================================================================
//
//  NotNodeCondition
//
//=========================================================================

bool NotNodeCondition::execute(VNode* node)
{
	return !(operands_[0]->execute(node));
}

//=========================================================================
//
//  TypeNodeCondition
//
//=========================================================================

bool TypeNodeCondition::execute(VNode* vnode)
{
    if (type_ == NodeExpressionParser::SERVER && vnode->isServer())
        return true;

    if(!vnode->isServer())
    {
        node_ptr node = vnode->node();

        if (type_ == NodeExpressionParser::SUITE && node->isSuite())
            return true;

        if (type_ == NodeExpressionParser::TASK && node->isTask())
            return true;

        if (type_ == NodeExpressionParser::ALIAS && node->isAlias())
            return true;

        if (type_ == NodeExpressionParser::FAMILY && node->isFamily())
            return true;

        if (type_ == NodeExpressionParser::NODE)
            return true;
    }

    return false;
}

//=========================================================================
//
//  StateNodeCondition
//
//=========================================================================

bool StateNodeCondition::execute(VNode* vnode)
{
    return vnode->stateName() == stateName_;

    /*
	if (vnode->isServer())
    {
        return (VSState::toName(nodeInfo->server()) == stateName_);
    }

    if(nodeInfo->isNode())
    {
        return (VNState::toName(nodeInfo->node()) == stateName_);
    }

    return false;*/
}

//=========================================================================
//
//  UserLevelCondition
//
//=========================================================================

bool UserLevelCondition::execute(VNode* vnode)
{
    // since we don't currently have the concept of user levels, we just 
    // return true for now

    return true;
}


//=========================================================================
//
//  String match utility functions
//
//=========================================================================

bool StringMatchExact::match(std::string searchFor, std::string searchIn)
{
    return searchFor == searchIn;
}

bool StringMatchContains::match(std::string searchFor, std::string searchIn)
{
    QRegExp regexp(QString::fromStdString(searchFor));
    int index = regexp.indexIn(QString::fromStdString(searchIn));
    return (index != -1);  // -1 means no match
}

bool StringMatchWildcard::match(std::string searchFor, std::string searchIn)
{
    QRegExp regexp(QString::fromStdString(searchFor));
    regexp.setPatternSyntax(QRegExp::Wildcard);
    return regexp.exactMatch(QString::fromStdString(searchIn));
}

bool StringMatchRegexp::match(std::string searchFor, std::string searchIn)
{
    QRegExp regexp(QString::fromStdString(searchFor));
    return regexp.exactMatch(QString::fromStdString(searchIn));
}

//=========================================================================
//
//  String match condition
//
//=========================================================================

StringMatchCondition::StringMatchCondition(StringMatchMode::Mode matchMode)
{
    switch (matchMode)
    {
        case StringMatchMode::ContainsMatch:
            matcher_ = new StringMatchContains();
            break;
        case StringMatchMode::WildcardMatch:
            matcher_ = new StringMatchWildcard();
            break;
        case StringMatchMode::RegexpMatch:
            matcher_ = new StringMatchRegexp();
            break;
        default:
            UserMessage::message(UserMessage::ERROR, false, "StringMatchCondition: bad matchMode");
            matcher_ = new StringMatchExact();
            break;
    }
}



bool StringMatchCondition::execute(VNode *node)
{
    WhatToSearchForOperand *searchForOperand = static_cast<WhatToSearchForOperand*> (operands_[0]);
    WhatToSearchInOperand  *searchInOperand  = static_cast<WhatToSearchInOperand*>  (operands_[1]);

    std::string searchIn = searchInOperand->what();

    //TODO  XXXX check - name, label, variable, etc
    if (searchIn == "node_name")
    {
        bool ok = matcher_->match(searchForOperand->what(), node->strName());
        return (ok);
    }
    else
        return false;
};

// -----------------------------------------------------------------

bool NodeAttributeCondition::execute(VNode* vnode)
{
    if (vnode->isServer())
    {
        if (nodeAttrName_ == "locked")  
        {
            return false;   //  XXX temporary for now
        }
    }

    else //if(nodeInfo->isNode())
    {
        node_ptr node = vnode->node();

        if (nodeAttrName_ == "has_time")
        {
            return (node->timeVec().size()  > 0 ||
                    node->todayVec().size() > 0 ||
                    node->crons().size()    > 0);
        }
        else if (nodeAttrName_ == "has_date")
        {
            return (node->days().size()  > 0 || 
                    node->dates().size() > 0);
        }
        else if (nodeAttrName_ == "has_triggers")
        {
            return (node->triggerAst() || 
                    node->completeAst());
        }
    }

    return false;
};


WhatToSearchInOperand::WhatToSearchInOperand(std::string what, bool &attr)
{
    what_ = what;
    searchInAttributes_ = attr;
};


WhatToSearchInOperand::~WhatToSearchInOperand() {};
WhatToSearchForOperand::~WhatToSearchForOperand() {};
