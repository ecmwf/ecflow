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

#include "UserMessage.hpp"
#include "VAttributeType.hpp"
#include "VConfigLoader.hpp"
#include "VProperty.hpp"
#include "VSettings.hpp"

StringMatchMode::Mode NodeQueryStringOption::defaultMatchMode_=StringMatchMode::WildcardMatch;
bool NodeQueryStringOption::defaultCaseSensitive_=false;
QMap<QString,NodeQueryDef*> NodeQuery::def_;
QMap<QString,NodeQueryAttrDef*> NodeQuery::attrDef_;
int NodeQuery::defaultMaxNum_=50000;

#define _UI_NODEQUERY_DEBUG

//===============================================
//
// NodeQueryDef
//
//===============================================

NodeQueryDef::NodeQueryDef(VProperty* p)
{
    name_=p->name();
    label_=p->param("label");
    if(label_.isEmpty())
        label_=name_;
}

void NodeQueryStringDef::buildOption(NodeQuery* q)
{
    q->options_[name_]=new NodeQueryStringOption(this);
}

NodeQueryListDef::NodeQueryListDef(VProperty* p) : NodeQueryDef(p)
{
    values_=p->param("values").split("|");
    valueLabels_=p->param("labels").split("|");
    if(valueLabels_.count() == 1 && valueLabels_[0].isEmpty())
        valueLabels_=values_;

    Q_ASSERT(valueLabels_.count() == values_.count());
}

void NodeQueryListDef::buildOption(NodeQuery* q)
{
    q->options_[name_]=new NodeQueryListOption(this);
}

NodeQueryComboDef::NodeQueryComboDef(VProperty* p) : NodeQueryDef(p)
{
    values_=p->param("values").split("|");
    valueLabels_=p->param("labels").split("|");
    if(valueLabels_.count() == 1 && valueLabels_[0].isEmpty())
        valueLabels_=values_;

    Q_ASSERT(valueLabels_.count() == values_.count());
}

void NodeQueryComboDef::buildOption(NodeQuery* q)
{
    q->options_[name_]=new NodeQueryComboOption(this);
}

NodeQueryAttrDef::NodeQueryAttrDef(VProperty* p) : NodeQueryDef(p)
{
    QString types=p->param("types");
    if(types.isEmpty())
    {
        VAttributeType *t=VAttributeType::find(types.toStdString());
        Q_ASSERT(t);
        types_ << t;
    }
    else
    {
        QStringList typeLst=types.split("|");
        Q_FOREACH(QString s,typeLst)
        {
            VAttributeType *t=VAttributeType::find(s.toStdString());
            Q_ASSERT(t);
            types_ << t;
        }
     }

     Q_ASSERT(!types_.empty());

     //TODO: use a factory here
     Q_FOREACH(VProperty* ch,p->children())
     {
         if(ch->name() == "string")
         {
            defs_ << new NodeQueryStringDef(ch);
         }
         else if(ch->name() == "combo")
         {
            defs_ << new NodeQueryComboDef(ch);
         }
    }
}

void NodeQueryAttrDef::buildOption(NodeQuery* q)
{
    Q_FOREACH(NodeQueryDef *t,defs_)
        t->buildOption(q);
}

//===============================================
//
// NodeQueryStringOption
//
//===============================================

NodeQueryStringOption::NodeQueryStringOption(NodeQueryDef* def) :
  NodeQueryOption(def),
  matchMode_(defaultMatchMode_),
  caseSensitive_(defaultCaseSensitive_)
{
}

QString NodeQueryStringOption::query() const
{
    QString s;
    if(!value_.isEmpty())
    {
        s= name() + " " + matchOperator() + " \'" + value_ + "\'";
    }
    return s;
}

void NodeQueryStringOption::swap(const NodeQueryOption* option)
{
    const NodeQueryStringOption* op=static_cast<const NodeQueryStringOption*>(option);
    Q_ASSERT(op);

    value_=op->value();
	matchMode_=op->matchMode();
	caseSensitive_=op->caseSensitive();
}

void NodeQueryStringOption::save(VSettings* vs)
{
    if(value_.isEmpty() && matchMode_.mode() == defaultMatchMode_ &&
       caseSensitive_ == defaultCaseSensitive_)
        return;
        
    vs->beginGroup(name().toStdString());
    vs->put("value",value_.toStdString());
    vs->put("matchMode",matchMode_.toInt());
    vs->putAsBool("caseSensistive",caseSensitive_);
    vs->endGroup();   
}

void NodeQueryStringOption::load(VSettings* vs)
{
    if(!vs->contains(name().toStdString()))
        return;
    
    vs->beginGroup(name().toStdString());
    value_=QString::fromStdString(vs->get("value",value_.toStdString())); 
    matchMode_.setMode(static_cast<StringMatchMode::Mode>(vs->get<int>("matchMode",matchMode_.toInt())));
    caseSensitive_=vs->getAsBool("caseSensistive",caseSensitive_);
    vs->endGroup();   
}

//===============================================
//
// NodeQueryListOption
//
//===============================================

QString NodeQueryListOption::query(QString op) const
{
    QString s;
    if(!selection_.isEmpty())
    {
        s=selection_.join(op);
    }
    return s;
}

void NodeQueryListOption::swap(const NodeQueryOption* option)
{
    const NodeQueryListOption* op=static_cast<const NodeQueryListOption*>(option);
    Q_ASSERT(op);
    selection_=op->selection();
}

void NodeQueryListOption::save(VSettings* vs)
{
    if(selection_.isEmpty())
        return;

    std::vector<std::string> v;
    Q_FOREACH(QString s, selection_)
        v.push_back(s.toStdString());
        
    vs->put(name().toStdString(),v);
}

void NodeQueryListOption::load(VSettings* vs)
{
    if(!vs->contains(name().toStdString()))
        return;

    std::vector<std::string> v;
    vs->get(name().toStdString(),v);
    
    selection_.clear();
    for(std::vector<std::string>::const_iterator it=v.begin(); it != v.end(); ++it)
        selection_ << QString::fromStdString(*it);
}

//===============================================
//
// NodeQueryComboOption
//
//===============================================

QString NodeQueryComboOption::query(QString op) const
{
    QString s;
    if(!value_.isEmpty())
    {
        s= name() + " " + " \'" + value_ + "\'";
    }
    return s;
}

void NodeQueryComboOption::swap(const NodeQueryOption* option)
{
    const NodeQueryComboOption* op=static_cast<const NodeQueryComboOption*>(option);
    Q_ASSERT(op);
    value_=op->value();
}

void NodeQueryComboOption::save(VSettings* vs)
{
    vs->put(name().toStdString(),value().toStdString());
}

void NodeQueryComboOption::load(VSettings* vs)
{

}

//===============================================
//
// NodeQuery
//
//===============================================

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
    for(QMap<QString,NodeQueryDef*>::const_iterator it = def_.constBegin(); it != def_.constEnd(); ++it)
    {
        it.value()->buildOption(this);
    }

    for(QMap<QString,NodeQueryAttrDef*>::const_iterator it = attrDef_.constBegin(); it != attrDef_.constEnd(); ++it)
    {
        it.value()->buildOption(this);
    }

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

#if 0

QStringList NodeQuery::attrDefs(QString group)
{
    if(NodeQueryAttributeDef* a=attr_.value(group))
    {
        //return a->defs_;
    }
}
#endif


NodeQueryOption* NodeQuery::option(QString name) const
{
    QMap<QString,NodeQueryOption*>::const_iterator it = options_.find(name);
    if(it != options_.constEnd())
        return it.value();
    return NULL;
}

NodeQueryDef* NodeQuery::def(QString name)
{
    QMap<QString,NodeQueryDef*>::const_iterator it = def_.find(name);
    if(it != def_.constEnd())
        return it.value();
    return NULL;
}


#if 0
QStringList NodeQuery::attrGroupSelection() const
{
    return options_["attribute"]->selection();
    //return selectOptions_["attr"]->selection_;
}
#endif
#if 0
std::vector<std::string> NodeQuery::attrGroupSelectionVec() const
{
	return lstToVec(selectOptions_["attr"]->selection_);
}
#endif

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
    QStringList attrSel; //=options_["attribute"]->selection();
    QString q;
    Q_FOREACH(NodeQueryAttrDef* tm,attrDef_.values())
    {
        if(tm->hasType(t) &&
           attrSel.contains(tm->name()))
        {
            QStringList qLst;

            Q_FOREACH(NodeQueryDef* def,tm->defs_)
            {
                NodeQueryOption* op=options_[def->name()];
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

#if 0
    QString q;

    NodeQueryAttributeDef* def=0;
    Q_FOREACH(NodeQueryAttributeDef* tm,attr_.values())
    {
        if(tm->types_.contains(t) &&
           attrGroupSelection().contains(tm->name_))
        {
            def=tm;
            break;
        }
    }

    if(term)
    {
        QStringList qLst;
        Q_FOREACH(QString opName,term->terms_)
        {
            NodeQueryStringOption* op=stringOption(opName);
            assert(op);
            QString s=op->value();
            qDebug() << "attr" << op->name() << op->value();
            if(!s.isEmpty())
            {
                qLst <<  opName + " " + op->matchOperator() + " \'" + s + "\'";
            }
        }

        if(qLst.isEmpty())
            q=t->name();
        else
            q=qLst.join(" or ");
    }

    return q;
#endif

}

bool NodeQuery::hasAttribute(VAttributeType *t) const
{
    Q_FOREACH(NodeQueryAttrDef* d,attrDef_.values())
    {
#if 0
        if(d->hasType(t))
            return attrGroupSelection().contains(term->name_);
#endif
    }
    return false;
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
#if 0
    Q_FOREACH(QString gr,attrGroupSelection())
	{
        QString grPart=attrQueryPart(VAttributeType::find(gr.toStdString()));

		if(!attrPart.isEmpty())
			attrPart+=" or ";
		
        attrPart+=grPart;
	}
#endif
	if(!attrPart.isEmpty())
        extQuery_["attr"]="(" + attrPart + ")";

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
#if 0
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
#endif

//Called from VConfigLoader
void NodeQuery::load(VProperty* group)
{
#ifdef _UI_NODEQUERY_DEBUG
    UserMessage::debug("NodeQuery::load -->");
#endif

    if(group->name() == "node_query")
    {
        for(int i=0; i < group->children().size(); i++)
        {
            VProperty *p=group->children().at(i);
            QString type=p->param("type");

#ifdef _UI_NODEQUERY_DEBUG
            UserMessage::qdebug("   name=" + p->name() + " type=" + type);
#endif
            if(type.isEmpty() || type == "string")
            {
                def_[p->name()]=new NodeQueryStringDef(p);
            }
            else if(type == "list")
            {
                def_[p->name()]=new NodeQueryListDef(p);
            }
        }
     }
     else if(group->name() == "attribute_query")
     {
        for(int i=0; i < group->children().size(); i++)
        {
            VProperty *p=group->children().at(i);
            if(p->name() == "attribute")
            {
                def_[p->name()]=new NodeQueryListDef(p);

                Q_FOREACH(VProperty* ch,p->children())
                {
                    //Q_ASSERT(attrDef_->values_.contains(ch->name()));
                    attrDef_[ch->name()] = new NodeQueryAttrDef(ch);
                }
           }
        }
    }
}

static SimpleLoader<NodeQuery> loaderNodeQuery("node_query");
static SimpleLoader<NodeQuery> loaderAttrQuery("attribute_query");
