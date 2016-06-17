//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "NodeQuery.hpp"

#include <QDebug>

#include "NodeQueryOption.hpp"
#include "UserMessage.hpp"
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


NodeQuery::NodeQuery(const std::string& name,bool ignoreMaxNum) :
  name_(name),
  advanced_(false),
  caseSensitive_(false),
  maxNum_(defaultMaxNum_),
  ignoreMaxNum_(ignoreMaxNum)
{

#ifdef _UI_NODEQUERY_DEBUG
    UserMessage::debug("NodeQuery::NodeQuery -->");
#endif

    //Build options from defs. Options store the actual query settings. They are
    //editable through the gui.
    NodeQueryOption::build(this);

#ifdef _UI_NODEQUERY_DEBUG
    qDebug() << options_;
#endif

#ifdef _UI_NODEQUERY_DEBUG
    UserMessage::debug("<-- NodeQuery::NodeQuery");
#endif
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
    UserMessage::debug("NodeQuery::hasServer -->");
    qDebug() << "   servers=" << servers_;
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
    return NULL;
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
    NodeQueryListOption* op=static_cast<NodeQueryListOption*>(a);
    Q_ASSERT(op);
    return op->selection();
}


void NodeQuery::buildQueryString()
{
#ifdef _UI_NODEQUERY_DEBUG
    UserMessage::debug("NodeQuery::buildQueryString -->");
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

    bool hasEq=extQuery_.values().join("").contains("=");

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

#ifdef _UI_NODEQUERY_DEBUG
    qDebug() << extQuery_;
#endif

    sqlQuery_.clear();
    QStringList selectPart;
    QStringList fromPart;
    QStringList wherePart;
    sqlQuery_="SELECT";
    QStringList nodeParts;
    nodeParts << "node" << "type" << "state" << "flag";
    Q_FOREACH(QString s,nodeParts)
    {
        if(!extQuery_.value(s).isEmpty())
            wherePart << extQuery_.value(s);
    }
    selectPart << "node";

    //Attribute
    Q_FOREACH(QString attrName,attrSelection())
    {
        NodeQueryAttrGroup* grp=attrGroup_.value(attrName);
        Q_ASSERT(grp);
        selectPart << grp->name();

        QString grpPart=grp->query();
        wherePart << grpPart;
    }

    sqlQuery_+=" " + selectPart.join(", ");

    //FROM
    if(!servers_.isEmpty())
    {
        if(servers_.size() ==1 && !rootNode_.empty())
        {
            fromPart << QString::fromStdString(rootNode_);
        }
        else
            fromPart=servers_;

        sqlQuery_+=" FROM " + fromPart.join(", ");
    }
    else
    {
        sqlQuery_+=" FROM any";
    }

    if(wherePart.count() > 0)
        sqlQuery_+=" WHERE " + wherePart.join(" and ");

#ifdef _UI_NODEQUERY_DEBUG
    UserMessage::debug("<-- NodeQuery::buildQueryString");
#endif
}

QString NodeQuery::extQueryHtml(bool multi,QColor bgCol,int firstColWidth) const
{
	QString str;
	QString bg=bgCol.name();

    //Multiline version
	if(multi)
	{
			str="<table width=\"100%\" cellPadding=\"2\">";
			if(!extQuery_.value("scope").isEmpty())
				str+="<tr><td width=\"" + QString::number(firstColWidth) + "\" bgcolor=\"" + bg +
				       "\">scope</td><td bgcolor=\"" + bg + "\">" + extQuery_.value("scope") + "</tr></td>";

            if(nodeQueryPart().isEmpty())
            {
                QString v="ALL";
                if(!str.contains("nodes</td>"))
                    str+="<tr><td bgcolor=\"" + bg + "\">nodes</td><td bgcolor=\"" +
                            bg + "\">"+ v;
                else
                    str+=" and<br> " + v;
            }
            else
            {
                QStringList nodeParts;
                nodeParts << "node" << "type" << "state" << "flag";

                Q_FOREACH(QString s,nodeParts)
                {
                    if(!extQuery_.value(s).isEmpty())
                    {
                        if(!str.contains("nodes</td>"))
                            str+="<tr><td bgcolor=\"" + bg + "\">nodes</td><td bgcolor=\"" + bg + "\">"+ extQuery_.value(s);
                        else
                            str+=" and<br> " + extQuery_.value(s);
                    }
                }
            }

			if(str.contains("nodes</td>"))
				str+="</td></tr>";

            if(!extQuery_.value("attr").isEmpty())
            {              
                str+="<tr><td bgcolor=\"" + bg + "\">attributes</td><td bgcolor=\"" + bg + "\">" + extQuery_.value("attr") +"</td></tr>";
            }

			if(!extQuery_.value("options").isEmpty())
			{				
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

        if(nodeQueryPart().isEmpty())
        {
            QString v="ALL";
            if(!str.contains("nodes"))
                str+="&nbsp;<td  bgcolor=\"" + bg + "\">&nbsp;nodes:&nbsp;</td><td bgcolor=\"" +
                        bgDark + "\">&nbsp;"+ v;
            else
                str+=" and " + v;
        }
        else
        {
            QStringList nodeParts;
            nodeParts << "node" << "type" << "state" << "flag";
            Q_FOREACH(QString s,nodeParts)
            {
                if(!extQuery_.value(s).isEmpty())
                {
                    if(!str.contains("nodes"))
                        str+="&nbsp;<td  bgcolor=\"" + bg + "\">&nbsp;nodes:&nbsp;</td><td bgcolor=\"" + bgDark + "\">&nbsp;"+ extQuery_.value(s);
                    else
                        str+=" and " + extQuery_.value(s);
                }
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
