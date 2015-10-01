//============================================================================
// Copyright 2014 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
//============================================================================

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
    std::string delimiters(" ");

    UserMessage::message(UserMessage::DBG, false, std::string("    in: ") + expr);

    ecf::Str::replace_all(expr, std::string("("), std::string(" ( "));
    ecf::Str::replace_all(expr, std::string(")"), std::string(" ) "));

    boost::algorithm::to_lower(expr); // convert to lowercase
    ecf::Str::split(expr, tokens, delimiters);

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



// -----------------------------------------------------------------

bool TypeNodeCondition::execute(VInfo_ptr nodeInfo)
{
    if (type_ == NodeExpressionParser::SERVER && nodeInfo->isServer())
        return true;

    if(nodeInfo->isNode())
    {
        node_ptr node = nodeInfo->node()->node();

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
};


// -----------------------------------------------------------------

bool StateNodeCondition::execute(VInfo_ptr nodeInfo)
{
    if (nodeInfo->isServer())
    {
        return (VSState::toName(nodeInfo->server()) == stateName_);
    }

    if(nodeInfo->isNode())
    {
        return (VNState::toName(nodeInfo->node()) == stateName_);
    }

    return false;
};

// -----------------------------------------------------------------

bool UserLevelCondition::execute(VInfo_ptr nodeInfo)
{
    // since we don't currently have the concept of user levels, we just 
    // return true for now

    return true;
};

// -----------------------------------------------------------------

bool NodeAttributeCondition::execute(VInfo_ptr nodeInfo)
{
    if (nodeInfo->isServer())
    {
        if (nodeAttrName_ == "locked")  
        {
            return false;   //  XXX temporary for now
        }
    }

    if(nodeInfo->isNode())
    {
        node_ptr node = nodeInfo->node()->node();

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

