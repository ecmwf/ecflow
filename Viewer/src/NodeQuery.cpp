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

#include <QDebug>

#include "UserMessage.hpp"
#include "VSettings.hpp"

StringMatchMode::Mode NodeQueryStringOption::defaultMatchMode_=StringMatchMode::WildcardMatch;
bool NodeQueryStringOption::defaultCaseSensitive_=false;
//QMap<NodeQueryStringOption::MatchMode,QString> NodeQueryStringOption::matchOper_;

QStringList NodeQuery::nodeTerms_;
QStringList NodeQuery::typeTerms_;
QStringList NodeQuery::stateTerms_;
QStringList NodeQuery::flagTerms_;
QStringList NodeQuery::attrGroupTerms_;
QMap<QString,QStringList> NodeQuery::attrTerms_;
int NodeQuery::defaultMaxNum_=50000;

#define _UI_NODEQUERY_DEBUG

NodeQueryStringOption::NodeQueryStringOption(QString name) :
  name_(name),
  matchMode_(defaultMatchMode_),
  caseSensitive_(defaultCaseSensitive_)
{
	/*if(matchOper_.isEmpty())
	{
		matchOper_[ContainsMatch]="~";
		matchOper_[WildcardMatch]="=";
		matchOper_[RegexpMatch]="=~";
	}*/
}

void NodeQueryStringOption::swap(const NodeQueryStringOption* op)
{
	value_=op->value();
	matchMode_=op->matchMode();
	caseSensitive_=op->caseSensitive();
}


/*QString NodeQueryStringOption::matchOperator() const
{
	return matchOper_.value(matchMode_);
}*/

void NodeQueryStringOption::save(VSettings* vs)
{
    if(value_.isEmpty() && matchMode_.mode() == defaultMatchMode_ &&
       caseSensitive_ == defaultCaseSensitive_)
        return;
        
    vs->beginGroup(name_.toStdString());
    vs->put("value",value_.toStdString());
    vs->put("matchMode",matchMode_.toInt());
    vs->putAsBool("caseSensistive",caseSensitive_);
    vs->endGroup();   
}


void NodeQueryStringOption::load(VSettings* vs)
{
    if(!vs->contains(name_.toStdString()))
        return;
    
    vs->beginGroup(name_.toStdString());
    value_=QString::fromStdString(vs->get("value",value_.toStdString())); 
    matchMode_.setMode(static_cast<StringMatchMode::Mode>(vs->get<int>("matchMode",matchMode_.toInt())));
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


NodeQuery::NodeQuery(const std::string& name,bool ignoreMaxNum) :
  name_(name),
  advanced_(false),
  //allServers_(true),
  caseSensitive_(false),
  maxNum_(defaultMaxNum_),
  ignoreMaxNum_(ignoreMaxNum)
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
	selectOptions_["attr"]=new NodeQuerySelectOption("attr");
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
	//query_=q->query_;
	extQuery_=q->extQuery_;
	rootNode_=q->rootNode_;
	servers_=q->servers_;
	//allServers_=q->allServers_;
	caseSensitive_=q->caseSensitive_;
	maxNum_=q->maxNum_;
	ignoreMaxNum_=q->ignoreMaxNum_;

	Q_FOREACH(QString s,stringOptions_.keys())
	{
		stringOptions_[s]->swap(q->stringOptions_.value(s));
	}

	Q_FOREACH(QString s,selectOptions_.keys())
	{
		selectOptions_[s]->swap(q->selectOptions_.value(s));
	}

	buildQueryString();
}

void  NodeQuery::setName(const std::string& name)
{
	name_=name;
}

#if 0
    void NodeQuery::setQuery(QString query)
{
	query_=query;
}
#endif

bool NodeQuery::hasServer(const std::string& name) const
{
	qDebug() << "servers" << servers_;

	if(servers_.empty())
		return true;

	return servers_.contains(QString::fromStdString(name));
}

void NodeQuery::adjustServers(const std::vector<std::string>& all)
{
	QStringList allLst=vecToLst(all);

	/*if(allServers_)
	{
		servers_=allLst;
		buildQueryString();
	}*/
}

QStringList NodeQuery::typeSelection() const
{
	return selectOptions_.value("type")->selection_;
}

std::vector<std::string> NodeQuery::typeSelectionVec() const
{
	return lstToVec(selectOptions_.value("type")->selection_);
}

QStringList NodeQuery::stateSelection() const
{
	return selectOptions_.value("state")->selection_;
}

std::vector<std::string> NodeQuery::stateSelectionVec() const
{
	return lstToVec(selectOptions_.value("state")->selection_);
}

QStringList NodeQuery::flagSelection() const
{
	return selectOptions_["flag"]->selection_;
}

std::vector<std::string> NodeQuery::flagSelectionVec() const
{
	return lstToVec(selectOptions_["flag"]->selection_);
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

std::vector<std::string> NodeQuery::attrGroupSelectionVec() const
{
	return lstToVec(selectOptions_["attr"]->selection_);
}

void NodeQuery::setTypeSelection(QStringList lst)
{
	selectOptions_["type"]->selection_=lst;
}

void NodeQuery::setTypeSelection(const std::vector<std::string>& vec)
{
	selectOptions_["type"]->selection_=vecToLst(vec);
}


void NodeQuery::setStateSelection(QStringList lst)
{
	selectOptions_["state"]->selection_=lst;
}

void NodeQuery::setStateSelection(const std::vector<std::string>& vec)
{
	selectOptions_["state"]->selection_=vecToLst(vec);
}

void NodeQuery::setFlagSelection(QStringList lst)
{
	selectOptions_["flag"]->selection_=lst;
}

void NodeQuery::setFlagSelection(const std::vector<std::string>& vec)
{
	selectOptions_["flag"]->selection_=vecToLst(vec);
}

void NodeQuery::setAttrGroupSelection(QStringList lst)
{
	selectOptions_["attr"]->selection_=lst;
}

void NodeQuery::setAttrGroupSelection(const std::vector<std::string>& vec)
{
	selectOptions_["attr"]->selection_=vecToLst(vec);
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
    keys << "node" << "type" << "state" << "flag";
    Q_FOREACH(QString s,keys)
    {
        if(!extQuery_.value(s).isEmpty())
            lst << extQuery_.value(s);
    }
    
    //TODO : handle all   
    return lst.join(" and ");
}    
    
QString NodeQuery::attrQueryPart()  const  
{        
    return extQuery_.value("attr");
}

bool NodeQuery::hasAttribute(QString typeName) const
{
    return attrGroupSelection().contains(typeName);
}

void NodeQuery::buildQueryString()
{
#ifdef _UI_NODEQUERY_DEBUG
    UserMessage::debug("NodeQuery::buildQueryString -->");
#endif

    //Scope
	QString nodePart;
	QString name=stringOptions_["node_name"]->value().simplified();
	QString path=stringOptions_["node_path"]->value().simplified();
	if(!name.isEmpty())
	{
		nodePart="node_name " + stringOptions_["node_name"]->matchOperator() + " \'" +  name + "\'";
	}
	if(!path.isEmpty())
	{
		if(!nodePart.isEmpty())
			nodePart+=" and ";

		nodePart+="node_path " + stringOptions_["node_path"]->matchOperator() + " \'" +  path + "\'";
	}
	if(!nodePart.isEmpty())
	{
		nodePart="(" + nodePart + ")";
	}

	//Type
	QString typePart;
	if(typeSelection().count() >0)
	{
		typePart="(" + typeSelection().join(" or ") + ")";
	}

	//State
	QString statePart;
	if(stateSelection().count() >0)
	{
		statePart="(" + stateSelection().join(" or ") + ")";
	}

	//Flag
	QString flagPart;
	if(flagSelection().count() >0)
	{
		flagPart="(" + flagSelection().join(" or ") + ")";
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
            qDebug() << "attr" << op->name() << op->value();
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
		attrPart="(" + attrPart + ")";
	}

	//Put everything together
	//query_.clear()
	
    extQuery_.clear();

    extQuery_["node"]=nodePart;
    extQuery_["type"]=typePart;
    extQuery_["state"]=statePart;
    extQuery_["flag"]=flagPart;
    
    if(extQuery_.values().join("") == "")
        extQuery_["node"] = "ALL";
    
    extQuery_["attr"]=attrPart;
    
    bool hasEq=extQuery_.values().join("").contains("=");
      
#if 0    
	if(!nodePart.isEmpty())
	{
		hasNodeAPrt=true;
        //query_+=nodePart;
	   extQuery_["node"]=nodePart;
	}

	if(!typePart.isEmpty())
	{
		//if(!query_.isEmpty())
		//	query_+=" and ";

		//query_+=typePart;
		extQuery_["type"]=typePart;
	}

	if(!statePart.isEmpty())
	{
		//if(!query_.isEmpty())
		//	query_+=" and ";

		//query_+=statePart;
		extQuery_["state"]=statePart;
	}

	if(!flagPart.isEmpty())
	{
		//if(!query_.isEmpty())
		//	query_+=" and ";

		//query_+=flagPart;
		extQuery_["flag"]=flagPart;
	}
	
	if(!attrPart.isEmpty())
	{
		//if(!query_.isEmpty())
		//	query_+=" and ";

		//query_+=attrPart;
		extQuery_["attr"]=attrPart;
	}
#endif

	//Extended query
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

	//if(query_.isEmpty())
	//	extQuery_["node"] = "ALL";

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

#ifdef _UI_NODEQUERY_DEBUG
    qDebug() << extQuery_;
#endif

#ifdef _UI_NODEQUERY_DEBUG
    UserMessage::debug("<-- NodeQuery::buildQueryString");
#endif
}

QString NodeQuery::extQueryHtml(bool multi,QColor bgCol,int firstColWidth) const
{
	QString str;
	QString bg=bgCol.name();

	if(multi)
	{
			str="<table width=\"100%\" cellPadding=\"2\">";
			if(!extQuery_.value("scope").isEmpty())
				str+="<tr><td width=\"" + QString::number(firstColWidth) + "\" bgcolor=\"" + bg +
				       "\">scope</td><td bgcolor=\"" + bg + "\">" + extQuery_.value("scope") + "</tr></td>";

			QStringList nodeParts;
            nodeParts << "node" << "type" << "state" << "flag";
			Q_FOREACH(QString s,nodeParts)
			{
				if(!extQuery_.value(s).isEmpty())
				{
					//if(!str.isEmpty() && !str.contains("<td>nodes"))
					//	str+="<br>";

					if(!str.contains("nodes</td>"))
						str+="<tr><td bgcolor=\"" + bg + "\">nodes</td><td bgcolor=\"" + bg + "\">"+ extQuery_.value(s);
					else
						str+=" and<br> " + extQuery_.value(s);
				}
			}

			if(str.contains("nodes</td>"))
				str+="</td></tr>";

            if(!extQuery_.value("attr").isEmpty())
            {
                //if(!str.isEmpty())
                //	str+="\n";
                str+="<tr><td bgcolor=\"" + bg + "\">attributes</td><td bgcolor=\"" + bg + "\">" + extQuery_.value("attr") +"</td></tr>";
            }



			if(!extQuery_.value("options").isEmpty())
			{
				//if(!str.isEmpty())
				//	str+="\n";
				str+="<tr><td bgcolor=\"" + bg + "\">options</td><td bgcolor=\"" + bg + "\">" + extQuery_.value("options") +"</td></tr>";
			}

			str+="</table>";
		}

	else
	{
		QString css;
		css = "<style type=\"text/css\">";
		css += "table.tbl {border-width: 1px;border-style: solid;border-color: \"#AAAAAA\";margin-top: 0px;margin-bottom: 0px;color: black;}";
		//css += "table.tbl td {padding: 3px;}";
		//css += "table.tbl th {padding: 3px;}";
		css+="</style>";

		QString bgDark=bgCol.darker(108).name();
		str="<table cellSpacing=\"0\" class=\"tbl\">";

		if(!extQuery_.value("scope").isEmpty())
			str+="<tr><td bgcolor=\"" + bg + "\">&nbsp;scope:&nbsp;</td>" +
			"<td bgcolor=\"" + bgDark + "\">&nbsp;" + extQuery_.value("scope") + "&nbsp;</td><td>&nbsp;&nbsp;</td>";

		QStringList nodeParts;
		nodeParts << "node" << "type" << "state" << "flag";
		Q_FOREACH(QString s,nodeParts)
		{
			if(!extQuery_.value(s).isEmpty())
			{
				//if(!str.isEmpty() && !str.contains("nodes: "))
				//	str+=" | ";

				if(!str.contains("nodes"))
					str+="&nbsp;<td  bgcolor=\"" + bg + "\">&nbsp;nodes:&nbsp;</td><td bgcolor=\"" + bgDark + "\">&nbsp;"+ extQuery_.value(s);
				else
					str+=" and " + extQuery_.value(s);
			}
		}

		if(str.contains("nodes:"))
			str+="&nbsp;</td></td><td>&nbsp;&nbsp;</td>";

		if(!extQuery_.value("options").isEmpty())
		{
			//if(!str.isEmpty())
			//	str+=" | ";
			str+="&nbsp;<td bgcolor=\"" + bg + "\"> &nbsp;options:&nbsp;</td><td bgcolor=\"" + bgDark + "\">&nbsp;" + extQuery_.value("options") + "&nbsp;</td>";
		}

		if(str.contains("<tr>"))
			str+="</tr>";

		str+="</table>";

		//str=css+str;
	}

	return str;
}

void NodeQuery::load(VSettings* vs)
{
	advanced_=vs->getAsBool("advanced",advanced_);
	caseSensitive_=vs->getAsBool("case",caseSensitive_);

	int maxNum=vs->get<int>("maxNum",maxNum_);
	if(maxNum_ > 1 && maxNum < 5000000)
		maxNum_=maxNum;

	std::vector<std::string> v;
	vs->get("servers",v);
	servers_.clear();
	for(std::vector<std::string>::const_iterator it=v.begin(); it != v.end(); ++it)
	    servers_ << QString::fromStdString(*it);

	//allServers_=vs->getAsBool("allServers",allServers_);

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

    //if(!allServers_)
   //     vs->putAsBool("allServers",allServers_);

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

std::vector<std::string> NodeQuery::lstToVec(QStringList lst) const
{
	std::vector<std::string> vec;
	Q_FOREACH(QString s,lst)
		vec.push_back(s.toStdString());
	return vec;
}

QStringList NodeQuery::vecToLst(const std::vector<std::string>& vec) const
{
	QStringList lst;
	for(std::vector<std::string>::const_iterator it=vec.begin(); it != vec.end(); ++it)
		lst << QString::fromStdString(*it);
	return lst;
}


