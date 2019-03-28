/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #15 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : Holds the zombie structure
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <boost/foreach.hpp>

#include "Zombie.hpp"
#include "Calendar.hpp"
#include "Serialization.hpp"

using namespace ecf;
using namespace std;

// support return by reference
const Zombie& Zombie::EMPTY() { static const Zombie ZOMBIE = Zombie(); return ZOMBIE; }
Zombie& Zombie::EMPTY_() { static Zombie ZOMBIE = Zombie(); return ZOMBIE; }

Zombie::Zombie( ecf::Child::ZombieType zombie_type,
                ecf::Child::CmdType cmd,
                const ZombieAttr& attr,
                const std::string& pathToTask,
                const std::string& jobsPassword,
                const std::string& process_or_remote_id,
                int try_no,
                const std::string& host,
                const std::string& user_cmd
)
: 
  try_no_(try_no),
  zombie_type_(zombie_type),
  last_child_cmd_(cmd),
  path_to_task_(pathToTask),
  jobs_password_(jobsPassword),
  process_or_remote_id_(process_or_remote_id),
  user_cmd_(user_cmd),
  host_(host),
  attr_(attr),
  creation_time_( Calendar::second_clock_time() )
{}

Zombie::Zombie() = default;

bool Zombie::operator==(const Zombie& rhs) const
{  // for python interface only
   if (zombie_type_ != rhs.zombie_type_) return false;
   if (user_action_ != rhs.user_action_) return false;
   if (try_no_ != rhs.try_no_) return false;
   if (duration_ != rhs.duration_) return false;
   if (calls_ != rhs.calls_) return false;
   if (last_child_cmd_ != rhs.last_child_cmd_) return false;
   if (path_to_task_ != rhs.path_to_task_) return false;
   if (jobs_password_ != rhs.jobs_password_) return false;
   if (process_or_remote_id_ != rhs.process_or_remote_id_) return false;
   if (user_cmd_ != rhs.user_cmd_) return false;
   if (host_ != rhs.host_) return false;
   if (user_action_set_ != rhs.user_action_set_) return false;
   if (!(attr_ == rhs.attr_)) return false;
   return true;
}

std::string Zombie::to_string() const
{  // for python interface only
   std::vector<Zombie> vec; vec.push_back(*this);
   return Zombie::pretty_print(vec,1);
}

std::string Zombie::type_str() const
{
	return Child::to_string(zombie_type_);
}

ecf::User::Action Zombie::user_action() const
{
   // User action needs to take into account, last child command and setting on attr_
   if (fob()) return User::FOB;
   else if (block()) return User::BLOCK;
   else if (fail()) return User::FAIL;
   else if (remove()) return User::REMOVE;
   else if (kill()) return User::KILL;
   else if (adopt()) return User::ADOPT;

   return User::BLOCK; // the default action
}

std::string Zombie::user_action_str() const {
   std::string ret;
   if (manual_user_action()) ret = "manual-";
   else                      ret = "auto-";
   ret += User::to_string(user_action());
   return ret;
}

/// accessors
bool Zombie::fob() const {
	if (user_action_set_) return user_action_ == User::FOB;
	return attr_.fob(last_child_cmd_);
}
bool Zombie::fail() const   {
	if (user_action_set_) return user_action_ == User::FAIL;
	return attr_.fail(last_child_cmd_);
}
bool Zombie::adopt() const  {
	if (user_action_set_) return user_action_ == User::ADOPT;
	return attr_.adopt(last_child_cmd_);
}
bool Zombie::block() const  {
	if (user_action_set_) return user_action_ == User::BLOCK;
	return attr_.block(last_child_cmd_);
}
bool Zombie::remove() const {
	if (user_action_set_) return user_action_ == User::REMOVE;
	return attr_.remove(last_child_cmd_);
}
bool Zombie::kill() const {
   if (user_action_set_) return user_action_ == User::KILL;
   return attr_.kill(last_child_cmd_);
}

int Zombie::allowed_age() const
{
	return attr_.zombie_lifetime();
}

void Zombie::set_fob()   { user_action_ = User::FOB;   user_action_set_ = true;}
void Zombie::set_fail()  { user_action_ = User::FAIL;  user_action_set_ = true;}
void Zombie::set_adopt() { user_action_ = User::ADOPT; user_action_set_ = true;}
void Zombie::set_block() { user_action_ = User::BLOCK; user_action_set_ = true;}
void Zombie::set_kill()  { user_action_ = User::KILL;  user_action_set_ = true;}


std::ostream& operator<<(std::ostream& os, const Zombie& z)
{
	os << z.path_to_task() << " ";
	os << z.type_str() << " ";
 	os << z.duration() << " ";
	os << z.jobs_password() << " ";
	os << z.process_or_remote_id() << "<pid> ";
 	os << z.try_no() << " ";
 	os << "calls(" << z.calls() << ") ";
 	os << z.user_action_str();
	os << " ";
	os << Child::to_string(z.last_child_cmd());
	return os;
}

std::string Zombie::explanation() const
{
   std::string ecf_pid_expl =        "PID miss-match, password matches. Job scheduled twice. Check submitter";
   std::string ecf_pid_passwd_expl = "Both PID and password miss-match. Re-queue & submit of active job?";
   std::string ecf_passwd_expl =     "Password miss-match, PID matches, system has re-cycled PID or hacked job file?";
   std::string ecf_expl =            "Two init commands or task complete or aborted but receives another child cmd";
   std::string ecf_user =            "Created by user action(";
   std::string ecf_path =            "Task not found. Nodes replaced whilst jobs were running";
   std::string exp;

   switch (zombie_type_) {
         case Child::USER:           exp = ecf_user; exp += user_cmd_; exp += ")"; break;
         case Child::PATH:           exp = ecf_path ; break;
         case Child::ECF:            exp = ecf_expl ; break;
         case Child::ECF_PID:        exp = ecf_pid_expl ; break;
         case Child::ECF_PID_PASSWD: exp = ecf_pid_passwd_expl ; break;
         case Child::ECF_PASSWD:     exp = ecf_passwd_expl ; break;
         case Child::NOT_SET:        break;
   }

   return exp;
}


std::string Zombie::pretty_print(const std::vector<Zombie>& zombies,int indent)
{
  std::stringstream ss;
  std::vector<std::string> list;
  pretty_print(zombies, list, indent);
  for (const auto & i : list) {
    ss << i << "\n";
  }
  return ss.str();
}


void Zombie::pretty_print(const std::vector<Zombie>& zombies,
			  std::vector<std::string>& list,
			  int indent)
{
   string path("task-path");
   string type("type");
   string password("password");
   string rid("pid");
   string duration("age(s)");
   string calls("calls");
   string try_no("try_no");
   string user_action("action");
   string child_type("child");
   string host("host");
   string explanation("explanation");

   size_t path_width = path.size();
   size_t type_width = type.size();
   size_t duration_width = duration.size();
   size_t password_width = password.size();
   size_t tryno_width = try_no.size();
   size_t rid_width = rid.size();
   size_t user_action_width = user_action.size();  // max of FOB,FAIL,ADOPT,BLOCK,REMOVE + (manual- | auto- )
   size_t child_type_width = child_type.size();
   size_t calls_width = calls.size();
   size_t host_width = host.size();
   size_t explanation_width = explanation.size();

   std::string ecf_pid_expl =        "PID miss-match, password matches. Job scheduled twice. Check submitter";
   std::string ecf_pid_passwd_expl = "Both PID and password miss-match. Re-queue & submit of active job?";
   std::string ecf_passwd_expl =     "Password miss-match, PID matches, system has re-cycled PID or hacked job file?";
   std::string ecf_expl =            "Two init commands or task complete or aborted but receives another child cmd";
   std::string ecf_user =            "Created by user action(";
   std::string ecf_path =            "Task not found. Nodes replaced whilst jobs were running";
   std::string exp;

   BOOST_FOREACH(const Zombie& z, zombies) {
      path_width = std::max(path_width,z.path_to_task().size());
      type_width = std::max(type_width,z.type_str().size());
      password_width = std::max(password_width,z.jobs_password().size());
      rid_width = std::max(rid_width,z.process_or_remote_id().size());
      std::string no_of_calls = boost::lexical_cast<std::string>(z.calls());
      calls_width = std::max(calls_width,no_of_calls.size());
      host_width = std::max(host_width,z.host().size());

      std::string try_no_int = boost::lexical_cast<std::string>(z.try_no());
      tryno_width = std::max(tryno_width,try_no_int.size());
      child_type_width = std::max(child_type_width,Child::to_string(z.last_child_cmd()).size());
      user_action_width = std::max(user_action_width,z.user_action_str().size());

      switch (z.type()) {
         case Child::USER:           exp = ecf_user; exp += z.user_cmd(); exp += ")"; break;
         case Child::PATH:           exp = ecf_path ; break;
         case Child::ECF:            exp = ecf_expl ; break;
         case Child::ECF_PID:        exp = ecf_pid_expl ; break;
         case Child::ECF_PID_PASSWD: exp = ecf_pid_passwd_expl ; break;
         case Child::ECF_PASSWD:     exp = ecf_passwd_expl ; break;
         case Child::NOT_SET:        break;
      }
      explanation_width = std::max(explanation_width,exp.size());
   }

   std::stringstream ss;
   if (indent != 0) for(int i = 0; i < indent; i++) ss << " ";
   ss << left << setw(path_width) << path << " "
            << setw(type_width) << type << " "
            << setw(duration_width) << duration << " "
            << setw(password_width) << password << " "
            << setw(rid_width) << rid << " "
            << setw(tryno_width) << try_no << " "
            << setw(user_action_width) << user_action << " "
            << setw(child_type_width) << child_type << " "
            << setw(calls_width) << calls << " "
            << setw(host_width) << host << " "
            << setw(explanation_width) << explanation
   ;

   list.push_back(ss.str());

   BOOST_FOREACH(const Zombie& z, zombies) {
     std::stringstream ss;
     if (indent != 0) for(int i = 0; i < indent; i++) ss << " ";

      switch (z.type()) {
         case Child::USER:           exp = ecf_user; exp += z.user_cmd(); exp += ")"; break;
         case Child::PATH:           exp = ecf_path ; break;
         case Child::ECF:            exp = ecf_expl ; break;
         case Child::ECF_PID:        exp = ecf_pid_expl ; break;
         case Child::ECF_PID_PASSWD: exp = ecf_pid_passwd_expl ; break;
         case Child::ECF_PASSWD:     exp = ecf_passwd_expl ; break;
         case Child::NOT_SET:        break;
      }

      ss << left << setw(path_width)         << z.path_to_task() << " "
                << setw(type_width)          << z.type_str() << " "
                << setw(duration_width)      << z.duration() << " "
                << setw(password_width)      << z.jobs_password() << " "
                << setw(rid_width)           << z.process_or_remote_id() << " "
                << setw(tryno_width)         << z.try_no() << " "
                << setw(user_action_width)   << z.user_action_str() << " "
                << setw(child_type_width)    << Child::to_string(z.last_child_cmd())  << " "
                << setw(calls_width)         << z.calls() << " "
                << setw(host_width)          << z.host() << " "
                << setw(explanation_width)   << exp ;
      list.push_back(ss.str());
   }
}


template<class Archive>
void Zombie::serialize(Archive & ar, std::uint32_t const version )
{
   ar( CEREAL_NVP(user_action_),
       CEREAL_NVP(try_no_),
       CEREAL_NVP(duration_),
       CEREAL_NVP(calls_),
       CEREAL_NVP(zombie_type_),
       CEREAL_NVP(last_child_cmd_),
       CEREAL_NVP(path_to_task_),
       CEREAL_NVP(jobs_password_),
       CEREAL_NVP(process_or_remote_id_),
       CEREAL_NVP(user_cmd_),
       CEREAL_NVP(host_),
       CEREAL_NVP(user_action_set_),
       CEREAL_NVP(attr_)
   );
}
CEREAL_TEMPLATE_SPECIALIZE_V(Zombie);
