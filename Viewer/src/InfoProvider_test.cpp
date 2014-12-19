//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "InfoProvider.hpp"

#include "Node.hpp"
#include "Suite.hpp"
#include "VNState.hpp"

#include "boost/date_time/posix_time/posix_time.hpp"

static std::string suiteStr("suite");
static std::string familyStr("family");
static std::string taskStr("task");
static std::string defaultStr("node");

const std::string&  InfoProvider::nodeType(Node* node)
{
	if(node->isSuite())
		return suiteStr;
	else if(node->isFamily())
		return familyStr;
	else if(node->isTask())
			return taskStr;

	return defaultStr;
}



//Re-implemented from host.cc
//echost::stats(std::ostream& f)

//void InfoProvider::info(ServerHandler* server,std::stringstream& f)
//{
//	client_.stats();
 //   client_.server_reply().stats().show(buf);
//}


//Re-implemented from simple_node.cc
//simple_node::info(std::ostream& f)

void InfoProvider::info(Node* node,std::stringstream& f)
{
	if(!node) return;

	static const std::string inc = "  ";

	using namespace boost::posix_time;
	using namespace boost::gregorian;

	std::string typeName=nodeType(node);
	std::string nodeName=node->name();
	std::string statusName(VNState::toName(node).toStdString());

	//Header
	f << "name    : " << nodeName << "\n";
	f << "type    : " << typeName << "\n";
	f << "status   : " << statusName << "\n";

	boost::posix_time::ptime state_change_time = node->state_change_time();
    if(!state_change_time.is_special())
    {
    	f << "at      : " << boost::posix_time::to_simple_string(state_change_time) << "\n";
	}

    f << "----------\n";

    //Start block: Type, name
	f << typeName << " " << node->name() << "\n";

	//Clock information for suites
    if(node->isSuite())
    {
    	Suite* suite = dynamic_cast<Suite*>(node);
    	// f << "clock    : ";
    	if (suite->clockAttr()) {
    		suite->clockAttr().get()->print(f); // f << "\n";
    	}
    }

    //Default status: the status the node should have when the begin/re-queue is called
    //if(st  != DState::QUEUED && st != DState::UNKNOWN)
    f << inc << "defstatus " <<  VNState::toDefaultStateName(node).toStdString() << "\n";

    //Zombies attribute
    const std::vector<ZombieAttr> & vect = node->zombies();
    for (std::vector<ZombieAttr>::const_iterator it = vect.begin(); it != vect.end(); ++it)
    		f << inc << it->toString() << "\n";

    //Autocancel
    if(node->hasAutoCancel() && node->get_autocancel())
          f << inc << node->get_autocancel()->toString() << "\n";

    f << inc << "# " << typeName << " " << nodeName << " is " << statusName << "\n";

    if(node->hasTimeDependencies())
    {
	      f << inc << "# time-date-dependencies: ";
	      if (node->isTimeFree()) f << "free\n";
	      else f << "holding\n";
    }

    //Generated variables
    std::vector<Variable> gvar;
    std::vector<Variable>::const_iterator gvar_end;
    node->gen_variables(gvar);
    for(std::vector<Variable>::const_iterator it = gvar.begin(); it != gvar.end(); ++it)
    {
    	f << inc << "# edit " << (*it).name() << " '" << (*it).theValue() << "'\n";
    }

    //Variables
    gvar = node->variables();
    for(std::vector<Variable>::const_iterator it = gvar.begin(); it != gvar.end(); ++it)
    {
	      f << inc << "edit " << (*it).name() << " '" << (*it).theValue() << "'\n";
    }


    //Print children
    std::vector<node_ptr> nodes;
    node->immediateChildren(nodes);
    for(unsigned int i=0; i < nodes.size(); i++)
    {
    	f << inc << nodeType(nodes.at(i).get()) << " " << nodes.at(i)->name() << "\n";
	}

    //Here we should print some additional information from the attributes well. It i not clear exactly what!

    //End block
    f << "end" << typeName << " # " << nodeName << "\n";
}

/*
    ServerDefsAccess defsAc(server);
    defs_ptr defs = defsAc.defs();

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
	}*/





/*

void simple_node::info(std::ostream& f)
{
  static const std::string inc = "  ";
  node::info(f);
  f << type_name() << " " << name() << "\n";
  {
    if (owner_) {

      if (owner_->type() == NODE_SUITE) {
	Suite* suite = dynamic_cast<Suite*>(owner_->get_node());
	// f << "clock    : ";
	if (suite->clockAttr()) {
	  suite->clockAttr().get()->print(f); // f << "\n";
	}
      }

      int defs = owner_->defstatus();
      if (defs != STATUS_QUEUED && defs != STATUS_UNKNOWN)
        f << inc << "defstatus " << ecf::status_name[defs] << "\n";

      Node* node = owner_->get_node();
      if (node) {
        // if (node->repeat().toString() != "") // repeat // duplicated on suite node
        //  f << inc << node->repeat().toString() << "\n";

        // zombies attribute
        const std::vector<ZombieAttr> & vect = node->zombies();
        std::vector<ZombieAttr>::const_iterator it;
        for (it = vect.begin(); it != vect.end(); ++it)
          f << inc << it->toString() << "\n";

        //autocancel
        if (node->hasAutoCancel() && node->get_autocancel())
          f << inc << node->get_autocancel()->toString() << "\n";
      }
    }
    if(status() == STATUS_SUSPENDED)
      f << inc << "# " << type_name() << " " << this->name() << " is " << status_name()
        << "\n";
    }
  {
    std::vector<Variable> gvar;
    std::vector<Variable>::const_iterator it, gvar_end;
    ecf_node* prox = __node__();
    if (!prox) return;

         Defs* defs = 0;
         Node* ecf = 0;
         if (dynamic_cast<ecf_concrete_node<Node>*>(prox)) {
            ecf = dynamic_cast<ecf_concrete_node<Node>*>(prox)->get();
         }
         else if (dynamic_cast<ecf_concrete_node<Task>*>(prox)) {
            ecf = dynamic_cast<ecf_concrete_node<Task>*>(prox)->get();
         }
         else if (dynamic_cast<ecf_concrete_node<Family>*>(prox)) {
            ecf = dynamic_cast<ecf_concrete_node<Family>*>(prox)->get();
         }
         else if (dynamic_cast<ecf_concrete_node<Suite>*>(prox)) {
            ecf = dynamic_cast<ecf_concrete_node<Suite>*>(prox)->get();
         }
         else if (dynamic_cast<ecf_concrete_node<Defs>*>(prox)) {
            defs = dynamic_cast<ecf_concrete_node<Defs>*>(prox)->get();
         }
         if (!ecf && !defs) {
	   return;
         }

         if (ecf ) {
            gvar.clear();

	    if (ecf->hasTimeDependencies()) {
	      f << inc << "# time-date-dependencies: ";
	      if (ecf->isTimeFree()) f << "free\n";
	      else f << "holding\n";
	    }
            ecf->gen_variables(gvar);
            for(it = gvar.begin(); it != gvar.end(); ++it) {
	      f << inc << "# edit " << (*it).name() << " '" << (*it).theValue() << "'\n";
            }

            gvar = ecf->variables();
            for(it = gvar.begin(); it != gvar.end(); ++it) {
	      f << inc << "edit " << (*it).name() << " '" << (*it).theValue() << "'\n";
            }
         }
         if (defs) {
            const std::vector<Variable>& gvar = defs->server().user_variables();
            for(it = gvar.begin(); it != gvar.end(); ++it) {
	      f << inc << "# edit " << (*it).name() << " '" << (*it).theValue() << "'\n";
            }
            const std::vector<Variable>& var = defs->server().server_variables();
            for(it = var.begin(); it != var.end(); ++it) {
	      f << inc << "edit " << (*it).name() << " '" << (*it).theValue() << "'\n";
            }
         }}

  for (node *run=kids(); run; run=run->next())
    if (run->type() == NODE_VARIABLE) {

    } else {
      f << inc;
      int i = run->type();
      if (!owner_ || (i == NODE_SUITE || i == NODE_FAMILY ||
                      i == NODE_TASK  || i == NODE_ALIAS))
        f << run->type_name() << " ";
      f << run->toString() << "\n";
      // f << run->dump() << "\n";
    }
  f << "end" << type_name() << " # " << name() << "\n";
}


*/
