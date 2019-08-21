#ifndef SUBMITTABLE_HPP_
#define SUBMITTABLE_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #16 $ 
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
#include "Node.hpp"

class EcfFile;
class SubGenVariables;

class Submittable : public Node {
protected:
   Submittable( const std::string& name, bool check) : Node(name,check) {}
   Submittable()= default;
   Submittable(const Submittable& rhs)
   : Node(rhs),
     paswd_(rhs.paswd_),
     rid_(rhs.rid_),
     abr_(rhs.abr_),
     tryNo_(rhs.tryNo_),
     state_change_no_(0) {}

   Submittable& operator=(const Submittable&);

   bool operator==(const Submittable& rhs) const;

public:
   ~Submittable() override;
   bool check_defaults() const override;

   /// Initialise the task. will set the state to NState::ACTIVE
   void init(const std::string& processId);

   /// complete the task. Will set the state to NState::COMPLETE
   /// However if there is a valid Repeat or time dependencies then
   /// task will be re-queued afterwards
   void complete();

   /// The late attribute, ONLY applies to the Submittable and not NodeContainer
   void calendarChanged(const ecf::Calendar&, Node::Calendar_args&,const ecf::LateAttr* inherited_late) override;

   /// Overridden to reset the try number
   /// The tasks job can be invoked multiple times. For each invocation we want to preserve
   /// the output. The try number is used in SMSJOB/SMSJOBOUT to preserve the output when
   /// there are multiple runs.  re-queue/begin() resets the try Number
   void reset() override;
   void begin() override;
   void requeue(Requeue_args&) override;
   bool run(JobsParam& jobsParam, bool force) override;
   void kill(const std::string& zombie_pid = "") override;
   void status() override;

   void update_generated_variables() const override;
   const Variable& findGenVariable(const std::string& name) const override;
   void gen_variables(std::vector<Variable>& vec) const override;


   static const std::string& DUMMY_JOBS_PASSWORD();
   static const std::string& FREE_JOBS_PASSWORD();
   static const std::string& DUMMY_PROCESS_OR_REMOTE_ID();

   // returns the corresponding .ecf file.
   // i.e if the ecf file is missing or empty std::runtime_error is thrown
   // For dummy tasks, i.e tasks with no .ecf file user should add ECF_DUMMY_TASK variable
   EcfFile locatedEcfFile() const;
   virtual const std::string& script_extension() const = 0;

   /// Spawn of a child process. Clear process and remote id before jobs is spawned
   /// Will increment try no first, and then update generated varaibles
   bool submitJob( JobsParam& ) ;

   /// generates job file independent of dependencies, resets the try Number
   void check_job_creation( job_creation_ctrl_ptr jobCtrl) override;

   /// See Defs.hpp
   void generate_scripts( const std::map<std::string,std::string>&) const override {}

   NState::State computedState(Node::TraverseType) const override { return state();}

/// data members accessor's and mutators:
   /// return the current try number as a string, and int
   std::string tryNo() const;
   int try_no() const { return tryNo_;}

   const std::string& jobsPassword() const { return paswd_;}

   /// The remote id (ECF_RID) allows a jobs to be killed when added via a queueing system
   /// in which case the remote id is really the queueing id.
   const std::string& process_or_remote_id() const { return rid_;}

   /// Set the task to aborted, providing a reason. Will set the state to NState::ABORTED
   /// This should only be called in two context's:
   ///    1/ Called via Child Cmd (AbortCmd) i.e job raised a trap,
   ///    2/ Abnormal process termination. i.e job killed by signal
   /// *IMPORTANT* If task try number is less than ECF_TRIES then we should
   /// resubmit the job. However we *should* not do this immediately here, instead we
   /// wait of *next* call to resolveDependencies, as that will check if we are *inlimit*
   void aborted(const std::string& reason);
   const std::string& abortedReason() const override { return abr_;}

// Memento functions:
   void incremental_changes(DefsDelta&, compound_memento_ptr& comp) const;
   void set_memento(const SubmittableMemento*,std::vector<ecf::Aspect::Type>& aspects,bool f);

   void read_state(const std::string& line,const std::vector<std::string>& lineTokens) override;
protected:

   void write_state(std::string&, bool&) const override;

   /// call just before job submission, reset data members, update try_no, and generate variable
   void increment_try_no(); // will increment state_change_no

   /// Submits the job *WITHOUT* incrementing the try number
   bool submit_job_only( JobsParam& );

   // Overridden from Node to increment/decrement limits
   void update_limits() override;

private:
   friend class ZombieCtrl;
   friend class AlterCmd;

   void set_jobs_password(const std::string& p);
   void set_process_or_remote_id(const std::string&);

   // Use when we _only_ want to set the state,
   void set_aborted_only(const std::string& reason);
   bool createChildProcess(JobsParam& jobsParam);
   void clear(); // process_id password and aborted reason

   bool script_based_job_submission(JobsParam& jobsParam);
   bool non_script_based_job_submission(JobsParam& jobsParam);

   void update_static_generated_variables(const std::string& ecf_home, const std::string& theAbsNodePath) const;
   const Variable& get_genvar_ecfrid() const;
   const Variable& get_genvar_ecfscript() const;
   void set_genvar_ecfjob(const std::string& value);
   void set_genvar_ecfrid(const std::string& value);

private:
   std::string         paswd_;
   std::string         rid_;
   std::string         abr_;
   int                 tryNo_{0};
   unsigned int state_change_no_{0};               // *not* persisted, only used on server side
   mutable SubGenVariables* sub_gen_variables_{nullptr}; // *not* persisted since they can be generated
   friend class SubGenVariables;

private:
   // never change defaults !! or will mess up client/server protocol
   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version );
};

// We can have several thousands Submittables. This class helps in avoiding
// the creation of generated variables until required.
// This improves client->server down load times by avoiding thousands of string constructions
class SubGenVariables {
private:
  SubGenVariables(const SubGenVariables&) = delete;
  const SubGenVariables& operator=(const SubGenVariables&) = delete;
public:
   explicit SubGenVariables(const Submittable*);

   void update_generated_variables() const;

   /// distinguish between the two kinds of generated variables
   void update_static_generated_variables(const std::string& ecf_home, const std::string& theAbsNodePath) const;
   void update_dynamic_generated_variables(const std::string& ecf_home,const std::string& theAbsNodePath) const;

   const Variable& findGenVariable(const std::string& name) const;
   void gen_variables(std::vector<Variable>& vec) const;

   const Variable& genvar_ecfrid() const  { return genvar_ecfrid_;}
   const Variable& genvar_ecfscript() const  { return genvar_ecfscript_;}

   void set_genvar_ecfjob(const std::string& value) { genvar_ecfjob_.set_value(value); }
   void set_genvar_ecfrid(const std::string& value) { genvar_ecfrid_.set_value(value); }

private:
   const Submittable* submittable_;
   mutable Variable genvar_ecfjob_;
   mutable Variable genvar_ecfjobout_;
   mutable Variable genvar_ecftryno_;
   mutable Variable genvar_task_;
   mutable Variable genvar_ecfpass_;
   mutable Variable genvar_ecfscript_;
   mutable Variable genvar_ecfname_;
   mutable Variable genvar_ecfrid_;
};

#endif
