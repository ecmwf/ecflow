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


NodeExpressionParser::NodeType NodeExpressionParser::nodeType(const std::string &name)
{
    if      (name == "server") return SERVER;
    else if (name == "suite")  return SUITE;
    else if (name == "family") return FAMILY;
    else if (name == "task")   return TASK;
    else if (name == "alias")  return ALIAS;
    else return BAD;
}

std::string NodeExpressionParser::typeName(const NodeType type)
{
         if (type == SERVER) return std::string("server");
    else if (type == SUITE)  return std::string("suite");
    else if (type == FAMILY) return std::string("family");
    else if (type == TASK)   return std::string("task");
    else if (type == ALIAS)  return std::string("alias");
    else return std::string("BAD");
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

    ecf::Str::replace_all(expr, std::string("("), std::string(" ( "));
    ecf::Str::replace_all(expr, std::string(")"), std::string(" ) "));

    boost::algorithm::to_lower(expr); // convert to lowercase
    ecf::Str::split(expr, tokens, delimiters);

    return parseExpression(tokens);
}


BaseNodeCondition *NodeExpressionParser::parseExpression(std::vector<std::string> &tokens)
{
    BaseNodeCondition *result = NULL;

    std::vector<BaseNodeCondition *> funcStack;
    std::vector<BaseNodeCondition *> operandStack;


    // short-circuit - if empty, then return a True condition

    if (tokens.size() == 0)
    {
        result = new TrueNodeCondition();
    }


    std::vector<std::string>::const_iterator i = tokens.begin();

    while (i != tokens.end() || funcStack.size() > 0)
    {
        bool tokenOk = true;
        bool updatedOperands = false;


        if (i != tokens.end())
        {

            if (*i == "(" || *i == ")")
            {
                // temporary to avoid errors
                i++;
                continue;
            }

            if (*i == "(")
            {
                //BaseNodeCondition subResult = NodeExpressionParser::parseExpression();
            }


            // node types
            NodeExpressionParser::NodeType type = NodeExpressionParser::nodeType(*i);
            if (type != BAD)
            {
                TypeNodeCondition *typeCond = new TypeNodeCondition(type);
                operandStack.push_back(typeCond);
                result = typeCond;
                updatedOperands = true;
            }

            // node states
            else if (DState::isValid(*i))
            {
                StateNodeCondition *stateCond = new StateNodeCondition(QString::fromStdString(*i));
                operandStack.push_back(stateCond);
                result = stateCond;
                updatedOperands = true;
            }


            // logical operators
            else if (*i == "and")
            {
                AndNodeCondition *andCond = new AndNodeCondition();
                funcStack.push_back(andCond);
                result = andCond;
            }

            else if (*i == "or")
            {
                OrNodeCondition *orCond = new OrNodeCondition();
                funcStack.push_back(orCond);
                result = orCond;
            }

            else if (*i == "not")
            {
                NotNodeCondition *notCond = new NotNodeCondition();
                funcStack.push_back(notCond);
                result = notCond;
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
            UserMessage::message(UserMessage::ERROR, true, std::string("Error parsing expression " + *i));
            result = new FalseNodeCondition();
        }

        if (i != tokens.end())
            i++; // move onto the next token
    }

    UserMessage::message(UserMessage::DBG, false, result->print());
    return result;
}



// -----------------------------------------------------------------

bool TypeNodeCondition::execute(VInfo_ptr nodeInfo)
{
    if (type_ == NodeExpressionParser::SERVER && nodeInfo->isServer())
        return true;

    if(nodeInfo->isNode())
    {
        Node *node = nodeInfo->node()->node();

        if (type_ == NodeExpressionParser::SUITE && node->isSuite())
            return true;

        if (type_ == NodeExpressionParser::TASK && node->isTask())
            return true;

        if (type_ == NodeExpressionParser::ALIAS && node->isAlias())
            return true;

        if (type_ == NodeExpressionParser::FAMILY && node->isFamily())
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
        Node *node = nodeInfo->node()->node();

        return (VNState::toName(node) == stateName_);
    }

    return false;
};

