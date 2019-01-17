//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
//============================================================================

#include <QDateTime>
#include <QRegExp>
#include <QtGlobal>

#include <boost/algorithm/string.hpp>

#include "Str.hpp"
#include "Node.hpp"
#include "Submittable.hpp"

#include "NodeExpression.hpp"
#include "MenuHandler.hpp"
#include "ServerHandler.hpp"
#include "UiLog.hpp"
#include "UIDebug.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "VNode.hpp"
#include "VNodeMover.hpp"

//#define _UI_NODEXPRESSIONPARSEER_DEBUG

// -------------------------
// Expression parser classes
// -------------------------

NodeExpressionParser* NodeExpressionParser::instance_=nullptr;


NodeExpressionParser* NodeExpressionParser::instance()
{
    if(!instance_)
        instance_=new NodeExpressionParser;

    return instance_;
}

NodeExpressionParser::NodeExpressionParser()
{
    nameToNodeType_["server"]=SERVER;
    nameToNodeType_["suite"]=SUITE;
    nameToNodeType_["family"]=FAMILY;
    nameToNodeType_["task"]=TASK;
    nameToNodeType_["alias"]=ALIAS;
    nameToNodeType_["node"]=NODE;

    for(std::map<std::string,NodeType>::const_iterator it=nameToNodeType_.begin();  it != nameToNodeType_.end(); ++it)
    {
        nodeTypeToName_[it->second]=it->first;
    }

    QStringList attrNames;
    attrNames << "meter" << "event" << "repeat" << "trigger" << "label" << "time" << "date" << "late" << "limit" <<
                 "limit" << "limiter" << "var" << "genvar";
    Q_FOREACH(QString s,attrNames)
    {
        VAttributeType *t=VAttributeType::find(s.toStdString());
        Q_ASSERT(t);
        nameToAttrType_[s.toStdString()]=t;
    }

    badTypeStr_="BAD";
    badAttributeStr_="BAD";
}

NodeExpressionParser::NodeType NodeExpressionParser::nodeType(const std::string &name) const
{
    auto it=nameToNodeType_.find(name);
    if(it != nameToNodeType_.end())
        return it->second;

    return BAD;
}

const std::string& NodeExpressionParser::typeName(const NodeType& type) const
{
    auto it=nodeTypeToName_.find(type);
    if(it != nodeTypeToName_.end())
        return it->second;

    return badTypeStr_;
}

VAttributeType* NodeExpressionParser::toAttrType(const std::string &name) const
{
    auto it=nameToAttrType_.find(name);
    if(it != nameToAttrType_.end())
        return it->second;

    return nullptr;
}

bool NodeExpressionParser::isMenuMode(const std::string &str) const
{
    if (str == "oper" || str == "admin" || str == "defStatusMenuModeControl")
        return true;

    return false;
}

bool NodeExpressionParser::isEnvVar(const std::string &str) const
{
    if (str == "ECFLOWUI_ECMWF_OPERATOR_MODE" || str == "ECFLOWUI_DEVELOP_MODE")
        return true;

    return false;
}

bool NodeExpressionParser::isNodeHasAttribute(const std::string &str) const
{
    if (str == "has_triggers" || str == "has_time" || str == "has_date" || str == "locked")
        return true;

    return false;
}

bool NodeExpressionParser::isNodeFlag(const std::string &str) const
{
    if (str == "is_late" || str == "has_message" ||
        str == "is_rerun" || str == "is_waiting" || str == "is_zombie" || str == "is_migrated" ||
        str ==  "is_ecfcmd_failed" || str == "is_killed")
        return true;

    return false;
}


bool NodeExpressionParser::isWhatToSearchIn(const std::string &str, bool &isAttr) const
{
    // list of non-attribute items that we can search in
    if (str == "node_name" || str == "node_path")
    {
        isAttr = false;
        return true;
    }

    // list of attributes that we can search in
    else if (str == "var_name" || str =="var_value" || str =="var_type" ||
        str == "label_name" || str == "label_value" ||
        str == "meter_name" ||  str == "meter_value" ||
        str == "event_name" || str == "event_value" ||
        str == "date_name" || str == "time_name" ||
        str == "limit_name" || str == "limit_value" || str == "limit_max" ||
        str == "limiter_name" ||
        str == "repeat_name" || str == "repeat_value" ||
        str == "trigger_expression" )
    {
        isAttr = true;
        return true;
    }

    return false;
}


bool NodeExpressionParser::isAttributeState(const std::string &str) const
{
    return (str == "event_set" || str == "event_clear" ||
            str == "repeat_date" || str == "repeat_int" || str == "repeat_string" || str == "repeat_enum" ||
            str == "repeat_day");
}

bool NodeExpressionParser::isIsoDate(const std::string &str) const
{
    if(str.size() == 19)
    {
        QDateTime d=QDateTime::fromString(QString::fromStdString(str),Qt::ISODate);
        return d.isValid();
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



BaseNodeCondition *NodeExpressionParser::parseWholeExpression(const std::string& exprIn, bool caseSensitiveStringMatch)
{       
    std::string expr=exprIn;

    std::vector<std::string> tokens;
    char delimiter = ' ';
    char insideQuote = '\0';  // \0 if not inside a quote, \' if we are inside a quote
                              // will not handle the case of nested quotes!

    UiLog().dbg() << "parseWholeExpression:    " << expr;

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

    return parseExpression(caseSensitiveStringMatch);
}


BaseNodeCondition *NodeExpressionParser::parseExpression(bool caseSensitiveStringMatch)
{
    bool returnEarly = false;
    BaseNodeCondition *result = nullptr;

    std::vector<BaseNodeCondition *> funcStack;
    std::vector<BaseNodeCondition *> operandStack;


    // short-circuit - if empty, then return a True condition

    if(tokens_.size() == 0)
    {
        result=new TrueNodeCondition();
    }

    while (!returnEarly && i_ != tokens_.end())
    {
        bool tokenOk = true;
        bool updatedOperands = false;

        if (i_ != tokens_.end())
        {

            // are we expecting an arbitrary string?
            if ((funcStack.size() > 0) && (funcStack.back()->operand2IsArbitraryString()))
            {
                auto *whatToSearchFor = new WhatToSearchForOperand(*i_);
                operandStack.push_back(whatToSearchFor);
                result = whatToSearchFor;
                updatedOperands = true;
            }
            else
            {
                bool attr = false;
                VAttributeType* attrType=nullptr;
                //NodeExpressionParser::AttributeType attrType=NodeExpressionParser::BADATTRIBUTE;

                // node types
                NodeExpressionParser::NodeType type = nodeType(*i_);
                if (type != BAD)
                {
                    auto *typeCond = new TypeNodeCondition(type);
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

                // node menu mode
                else if (isMenuMode(*i_))
                {
                    NodeMenuModeCondition *userCond = new NodeMenuModeCondition(QString::fromStdString(*i_));
                    operandStack.push_back(userCond);
                    result = userCond;
                    updatedOperands = true;
                }

                // node has attribute
                else if (isNodeHasAttribute(*i_))
                {
                    NodeAttributeCondition *attrCond = new NodeAttributeCondition(QString::fromStdString(*i_));
                    operandStack.push_back(attrCond);
                    result = attrCond;
                    updatedOperands = true;
                }
                // node flag
                else if (isNodeFlag(*i_))
                {
                    NodeFlagCondition *flagCond = new NodeFlagCondition(QString::fromStdString(*i_));
                    operandStack.push_back(flagCond);
                    result = flagCond;
                    updatedOperands = true;
                }
                // node status change date
                else if (*i_ == "status_change_time")
                {
                    auto *chDateCond = new NodeStatusChangeDateCondition();
                    operandStack.push_back(chDateCond);
                    result = chDateCond;
                    updatedOperands = true;
                }
                // node attribute type
                //else if ((attrType = toAttrType(*i_)) != NodeExpressionParser::BADATTRIBUTE)
                else if ((attrType = toAttrType(*i_)) != nullptr)
                {
                    auto *attrCond = new AttributeCondition(attrType);
                    operandStack.push_back(attrCond);
                    result = attrCond;
                    updatedOperands = true;
                }

                // node attribute state
                else if (isAttributeState(*i_))
                {
                    AttributeStateCondition *attrStateCond = new AttributeStateCondition(QString::fromStdString(*i_));
                    operandStack.push_back(attrStateCond);
                    result = attrStateCond;
                    updatedOperands = true;
                }

                // env var
                else if (isEnvVar(*i_))
                {
                    EnvVarCondition *envCond = new EnvVarCondition(QString::fromStdString(*i_));
                    operandStack.push_back(envCond);
                    result = envCond;
                    updatedOperands = true;
                }

                else if (isWhatToSearchIn(*i_, attr))
                {
                    WhatToSearchInOperand *searchCond = new WhatToSearchInOperand(*i_, attr);
                    operandStack.push_back(searchCond);
                    result = searchCond;
                    updatedOperands = true;
                }

                // isoDate
                else if (isIsoDate(*i_))
                {
                    IsoDateCondition *dateCond = new IsoDateCondition(QString::fromStdString(*i_));
                    operandStack.push_back(dateCond);
                    result = dateCond;
                    updatedOperands = true;
                }

                //iso date operator
                else if (*i_ == "date::<=")
                {
                    auto *dateCond = new IsoDateLessThanEqualCondition();
                    funcStack.push_back(dateCond);
                    result = dateCond;
                }

                //iso date operator
                else if (*i_ == "date::>=")
                {
                    auto *dateCond = new IsoDateGreaterThanEqualCondition();
                    funcStack.push_back(dateCond);
                    result = dateCond;
                }

                else if (*i_ == "marked")
                {
                    auto *uiCond = new UIStateCondition(*i_);
                    operandStack.push_back(uiCond);
                    result = uiCond;
                    updatedOperands = true;
                }

                // logical operators
                else if (*i_ == "and")
                {
                    auto *andCond = new AndNodeCondition();
                    funcStack.push_back(andCond);
                    result = andCond;
                }

                else if (*i_ == "or")
                {
                    auto *orCond = new OrNodeCondition();
                    funcStack.push_back(orCond);
                    result = orCond;
                }

                else if (*i_ == "not")
                {
                    auto *notCond = new NotNodeCondition();
                    funcStack.push_back(notCond);
                    result = notCond;
                }

                else if(StringMatchMode::operToMode(*i_) != StringMatchMode::InvalidMatch)
                {
                    auto *stringMatchCond = new StringMatchCondition(StringMatchMode::operToMode(*i_), caseSensitiveStringMatch);
                    funcStack.push_back(stringMatchCond);
                    result = stringMatchCond;
                }

                else if (*i_ == "(")
                {
                    ++i_;
                    result = parseExpression(caseSensitiveStringMatch);
                    operandStack.push_back(result);
                }
                else if (*i_ == ")")
                {
                    returnEarly = true;
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
            // but do not do this if the last function asks to delay this process
            if (!funcStack.empty() && !funcStack.back()->delayUnwinding())
            {
                if(updatedOperands && (static_cast<int>(operandStack.size()) >= funcStack.back()->numOperands()))
                {
                    std::vector<BaseNodeCondition *> operands;
                    result = funcStack.back();       // last function is the current result
                    operands = popLastNOperands(operandStack, result->numOperands());  // pop its operands off the stack
                    result->setOperands(operands);
                    funcStack.pop_back();            // remove the last function from the stack
                    operandStack.push_back(result);  // store the current result
                }
            }
        }
        else
        {
            UiLog().err() << "Error parsing expression " << *i_;
            result = new FalseNodeCondition();
            return result;
        }

        if (i_ != tokens_.end() && !returnEarly)
            ++i_; // move onto the next token
    }


    int iterCnt=0; //to avoid infinite loop we use this counter

    // final unwinding of the stack
    while (!funcStack.empty())
    {
        if(static_cast<int>(operandStack.size()) >= funcStack.back()->numOperands())
        {
            std::vector<BaseNodeCondition *> operands;
            result = funcStack.back();  // last function is the current result           
            operands = popLastNOperands(operandStack, result->numOperands());  // pop its operands off the stack
            result->setOperands(operands);
            funcStack.pop_back(); // remove the last function from the stack
            operandStack.push_back(result);  // store the current result
            iterCnt=0;
        }
        else
        {
            iterCnt++;
            if(iterCnt > 10)
            {
                if(result)
                {
                    delete result;
                    result=nullptr;
                }
                break;
            }
        }
    }

    if(result)
        UiLog().dbg() << "    " <<  result->print();

    return result;
}


bool BaseNodeCondition::execute(VInfo_ptr nodeInfo)
{
    if(!nodeInfo)
		return true;

    if(nodeInfo->isServer())
        return execute(nodeInfo->server()->vRoot());
    else if(nodeInfo->isNode())
        return execute(nodeInfo->node());
    else if(nodeInfo->isAttribute())
        return execute(nodeInfo->attribute());

    return false;
}

// -----------------------------------------------------------------

bool BaseNodeCondition::containsAttributeSearch()
{
    bool contains = false;

    // check child condition nodes
    for(auto & operand : operands_)
    {
        contains = contains | operand->containsAttributeSearch();
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

bool AndNodeCondition::execute(VItem* node)
{
	return operands_[0]->execute(node) && operands_[1]->execute(node);
}

//=========================================================================
//
//  OrNodeCondition
//
//=========================================================================

bool OrNodeCondition::execute(VItem* node)
{
#ifdef _UI_NODEXPRESSIONPARSEER_DEBUG
    UiLog().dbg() << "OrNodeCondition::execute --->";
    UiLog().dbg() <<  operands_[0]->execute(node) << " "  << operands_[1]->execute(node);
#endif
    return operands_[0]->execute(node) || operands_[1]->execute(node);
}


//=========================================================================
//
//  NotNodeCondition
//
//=========================================================================

bool NotNodeCondition::execute(VItem* node)
{
	return !(operands_[0]->execute(node));
}

//=========================================================================
//
//  TypeNodeCondition
//
//=========================================================================

bool TypeNodeCondition::execute(VItem* item)
{
    if (type_ == NodeExpressionParser::SERVER)
    {
        return (item->isServer() != nullptr);
    }

    else if(item->isNode())
    {
#ifdef _UI_NODEXPRESSIONPARSEER_DEBUG
        UiLog().dbg() << "TypeNodeCondition::execute --> " << NodeExpressionParser::instance()->typeName(type_);
        UiLog().dbg() << item->isNode() << " " << item->isSuite() << " " << item->isFamily() <<
                         " " << item->isTask() << " " << item->isAlias();
#endif
        switch(type_)
        {
        case NodeExpressionParser::NODE:
#ifdef _UI_NODEXPRESSIONPARSEER_DEBUG
            UiLog().dbg() << "   NODE";
#endif
            return true;
            break;
        case NodeExpressionParser::SUITE:
#ifdef _UI_NODEXPRESSIONPARSEER_DEBUG
            UiLog().dbg() << "   SUITE";
#endif
            return (item->isSuite() != nullptr);
            break;
        case NodeExpressionParser::TASK:
#ifdef _UI_NODEXPRESSIONPARSEER_DEBUG
            UiLog().dbg() << "   TASK";
#endif
            return (item->isTask() != nullptr);
            break;
        case NodeExpressionParser::FAMILY:
#ifdef _UI_NODEXPRESSIONPARSEER_DEBUG
            UiLog().dbg() << "   FAMILY";
#endif
            return (item->isFamily() != nullptr);
            break;
        case NodeExpressionParser::ALIAS:
#ifdef _UI_NODEXPRESSIONPARSEER_DEBUG
            UiLog().dbg() << "   ALIAS";
#endif
            return (item->isAlias() != nullptr);
            break;
        default:
            break;
        }
     }

    return false;
}

//=========================================================================
//
//  StateNodeCondition
//
//=========================================================================

bool StateNodeCondition::execute(VItem* item)
{
    if(item->isServer())
    {
        auto* s=static_cast<VServer*>(item);
        assert(s);
        return (s->serverStateName() == stateName_);
    }
    else
    {
        auto* n=static_cast<VNode*>(item);
        assert(n);
        return (n->stateName() == stateName_);
    }
    return false;
}

//=========================================================================
//
// NodeMenuModeCondition
//
//=========================================================================

bool NodeMenuModeCondition::execute(VItem* item)
{
    if(item)
    {
        if(menuModeName_ == "defStatusMenuModeControl")
        {
            Q_FOREACH(QString s,item->defStatusNodeMenuMode().split("/"))
            {
                if(item->nodeMenuMode() == s)
                    return true;
            }

        }
        else
        {
            return (item->nodeMenuMode() == menuModeName_);
        }
    }
    return false;
}

//=========================================================================
//
// NodeMenuModeCondition
//
//=========================================================================

bool EnvVarCondition::execute(VItem* item)
{
    if(item)
    {
        if(defined_ == -1)
        {
            defined_=0;
            if(const char* ch=getenv(envVarName_.toStdString().c_str()))
            {
                defined_=(strcmp(ch,"1") == 0)?1:0;
            }
        }

        return defined_ == 1;
    }
    return false;
}

//=========================================================================
//
//  UIStateCondition
//
//=========================================================================

bool UIStateCondition::execute(VItem*)
{
    if (uiStateName_ == "marked")
    {
        return VNodeMover::hasMarkedForMove();
    }

    return false;
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
    Qt::CaseSensitivity cs = (caseSensitive_) ? Qt::CaseSensitive : Qt::CaseInsensitive;
    QRegExp regexp(QString::fromStdString(searchFor), cs);
    int index = regexp.indexIn(QString::fromStdString(searchIn));
    return (index != -1);  // -1 means no match
}

bool StringMatchWildcard::match(std::string searchFor, std::string searchIn)
{
    Qt::CaseSensitivity cs = (caseSensitive_) ? Qt::CaseSensitive : Qt::CaseInsensitive;
    QRegExp regexp(QString::fromStdString(searchFor), cs);
    regexp.setPatternSyntax(QRegExp::Wildcard);
    return regexp.exactMatch(QString::fromStdString(searchIn));
}

bool StringMatchRegexp::match(std::string searchFor, std::string searchIn)
{
    Qt::CaseSensitivity cs = (caseSensitive_) ? Qt::CaseSensitive : Qt::CaseInsensitive;
    QRegExp regexp(QString::fromStdString(searchFor), cs);
    return regexp.exactMatch(QString::fromStdString(searchIn));
}

//=========================================================================
//
//  String match condition
//
//=========================================================================

StringMatchCondition::StringMatchCondition(StringMatchMode::Mode matchMode, bool caseSensitive)
{
    switch (matchMode)
    {
        case StringMatchMode::ContainsMatch:
            matcher_ = new StringMatchContains(caseSensitive);
            break;
        case StringMatchMode::WildcardMatch:
            matcher_ = new StringMatchWildcard(caseSensitive);
            break;
        case StringMatchMode::RegexpMatch:
            matcher_ = new StringMatchRegexp(caseSensitive);
            break;
        default:
            UiLog().dbg() << "StringMatchCondition: bad matchMode";
            matcher_ = new StringMatchExact(caseSensitive);
            break;
    }
}

bool StringMatchCondition::execute(VItem *item)
{
    auto *searchForOperand = static_cast<WhatToSearchForOperand*> (operands_[0]);
    auto  *searchInOperand  = static_cast<WhatToSearchInOperand*>  (operands_[1]);

    std::string searchIn = searchInOperand->what();

    if(item->isNode())
    {
        auto* n=static_cast<VNode*>(item);
        //TODO  XXXX check - name, label, variable, etc
        if(searchIn == "node_name")
        {
            return matcher_->match(searchForOperand->what(), n->strName());
        }

        else if (searchIn == "node_path")
        {
            return matcher_->match(searchForOperand->what(), n->absNodePath());
        }
    }
    else if(VAttribute* a=item->isAttribute())
    {
        std::string str;
        if(a->value(searchIn,str))
            return matcher_->match(searchForOperand->what(),str);
        else
            return false;
    }

    return false;
}

// -----------------------------------------------------------------

bool NodeAttributeCondition::execute(VItem* item)
{
    if (item->isServer())
    {
        if(nodeAttrName_ == "locked")
        {
            return false;   //  XXX temporary for now
        }
    }

    else if(item->isNode())
    {
         auto* n=static_cast<VNode*>(item);
         node_ptr node = n->node();

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
}
// -----------------------------------------------------------------

bool NodeFlagCondition::execute(VItem* item)
{
    if(item->isServer())
	{
		return false;
	}
    else if(item->isNode())
	{
         auto* vnode=static_cast<VNode*>(item);

        if(nodeFlagName_ == "is_zombie")
			return vnode->isFlagSet(ecf::Flag::ZOMBIE);

		if(nodeFlagName_ == "has_message")
			return vnode->isFlagSet(ecf::Flag::MESSAGE);

		else if(nodeFlagName_ == "is_late")
			return vnode->isFlagSet(ecf::Flag::LATE);

		else if(nodeFlagName_ == "is_rerun")
		{
			node_ptr node=vnode->node();
			if(!node.get()) return false;

			if(Submittable* s = node->isSubmittable())
			{
				return (s->try_no() > 1);
			}
			return false;
		}
		else if(nodeFlagName_ == "is_waiting")
            return vnode->isFlagSet(ecf::Flag::WAIT);

        else if(nodeFlagName_ == "is_migrated")
            return vnode->isFlagSet(ecf::Flag::ARCHIVED);

        else if(nodeFlagName_ == "is_ecfcmd_failed")
            return vnode->isFlagSet(ecf::Flag::JOBCMD_FAILED);

        else if(nodeFlagName_ == "is_killed")
            return vnode->isFlagSet(ecf::Flag::KILLED);


	}

	return false;
}

WhatToSearchInOperand::WhatToSearchInOperand(std::string what, bool &attr)
{
    what_ = what;
    searchInAttributes_ = attr;
}


WhatToSearchInOperand::~WhatToSearchInOperand() = default;
WhatToSearchForOperand::~WhatToSearchForOperand() = default;

//====================================================
//
// Attribute condition
//
//====================================================

bool AttributeCondition::execute(VItem* item)
{   
    if(!item)
        return false;

    VAttribute* a=item->isAttribute();
    if(!a)
        return false;

    Q_ASSERT(a->type());

    return a->type() == type_;
}

//====================================================
//
// Attribute state condition
//
//====================================================

bool AttributeStateCondition::execute(VItem* item)
{
    if(!item)
        return false;

    VAttribute* a=item->isAttribute();
    if(!a)
        return false;

    assert(a->type());

    if(attrState_.startsWith("event_"))
    {
        if(a->type()->name() == "event" && a->data().count() >= 3)
        {
            QString v=a->data()[2];
            if(attrState_ == "event_set")
                return v == "1";
            else if(attrState_ == "event_clear")
                return v == "0";
         }
    }   
    else if(attrState_.startsWith("repeat_"))
    {
        if(a->type()->name() == "repeat" && a->data().count() >= 2)
        {
            QString v=a->data()[1];
            if(attrState_ == "repeat_date")
                return v == "date";
            else if(attrState_ == "repeat_int")
                return v == "integer";
            else if(attrState_ == "repeat_string")
                return v == "string";
            else if(attrState_ == "repeat_enum")
                return v == "enumeration";
            else if(attrState_ == "repeat_day")
                return v == "day";
         }
    }
    return false;
}

//==========================================
//
// ISO date condition
//
//==========================================

IsoDateCondition::IsoDateCondition(QString dateStr)
{
    QDateTime d=QDateTime::fromString(dateStr,Qt::ISODate);
    if(d.isValid())
        secsSinceEpoch_=d.toMSecsSinceEpoch()/1000;
}

std::string IsoDateCondition::print()
{
    if(secsSinceEpoch_ > 0)
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
        return QDateTime::fromMSecsSinceEpoch(secsSinceEpoch_*1000,Qt::UTC).toString(Qt::ISODate).toStdString();
#else
        return QDateTime::fromMSecsSinceEpoch(secsSinceEpoch_*1000).toUTC().toString(Qt::ISODate).toStdString();
#endif
    return std::string();
}

//==========================================
//
// Node status change date condition
//
//==========================================

qint64 NodeStatusChangeDateCondition::secsSinceEpoch(VItem* item) const
{
    Q_ASSERT(item);
    if(item->isNode())
    {
         auto* vnode=static_cast<VNode*>(item);
         Q_ASSERT(vnode);
         return vnode->statusChangeTime();
    }

    return -1;
}

std::string NodeStatusChangeDateCondition::print()
{
    return "status_change_time";
}

//==========================================
//
// ISO date greater than equal condition
//
//==========================================

bool IsoDateGreaterThanEqualCondition::execute(VItem *node)
{
    UI_ASSERT(operands_.size() == 2,"operands size=" <<operands_.size() );
    Q_ASSERT(operands_[0]);
    Q_ASSERT(operands_[1]);
    //The operand order is swapped in popLastNOperands(). So 1 is right, 0 is left here.
    auto* leftOperand=static_cast<IsoDateCondition*> (operands_[1]);
    auto* rightOperand=static_cast<IsoDateCondition*> (operands_[0]);
    Q_ASSERT(leftOperand);
    Q_ASSERT(rightOperand);

    return leftOperand->secsSinceEpoch(node) >= rightOperand->secsSinceEpoch(node);
}

std::string IsoDateGreaterThanEqualCondition::print()
{
    UI_ASSERT(operands_.size() == 2,"operands size=" <<operands_.size() );
    Q_ASSERT(operands_[0]);
    Q_ASSERT(operands_[1]);
    //The operand order is swapped in popLastNOperands(). So 1 is right, 0 is left here.
    return operands_[1]->print() + " >= " + operands_[0]->print();
}

//==========================================
//
// ISO date less than equal condition
//
//==========================================

bool IsoDateLessThanEqualCondition::execute(VItem *node)
{
    UI_ASSERT(operands_.size() == 2,"operands size=" <<operands_.size() );
    Q_ASSERT(operands_[0]);
    Q_ASSERT(operands_[1]);
    //The operand order is swapped in popLastNOperands(). So 1 is right, 0 is left here.
    auto* leftOperand=static_cast<IsoDateCondition*> (operands_[1]);
    auto* rightOperand=static_cast<IsoDateCondition*> (operands_[0]);
    Q_ASSERT(leftOperand);
    Q_ASSERT(rightOperand);

    return leftOperand->secsSinceEpoch(node) <= rightOperand->secsSinceEpoch(node);
}

std::string IsoDateLessThanEqualCondition::print()
{
    UI_ASSERT(operands_.size() == 2,"operands size=" <<operands_.size() );
    Q_ASSERT(operands_[0]);
    Q_ASSERT(operands_[1]);
    //The operand order is swapped in popLastNOperands(). So 1 is right, 0 is left here.
    return operands_[1]->print() + " <= " + operands_[0]->print();
}
