//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "NodeQuery.hpp"

#include "NodeQueryOption.hpp"
#include "UiLog.hpp"
#include "VAttributeType.hpp"
#include "VSettings.hpp"

int NodeQuery::defaultMaxNum_=50000;

#define _UI_NODEQUERY_DEBUG
 
QString NodeQueryAttrGroup::query() const
{
    QStringList lst;
    Q_FOREACH(NodeQueryOption* op,options_)
    {
        QString s=op->query().simplified();
        if(!s.isEmpty())
            lst << op->query();
    }

    if(lst.count() == 0)
        return name_;
    else if(lst.count() == 1)
    {
        return lst[0];
    }
    else
    {
        return "(" + lst.join(" and ") + ")";
    }

    return QString();
}

bool NodeQueryVarAttrGroup::hasType(VAttributeType* t) const 
{
    if(types_.contains(t))
    {
        Q_FOREACH(NodeQueryOption* op,options_)
        {
            if(op->name() == "var_type")
            {
                QString v=op->valueAsString();
#ifdef _UI_NODEQUERY_DEBUG
                UiLog().dbg() << "NodeQueryVarAttrGroup::hasType  var_type=" << v.toStdString();
#endif
                if(v == "any")
                    return true;
                else
                    return (v == t->name());
            }
        }
        
        Q_ASSERT(false);
    }
    
    return false;
}

NodeQuery::NodeQuery(const std::string& name,bool ignoreMaxNum) :
  name_(name),
  advanced_(false),
  caseSensitive_(false),
  maxNum_(defaultMaxNum_),
  ignoreMaxNum_(ignoreMaxNum)
{   
#ifdef _UI_NODEQUERY_DEBUG
    UI_FUNCTION_LOG
#endif

    //Build options from defs. Options store the actual query settings. They are
    //editable through the gui.
    NodeQueryOption::build(this);
}

NodeQuery::~NodeQuery()
{
    qDeleteAll(options_);
}

NodeQuery* NodeQuery::clone()
{
	return clone(name_);
}

NodeQuery* NodeQuery::clone(const std::string& name)
{
	auto *q=new NodeQuery(name);
	q->swap(this);

	return q;
}

void NodeQuery::swap(const NodeQuery* q)
{
	advanced_=q->advanced_;
	//query_=q->query_;
	extQuery_=q->extQuery_;
	rootNode_=q->rootNode_;
	servers_=q->servers_;
	//allServers_=q->allServers_;
	caseSensitive_=q->caseSensitive_;
	maxNum_=q->maxNum_;
	ignoreMaxNum_=q->ignoreMaxNum_;

    for(QMap<QString,NodeQueryOption*>::const_iterator it = options_.constBegin(); it != options_.constEnd(); ++it)
        it.value()->swap(q->option(it.key()));

	buildQueryString();
}

void  NodeQuery::setName(const std::string& name)
{
	name_=name;
}

bool NodeQuery::hasServer(const std::string& name) const
{
#ifdef _UI_NODEQUERY_DEBUG
    UiLog().dbg() << "NodeQuery::hasServer -->";
#endif

	if(servers_.empty())
		return true;

	return servers_.contains(QString::fromStdString(name));
}

NodeQueryOption* NodeQuery::option(QString name) const
{
    QMap<QString,NodeQueryOption*>::const_iterator it = options_.find(name);
    if(it != options_.constEnd())
        return it.value();
    return nullptr;
}

QString NodeQuery::query() const
{
    QString s1=nodeQueryPart();   
    QString s2=attrQueryPart();
    if(!s1.isEmpty())       
        if(!s2.isEmpty())
            return s1+ " and " + s2;
        else
            return s1;
    else
        return s2;
           
    return QString();
}



QString NodeQuery::nodeQueryPart() const
{   
    QStringList lst;
    QStringList keys;
    keys << "node" << "type" << "state" << "flag" << "status_change_time";
    Q_FOREACH(QString s,keys)
    {
        if(!extQuery_.value(s).isEmpty())
            lst << extQuery_.value(s);
    }
    
    //TODO : handle all   
    return lst.join(" and ");
}    

bool NodeQuery::hasBasicNodeQueryPart() const
{
    QStringList keys;
    keys << "node" << "type" << "state" << "flag";
    Q_FOREACH(QString s,keys)
    {
        if(!extQuery_.value(s).isEmpty())
            return true;
    }
    return false;
}

bool NodeQuery::hasPeriodQueryPart() const
{
    return !extQuery_.value("status_change_time").isEmpty();
}

QString NodeQuery::attrQueryPart() const
{
    return extQuery_["attr"];
}

QString NodeQuery::attrQueryPart(VAttributeType* t) const
{        
    //TODO
    QStringList attrSel=attrSelection();
    QString q;
    Q_FOREACH(NodeQueryAttrGroup* tm,attrGroup_.values())
    {
        if(tm->hasType(t) &&
           attrSel.contains(tm->name()))
        {
            QStringList qLst;

            Q_FOREACH(NodeQueryOption* op,tm->options())
            {               
                Q_ASSERT(op);
                QString s=op->query();
                if(!s.isEmpty())
                {
                    qLst << s;
                }
            }

            if(qLst.isEmpty())
               q=t->name();
            else
               q=qLst.join(" and ");

            break;
         }
    }
    return q;
}

bool NodeQuery::hasAttribute(VAttributeType *t) const
{
    Q_FOREACH(NodeQueryAttrGroup* d,attrGroup_.values())
    {
        if(d->hasType(t))
        {
            return attrSelection().contains(d->name());
        }
    }
    return false;
}

QStringList NodeQuery::attrSelection() const
{
    NodeQueryOption *a=options_["attribute"];
    Q_ASSERT(a);
    auto* op=static_cast<NodeQueryListOption*>(a);
    Q_ASSERT(op);
    return op->selection();
}

NodeQueryListOption* NodeQuery::stateOption() const
{
    NodeQueryOption* op=option("state");
    Q_ASSERT(op);
    return op->isList();
}

void NodeQuery::buildQueryString()
{
#ifdef _UI_NODEQUERY_DEBUG
    UI_FUNCTION_LOG
#endif

    extQuery_.clear();

    //Node
    QStringList nodePart;
    QString name=options_["node_name"]->query();
    QString path=options_["node_path"]->query();
    if(!name.isEmpty()) nodePart << name;
    if(!path.isEmpty()) nodePart << path;
    if(nodePart.count() > 0)
        extQuery_["node"]="(" +nodePart.join(" and ") + ")";

	//Type
    QString typePart=options_["type"]->query(" or ");
    if(!typePart.isEmpty())
        extQuery_["type"]="(" + typePart + ")";

    //State
    QString statePart=options_["state"]->query(" or ");
    if(!statePart.isEmpty())
        extQuery_["state"]="(" + statePart + ")";

	//Flag
    QString flagPart=options_["flag"]->query(" or ");
    if(!flagPart.isEmpty())
        extQuery_["flag"]="(" + flagPart + ")";;

    //Status change time
    QString periodPart=options_["status_change_time"]->query();
    if(!periodPart.isEmpty())
        extQuery_["status_change_time"]="(" + periodPart + ")";

	//Attributes
	QString attrPart;

    Q_FOREACH(QString attrName,attrSelection())
	{
        NodeQueryAttrGroup* grp=attrGroup_.value(attrName);
        Q_ASSERT(grp);
        QString grpPart=grp->query();

		if(!attrPart.isEmpty())
			attrPart+=" or ";
		
        attrPart+=grpPart;
	}

	if(!attrPart.isEmpty())
        //extQuery_["attr"]="(" + attrPart + ")";
        extQuery_["attr"]=attrPart;

    bool hasEq=QStringList(extQuery_.values()).join("").contains("=");

    //Scope
	QString scopePart;
	if(!servers_.isEmpty())
		scopePart="servers = \'" + servers_.join(", ") + "\'";

	if(servers_.size() <= 1 && !rootNode_.empty())
	{
		if(!scopePart.isEmpty())
			scopePart+=" and ";

		scopePart+="root_node = \'" + QString::fromStdString(rootNode_) + "\'";
	}
	if(!scopePart.isEmpty())
		extQuery_["scope"]=scopePart;

    //Other options
	QString opPart;
	if(!ignoreMaxNum_)
	{
		opPart="max_results = " + QString::number(maxNum_);
	}

	if(hasEq)
	{
		if(!opPart.isEmpty())
			opPart+=" and ";
		if(caseSensitive_)
			opPart+="case_sensitive";
		else
			opPart+="case_insensitive";
	}

	if(!opPart.isEmpty())
		extQuery_["options"]=opPart;

    //SQL-like query
    sqlQuery_.clear();
    QStringList selectPart;
    QStringList fromPart;
    QStringList wherePart;
    sqlQuery_="SELECT";
    QStringList nodeParts;
    nodeParts << "node" << "type" << "state" << "flag";
    Q_FOREACH(QString s,nodeParts)
    {
        QString vs=extQuery_.value(s);
        if(!vs.isEmpty())
        {
            vs.replace("<","&lt;");
            vs.replace(">","&gt;");
            wherePart << vs;
        }
    }

    QString sqlPeriodPart=options_["status_change_time"]->sqlQuery();
    if(!sqlPeriodPart.isEmpty())
    {
        sqlPeriodPart="(" + sqlPeriodPart + ")";
        sqlPeriodPart.replace("<","&lt;");
        sqlPeriodPart.replace(">","&gt;");
        wherePart << sqlPeriodPart;
    }

    selectPart << "node";

    //Attribute
    Q_FOREACH(QString attrName,attrSelection())
    {
        NodeQueryAttrGroup* grp=attrGroup_.value(attrName);
        Q_ASSERT(grp);
        selectPart << grp->name();
        selectPart.removeOne("node");
        QString grpPart=grp->query();
        if(grpPart != grp->name())
            wherePart << grpPart;
    }

    sqlQuery_+=" " + selectPart.join(", ");

    //FROM
    if(!servers_.isEmpty())
    {
        if(servers_.size() ==1 && !rootNode_.empty())
        {
            fromPart << servers_[0] + ":/" + QString::fromStdString(rootNode_);
        }
        else
            fromPart=servers_;

        sqlQuery_+=" FROM " + fromPart.join(", ");
    }
    else
    {
        //sqlQuery_+=" FROM *";
    }

    if(wherePart.count() > 0)
        sqlQuery_+=" WHERE " + wherePart.join(" and ");

    if(!ignoreMaxNum_)
    {
        sqlQuery_+=" LIMIT " + QString::number(maxNum_);
    }
}

void NodeQuery::load(VSettings* vs)
{
	advanced_=vs->getAsBool("advanced",advanced_);
	caseSensitive_=vs->getAsBool("case",caseSensitive_);

	auto maxNum=vs->get<int>("maxNum",maxNum_);
	if(maxNum_ > 1 && maxNum < 5000000)
		maxNum_=maxNum;

	std::vector<std::string> v;
	vs->get("servers",v);
	servers_.clear();
	for(std::vector<std::string>::const_iterator it=v.begin(); it != v.end(); ++it)
	    servers_ << QString::fromStdString(*it);

	//allServers_=vs->getAsBool("allServers",allServers_);

	rootNode_=vs->get("rootNode",rootNode_);
    
    Q_FOREACH(QString s,options_.keys())
    {
        options_[s]->load(vs);
    }
#if 0
    Q_FOREACH(QString s,selectOptions_.keys())
    {
        selectOptions_[s]->load(vs);
    }
#endif
    buildQueryString();

	//query_=QString::fromStdString(vs->get("query",query_.toStdString()));
}


void NodeQuery::save(VSettings* vs)
{
    if(advanced_)
    	vs->putAsBool("advanced",advanced_);

    if(caseSensitive_)
    	vs->putAsBool("case",caseSensitive_);

    if(maxNum_ != defaultMaxNum_)
    	vs->put("maxNum",maxNum_);

    std::vector<std::string> v;
    Q_FOREACH(QString s, servers_)
    	v.push_back(s.toStdString());

    //if(!allServers_)
   //     vs->putAsBool("allServers",allServers_);

    if(!v.empty())
    	vs->put("servers",v);

    if(!rootNode_.empty())
		vs->put("rootNode",rootNode_);
    
    Q_FOREACH(QString s,options_.keys())
    {
        options_[s]->save(vs);
    }
#if 0
    Q_FOREACH(QString s,selectOptions_.keys())
    {
        selectOptions_[s]->save(vs);
    }
 #endif
    //vs->put("query",query_.toStdString());
}
