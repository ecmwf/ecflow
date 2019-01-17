#ifndef ALIAS_HPP_
#define ALIAS_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #17 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================
#include "Submittable.hpp"

class Alias : public Submittable {
public:
   explicit Alias(const std::string& name);
   Alias();
   Alias(const Alias&);
   Alias& operator=(const Alias&);

   virtual ~Alias();

   static alias_ptr create(const std::string& name);

   std::ostream& print(std::ostream&) const;
   bool operator==(const Alias& rhs) const;

   /// Overridden to reset the try number
   /// The tasks job can be invoked multiple times. For each invocation we want to preserve
   /// the output. The try number is used in SMSJOB/SMSJOBOUT to preserve the output when
   /// there are multiple runs.  re-queue/begin() resets the try Number
   virtual void begin();
   virtual void requeue(Requeue_args&);

   virtual Suite* suite() const { return parent()->suite(); }
   virtual Defs* defs() const { return (parent()) ? parent()->defs() : NULL;} // exposed to python hence check for NULL first
   virtual Alias* isAlias() const   { return const_cast<Alias*>(this);}
   virtual Submittable* isSubmittable() const { return const_cast<Alias*>(this); }

   virtual const std::string& debugType() const;

   virtual node_ptr removeChild( Node* child);
   virtual bool addChild( node_ptr child, size_t position = std::numeric_limits<std::size_t>::max());
   virtual bool isAddChildOk( Node* child, std::string& errorMsg) const;

   virtual const std::string& script_extension() const;

   virtual void collateChanges(DefsDelta&) const;
   void set_memento(const SubmittableMemento* m,std::vector<ecf::Aspect::Type>& aspects,bool f) { Submittable::set_memento(m,aspects,f); }

   virtual node_ptr find_node_up_the_tree(const std::string& name) const;

   // Pure node Functions that are not implemented for aliases
   virtual node_ptr find_relative_node(const std::vector<std::string>&) {return node_ptr();}

   virtual void get_all_nodes(std::vector<node_ptr>& nodes) const;

// Functions unique to aliases
   // Alias variable names by pass checking of valid names, allowing anything
   void add_alias_variable(const std::string& name, const std::string& value);

private:
   virtual size_t child_position(const Node*) const;

   /// Job creation checking is typically called from python API
   /// This has been disabled for Aliases
   virtual void check_job_creation( job_creation_ctrl_ptr) {}

   // Overridden from Node to increment/decrement limits only
   // i.e we do not update parent computed states for aliases
   virtual void handleStateChange();

   // Pure node Functions that are not implemented for aliases
   virtual void accept(ecf::NodeTreeVisitor&){}
   virtual void acceptVisitTraversor(ecf::NodeTreeVisitor&){}
   virtual void get_all_tasks(std::vector<task_ptr>&) const {}
   virtual void get_all_aliases(std::vector<alias_ptr>&) const {}
   virtual void getAllNodes(std::vector<Node*>&) const {}
   virtual void getAllTasks(std::vector<Task*>&) const {}
   virtual void getAllSubmittables(std::vector<Submittable*>&) const {}
   virtual void get_all_active_submittables(std::vector<Submittable*>&) const {}

   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int /*version*/) {
      ar & boost::serialization::base_object<Submittable>(*this); // Serialise base class information
   }
};

std::ostream& operator<<(std::ostream& os, const Alias&);

#endif
