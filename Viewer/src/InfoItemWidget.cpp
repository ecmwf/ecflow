//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "InfoItemWidget.hpp"

#include "Defs.hpp"
#include "DState.hpp"
#include "Node.hpp"
#include "Suite.hpp"
#include "Variable.hpp"

#include "ServerHandler.hpp"

#include "boost/date_time/posix_time/posix_time.hpp"

//========================================================
//
// InfoItemWidget
//
//========================================================

InfoItemWidget::InfoItemWidget(QWidget *parent) : TextItemWidget(parent)
{
}

QWidget* InfoItemWidget::realWidget()
{
	return this;
}

void InfoItemWidget::reload(ViewNodeInfo_ptr nodeInfo)
{
	loaded_=true;

	if(nodeInfo->isNode())
	{
		Node* n=nodeInfo->node();
		ServerHandler *server=nodeInfo->server();

		std::stringstream ss;
		info(server,n,ss);

		QString s=QString::fromStdString(ss.str());
		textEdit_->setPlainText(s);

	 //std::stringstream ss;
	 //n.info(ss);
	}
	else
	{
		textEdit_->clear();
	}

}

void InfoItemWidget::clearContents()
{
	loaded_=false;
	textEdit_->clear();
}

void InfoItemWidget::info(ServerHandler *server,Node* node,std::stringstream& f)
{
	//taken from task_node.cc

	static const std::string inc = "  ";

	using namespace boost::posix_time;
	using namespace boost::gregorian;

	if(!node) return;

	std::string typeName="node";
	std::string nodeName=node->name();
	DState::State st=node->dstate();
	std::string statusName(DState::toString(st));

	f << "name     : " << nodeName << "\n";
	f << "type     : " << typeName << "\n";
	f << "status   : " << statusName << "\n";

	boost::posix_time::ptime state_change_time = boost::posix_time::ptime();
    if(!state_change_time.is_special())
    {
    	f << "at  : " << boost::posix_time::to_simple_string(state_change_time) << "\n";
	}

    f << "----------\n";

	f << typeName << " " << node->name() << "\n";

    if(node->isSuite())
    {
    	Suite* suite = dynamic_cast<Suite*>(node);
    	// f << "clock    : ";
    	if (suite->clockAttr()) {
    		suite->clockAttr().get()->print(f); // f << "\n";
    	}
     }

    if(st  != DState::QUEUED && st != DState::UNKNOWN)
    	f << inc << "defstatus " << DState::toString(st) << "\n";

    //zombies attribute
    const std::vector<ZombieAttr> & vect = node->zombies();
    for (std::vector<ZombieAttr>::const_iterator it = vect.begin(); it != vect.end(); ++it)
    		f << inc << it->toString() << "\n";

    //autocancel
    if(node->hasAutoCancel() && node->get_autocancel())
          f << inc << node->get_autocancel()->toString() << "\n";


    if(node->dstate() == DState::SUSPENDED)
    	f << inc << "# " << typeName << " " << nodeName << " is " << DState::toString(st);
       //    << "\n";

    std::vector<Variable> gvar;
    std::vector<Variable>::const_iterator gvar_end;

    ServerDefsAccess defsAc(server);
    defs_ptr defs = defsAc.defs();

    if(node->hasTimeDependencies())
    {
	      f << inc << "# time-date-dependencies: ";
	      if (node->isTimeFree()) f << "free\n";
	      else f << "holding\n";
    }

    node->gen_variables(gvar);
    for(std::vector<Variable>::const_iterator it = gvar.begin(); it != gvar.end(); ++it)
    {
    	f << inc << "# edit " << (*it).name() << " '" << (*it).theValue() << "'\n";
    }

    gvar = node->variables();
    for(std::vector<Variable>::const_iterator it = gvar.begin(); it != gvar.end(); ++it)
    {
	      f << inc << "edit " << (*it).name() << " '" << (*it).theValue() << "'\n";
    }

    if(defs)
    {
    	const std::vector<Variable>& gvar = defs->server().user_variables();
        for(std::vector<Variable>::const_iterator it = gvar.begin(); it != gvar.end(); ++it)
        {
            f << inc << "# edit " << (*it).name() << " '" << (*it).theValue() << "'\n";
        }

        const std::vector<Variable>& var = defs->server().server_variables();
        for(std::vector<Variable>::const_iterator it = var.begin(); it != var.end(); ++it)
        {
        	f << inc << "edit " << (*it).name() << " '" << (*it).theValue() << "'\n";
        }
	}

  f << "end" << typeName << " # " << nodeName << "\n";
}

static InfoPanelItemMaker<InfoItemWidget> maker1("info");
