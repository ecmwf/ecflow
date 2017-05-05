#ifndef FAMILY_HPP_
#define FAMILY_HPP_

//============================================================================
// Name        : NodeTree.hpp
// Author      : Avi
// Revision    : $Revision: #37 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================

#include "NodeContainer.hpp"
class FamGenVariables;

class Family : public NodeContainer {
public:
   Family( const std::string& name ) : NodeContainer(name),fam_gen_variables_(NULL) {}
   Family() : fam_gen_variables_(NULL)  {}
   Family(const Family& rhs) : NodeContainer(rhs), fam_gen_variables_(NULL) {}
   Family& operator=(const Family&);

	virtual ~Family();

	static family_ptr create(const std::string& name);

	virtual Suite* suite() const { return parent()->suite(); }
	virtual Defs* defs() const { return (parent()) ? parent()->defs() : NULL;}     // exposed to python hence check for NULL first
 	virtual Family* isFamily() const { return const_cast<Family*>(this);}
   virtual NodeContainer* isNodeContainer() const { return const_cast<Family*>(this); }

   virtual void begin();
   virtual bool resolveDependencies(JobsParam& ); // overriden to speicy family for job profiler
   virtual void requeue(bool resetRepeats, int clear_suspended_in_child_nodes, bool reset_next_time_slot);
	virtual void accept(ecf::NodeTreeVisitor&);
	virtual void acceptVisitTraversor(ecf::NodeTreeVisitor& v);
	virtual void update_generated_variables() const;
	virtual const Variable& findGenVariable(const std::string& name) const;
	virtual void gen_variables(std::vector<Variable>&) const;

	virtual const std::string& debugType() const;

	std::ostream& print(std::ostream&) const;
	bool operator==(const Family& rhs) const;

	virtual void collateChanges(DefsDelta&) const;
   void set_memento(const OrderMemento* m,std::vector<ecf::Aspect::Type>& aspects,bool f) { NodeContainer::set_memento(m,aspects,f); }
   void set_memento(const ChildrenMemento* m,std::vector<ecf::Aspect::Type>& aspects,bool f) { NodeContainer::set_memento(m,aspects,f); }

   virtual void read_state(const std::string& line,const std::vector<std::string>& lineTokens);
private:
   virtual std::string write_state() const;

   mutable FamGenVariables* fam_gen_variables_;

private:
   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int /*version*/) {
      ar & boost::serialization::base_object<NodeContainer>(*this);
   }
};

std::ostream& operator<<(std::ostream& os, const Family&);


// We can have several thousands Families. This class helps in avoiding
// the creation of generated variables until required.
// This improves client->server down load times by avoiding thousands of string constructions
class FamGenVariables : private boost::noncopyable {
public:
   FamGenVariables(const Family*);

   void update_generated_variables() const;
   const Variable& findGenVariable(const std::string& name) const;
   void gen_variables(std::vector<Variable>& vec) const;

private:
   const Family* family_;
   mutable Variable genvar_family_;
   mutable Variable genvar_family1_;
};
#endif
