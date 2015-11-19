//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "NodeQuery.hpp"

#include "VSettings.hpp"

NodeQueryStringOption::MatchMode NodeQueryStringOption::defaultMatchMode_=NodeQueryStringOption::WildcardMatch;
bool NodeQueryStringOption::defaultCaseSensitive_=false;
QMap<NodeQueryStringOption::MatchMode,QString> NodeQueryStringOption::matchOper_;

QStringList NodeQuery::nodeTerms_;
QStringList NodeQuery::typeTerms_;
QStringList NodeQuery::stateTerms_;
QStringList NodeQuery::flagTerms_;
QStringList NodeQuery::attrGroupTerms_;
QMap<QString,QStringList> NodeQuery::attrTerms_;
int NodeQuery::defaultMaxNum_=50000;

NodeQueryStringOption::NodeQueryStringOption(QString name) :
  name_(name),
  matchMode_(defaultMatchMode_),
  caseSensitive_(defaultCaseSensitive_)
{
	if(matchOper_.isEmpty())
	{
		matchOper_[ContainsMatch]="~";
		matchOper_[WildcardMatch]="=";
		matchOper_[RegexpMatch]="=~";
	}
}

void NodeQueryStringOption::swap(const NodeQueryStringOption* op)
{
	value_=op->value();
	matchMode_=op->matchMode();
	caseSensitive_=op->caseSensitive();
}


QString NodeQueryStringOption::matchOperator() const
{
	return matchOper_.value(matchMode_);
}

void NodeQueryStringOption::save(VSettings* vs)
{
    if(value_.isEmpty() && matchMode_ == defaultMatchMode_ &&
       caseSensitive_ == defaultCaseSensitive_)
        return;
        
    vs->beginGroup(name_.toStdString());
    vs->put("value",value_.toStdString());
    vs->put("matchMode",matchModeAsInt());
    vs->putAsBool("caseSensistive",caseSensitive_);
    vs->endGroup();   
}


void NodeQueryStringOption::load(VSettings* vs)
{
    if(!vs->contains(name_.toStdString()))
        return;
    
    vs->beginGroup(name_.toStdString());
    value_=QString::fromStdString(vs->get("value",value_.toStdString())); 
    matchMode_=static_cast<MatchMode>(vs->get<int>("matchMode",matchModeAsInt()));
    caseSensitive_=vs->getAsBool("caseSensistive",caseSensitive_);
    vs->endGroup();   
}

void NodeQuerySelectOption::swap(const NodeQuerySelectOption* op)
{
	selection_=op->selection();
}

void NodeQuerySelectOption::save(VSettings* vs)
{
    if(selection_.isEmpty())
        return;

    std::vector<std::string> v;
    Q_FOREACH(QString s, selection_)
        v.push_back(s.toStdString());
        
    vs->put(name_.toStdString(),v);
}

void NodeQuerySelectOption::load(VSettings* vs)
{
    if(!vs->contains(name_.toStdString()))
        return;

    std::vector<std::string> v;
    vs->get(name_.toStdString(),v);
    
    selection_.clear();
    for(std::vector<std::string>::const_iterator it=v.begin(); it != v.end(); ++it)
        selection_ << QString::fromStdString(*it);
}


NodeQuery::NodeQuery(const std::string& name) :
  name_(name),
  advanced_(false),
  caseSensitive_(false),
  maxNum_(defaultMaxNum_)
{
	if(nodeTerms_.isEmpty())
	{
		nodeTerms_ << "node_name" << "node_path";
		typeTerms_ << "server" << "suite" << "family" << "task" << "alias";
		stateTerms_ << "aborted" << "active" << "complete" << "queued" << "submitted" << "suspended" << "unknown";
		flagTerms_ << "is_late" << "has_date" << "has_message" << "has_time" << "is_rerun" << "is_waiting" << "is_zombie";
		attrGroupTerms_ << "date" << "event" << "label" << "late" << "limit" << "limiter" << "meter"
			    	   << "repeat" << "time" << "trigger" << "variable";

		attrTerms_["date"] << "date_name";
		attrTerms_["event"] << "event_name";
		attrTerms_["label"] << "label_name" << "label_value";
		attrTerms_["limit"] << "limit_name" << "limit_value" << "limit_max";
		attrTerms_["limiter"] << "limiter_name";
		attrTerms_["meter"] << "meter_name";
		attrTerms_["repeat"] << "repeat_name" << "repeat_value";
		attrTerms_["time"] << "time_name";
		attrTerms_["trigger"] << "trigger_expression";
		attrTerms_["variable"] << "var_name" << "var_value";

	}

	Q_FOREACH(QString s,nodeTerms_)
	{
		stringOptions_[s]=new NodeQueryStringOption(s);
	}

	Q_FOREACH(QString gr,attrGroupTerms_)
	{
		Q_FOREACH(QString s,attrTerms_[gr])
			stringOptions_[s]=new NodeQueryStringOption(s);
	}

	selectOptions_["type"]=new NodeQuerySelectOption("type");
	selectOptions_["state"]=new NodeQuerySelectOption("state");
	selectOptions_["flag"]=new NodeQuerySelectOption("flag");
	selectOptions_["attr"]=new NodeQuerySelectOption("flag");
}

NodeQuery::~NodeQuery()
{
    Q_FOREACH(QString s,stringOptions_.keys())
    {
        delete stringOptions_[s];
    }

    Q_FOREACH(QString s,selectOptions_.keys())
    {
        delete selectOptions_[s];
    }
}


NodeQuery* NodeQuery::clone()
{
	return clone(name_);
}

NodeQuery* NodeQuery::clone(const std::string& name)
{
	NodeQuery *q=new NodeQuery(name);
	q->swap(this);

	return q;
}

void NodeQuery::swap(const NodeQuery* q)
{
	advanced_=q->advanced_;
	query_=q->query_;
	extQuery_=q->extQuery_;
	rootNode_=q->rootNode_;
	servers_=q->servers_;
	caseSensitive_=q->caseSensitive_;
	maxNum_=q->maxNum_;

	Q_FOREACH(QString s,stringOptions_.keys())
	{
		stringOptions_[s]->swap(q->stringOptions_.value(s));
	}

	Q_FOREACH(QString s,selectOptions_.keys())
	{
		selectOptions_[s]->swap(q->selectOptions_.value(s));
	}
}

void  NodeQuery::setName(const std::string& name)
{
	name_=name;
}

void NodeQuery::setQuery(QString query)
{
	query_=query;
}

bool NodeQuery::hasServer(const std::string& name) const
{
	if(servers_.empty())
		return true;

	return servers_.contains(QString::fromStdString(name));
}

QStringList NodeQuery::typeSelection() const
{
	return selectOptions_.value("type")->selection_;
}

QStringList NodeQuery::stateSelection() const
{
	return selectOptions_.value("state")->selection_;
}

QStringList NodeQuery::flagSelection() const
{
	return selectOptions_["flag"]->selection_;
}

NodeQueryStringOption*  NodeQuery::stringOption(QString name) const
{
	QMap<QString,NodeQueryStringOption*>::const_iterator it = stringOptions_.find(name);
	if(it != stringOptions_.constEnd())
		return it.value();
	return NULL;
}

QStringList NodeQuery::attrGroupSelection() const
{
	return selectOptions_["attr"]->selection_;
}

void NodeQuery::setTypeSelection(QStringList lst)
{
	selectOptions_["type"]->selection_=lst;
}

void NodeQuery::setStateSelection(QStringList lst)
{
	selectOptions_["state"]->selection_=lst;
}

void NodeQuery::setFlagSelection(QStringList lst)
{
	selectOptions_["flag"]->selection_=lst;
}

void NodeQuery::setAttrGroupSelection(QStringList lst)
{
	selectOptions_["attr"]->selection_=lst;
}

QString NodeQuery::queryString(bool update)
{
	if(update)
		buildQueryString();
	return query_;
}

void NodeQuery::buildQueryString()
{
	//Scope
	QString scopePart;
	QString name=stringOptions_["node_name"]->value().simplified();
	QString path=stringOptions_["node_path"]->value().simplified();
	if(!name.isEmpty())
	{
		scopePart="node_name " + stringOptions_["node_name"]->matchOperator() + " \'" +  name + "\'";
	}
	if(!path.isEmpty())
	{
		if(!scopePart.isEmpty())
			scopePart+=" or ";

		scopePart+="node_path " + stringOptions_["node_path"]->matchOperator() + " \'" +  path + "\'";
	}
	if(!scopePart.isEmpty())
	{
		scopePart="( " + scopePart + " )";
	}

	//Type
	QString typePart;
	if(typeSelection().count() >0)
	{
		typePart="( " + typeSelection().join(" or ") + " )";
	}

	//State
	QString statePart;
	if(stateSelection().count() >0)
	{
		statePart="( " + stateSelection().join(" or ") + " )";
	}

	//Flage
	QString flagPart;
	if(flagSelection().count() >0)
	{
		flagPart="( " + flagSelection().join(" or ") + " )";
	}

	//Attributes
	QString attrPart;
	Q_FOREACH(QString gr,attrGroupSelection())
	{
		QString grPart;
		Q_FOREACH(QString opName,attrTerms_[gr])
		{
			NodeQueryStringOption* op=stringOption(opName);
			assert(op);
			QString s=op->value();
			if(!s.isEmpty())
			{
				if(!grPart.isEmpty())
					grPart+=" or ";
				grPart+=opName + " " + op->matchOperator() + " \'" + s + "\'";
			}
		}

		if(grPart.isEmpty())
			grPart=gr;


		if(!attrPart.isEmpty())
			attrPart+=" or ";
		attrPart+=grPart;
	}

	if(!attrPart.isEmpty())
	{
		attrPart="( " + attrPart + " )";
	}

	//Put everything together
	query_.clear();

	if(!scopePart.isEmpty())
		query_+=scopePart;

	if(!typePart.isEmpty())
	{
		if(!query_.isEmpty())
			query_+=" and ";
		query_+=typePart;
	}

	if(!statePart.isEmpty())
	{
		if(!query_.isEmpty())
			query_+=" and ";
		query_+=statePart;
	}

	if(!flagPart.isEmpty())
	{
		if(!query_.isEmpty())
			query_+=" and ";
		query_+=flagPart;
	}

	if(!attrPart.isEmpty())
	{
		if(!query_.isEmpty())
			query_+=" and ";
		query_+=attrPart;
	}

	//Extended query
	extQuery_.clear();

	if(!servers_.isEmpty())
		extQuery_="servers = " + servers_.join(", ") + "; ";

	if(!rootNode_.empty())
	{
		//if(!extQuery_.isEmpty())
		//	extQuery_+="\n";
		extQuery_+="root_node = " + QString::fromStdString(rootNode_) + "; ";
	}
	extQuery_+=query_;
}


void NodeQuery::load(VSettings* vs)
{
	advanced_=vs->getAsBool("advanced",advanced_);
	caseSensitive_=vs->getAsBool("case",caseSensitive_);

	int maxNum=maxNum_;
	maxNum=vs->get<int>("maxNum",maxNum_);
	if(maxNum_ > 1 && maxNum < 5000000)
		maxNum_=maxNum;

	std::vector<std::string> v;
	vs->get("servers",v);
	servers_.clear();
	for(std::vector<std::string>::const_iterator it=v.begin(); it != v.end(); ++it)
	    servers_ << QString::fromStdString(*it);

	rootNode_=vs->get("rootNode",rootNode_);
    
    Q_FOREACH(QString s,stringOptions_.keys())
    {
        stringOptions_[s]->load(vs);
    }

    Q_FOREACH(QString s,selectOptions_.keys())
    {
        selectOptions_[s]->load(vs);
    }

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

    if(!v.empty())
    	vs->put("servers",v);

    if(!rootNode_.empty())
		vs->put("rootNode",rootNode_);
    
	Q_FOREACH(QString s,stringOptions_.keys())
    {
        stringOptions_[s]->save(vs);
    }

    Q_FOREACH(QString s,selectOptions_.keys())
    {
        selectOptions_[s]->save(vs);
    }
        
    //vs->put("query",query_.toStdString());
}



