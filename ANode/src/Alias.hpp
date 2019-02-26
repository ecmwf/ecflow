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
   node_ptr clone() const override;

   ~Alias() override;

   static alias_ptr create(const std::string& name);

   void print(std::string&) const override;
   bool operator==(const Alias& rhs) const;

   /// Overridden to reset the try number
   /// The tasks job can be invoked multiple times. For each invocation we want to preserve
   /// the output. The try number is used in SMSJOB/SMSJOBOUT to preserve the output when
   /// there are multiple runs.  re-queue/begin() resets the try Number
   void begin() override;
   void requeue(Requeue_args&) override;

   Suite* suite() const override { return parent()->suite(); }
   Defs* defs() const override { return (parent()) ? parent()->defs() : nullptr;} // exposed to python hence check for NULL first
   Alias* isAlias() const override   { return const_cast<Alias*>(this);}
   Submittable* isSubmittable() const override { return const_cast<Alias*>(this); }

   const std::string& debugType() const override;

   node_ptr removeChild( Node* child) override;
   bool addChild( const node_ptr& child, size_t position = std::numeric_limits<std::size_t>::max()) override;
   bool isAddChildOk( Node* child, std::string& errorMsg) const override;

   const std::string& script_extension() const override;

   void collateChanges(DefsDelta&) const override;
   void set_memento(const SubmittableMemento* m,std::vector<ecf::Aspect::Type>& aspects,bool f) { Submittable::set_memento(m,aspects,f); }

   node_ptr find_node_up_the_tree(const std::string& name) const override;

   // Pure node Functions that are not implemented for aliases
   node_ptr find_relative_node(const std::vector<std::string>&) override {return node_ptr();}

   void get_all_nodes(std::vector<node_ptr>& nodes) const override;

// Functions unique to aliases
   // Alias variable names by pass checking of valid names, allowing anything
   void add_alias_variable(const std::string& name, const std::string& value);

private:
   size_t child_position(const Node*) const override;

   /// Job creation checking is typically called from python API
   /// This has been disabled for Aliases
   void check_job_creation( job_creation_ctrl_ptr) override {}

   // Overridden from Node to increment/decrement limits only
   // i.e we do not update parent computed states for aliases
   void handleStateChange() override;

   // Pure node Functions that are not implemented for aliases
   void accept(ecf::NodeTreeVisitor&) override{}
   void acceptVisitTraversor(ecf::NodeTreeVisitor&) override{}
   void get_all_tasks(std::vector<task_ptr>&) const override {}
   void get_all_aliases(std::vector<alias_ptr>&) const override {}
   void getAllNodes(std::vector<Node*>&) const override {}
   void getAllTasks(std::vector<Task*>&) const override {}
   void getAllSubmittables(std::vector<Submittable*>&) const override {}
   void get_all_active_submittables(std::vector<Submittable*>&) const override {}

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version );
};

std::ostream& operator<<(std::ostream& os, const Alias&);

#endif
