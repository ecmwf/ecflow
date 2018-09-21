#ifndef IN_LIMIT_MGR_HPP_
#define IN_LIMIT_MGR_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #16 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <iosfwd>

#include "Serialization.hpp"

#include "InLimit.hpp"
#include "LimitFwd.hpp"
#include "NodeFwd.hpp"

// class InLimitMgr:
// Design notes:
// Please note: when ever we want access the inlimits, limit ptrs we
// must resolve(/compute them first). This save on client code which
// modifies the node tree from having handle it.
// If this proves to be a bottle next. We could add a caching mechanism
// base on the Ecf class,so that we need only update the pointers
// when a structural modification is made.
//
class InLimitMgr {
public:
   explicit InLimitMgr(Node* n) : node_(n) {}
   InLimitMgr(const InLimitMgr& rhs) : node_(nullptr),vec_(rhs.vec_){}
	InLimitMgr()= default;

   // needed by node copy constructor
   void set_node(Node* n) { node_ = n; }

// standard functions: ==============================================
   InLimitMgr& operator=(const InLimitMgr&);
 	std::ostream& print(std::ostream&) const;
 	bool operator==(const InLimitMgr& rhs) const;
 	void clear() { vec_.clear(); }

// Access functions: ======================================================
	const std::vector<InLimit>&  inlimits()  const { return vec_; }

// Add functions: ===============================================================
	void addInLimit(const InLimit& );    // will throw std::runtime_error if duplicate

// Delete functions: can throw std::runtime_error ===================================
	// if name argument is empty, delete all attributes of that type
	// if delete was successful return true, else return false.
	// Can throw std::runtime_error if the attribute can not be found
	bool deleteInlimit(const std::string& name);

// mementos functions:
  	void get_memento(compound_memento_ptr& comp) const;

// Find functions: ============================================================
 	/// *** This will resolve the in limits first ***
  	/// Used in *test* only
   Limit* findLimitViaInLimit(const InLimit& ) const;

 	bool findInLimitByNameAndPath(const InLimit& ) const;  // use name,path,token,

// Why:
 	bool why(std::vector<std::string>& vec, bool top_down, bool html = false) const; // return true if why found

// Limit functions:

	/// Are the in limits pointers to the Limits in limit.
 	/// This is a very heavily used function. *******
 	/// *** This will resolve the in limits first ***
 	bool inLimit() const;

	/// After job submission we need to increment the in limit, to indicate that a
	/// resource is consumed.
 	/// *** This will resolve the in limits first ***
 	void incrementInLimit(
 	         std::set<Limit*>& limitSet,   // The set ensure we only update once
 	         const std::string& task_path  // The task that was submitted, and hence caused Limit to increment
 	);

	/// After job aborts or completes we need to decrement the in limit, to indicate that
	/// additional resource is available.
 	/// *** This will resolve the in limits first ***
 	void decrementInLimit(
 	         std::set<Limit*>& limitSet,  // The set ensure we only update once
 	         const std::string& task_path // The task that completed or aborted. Gives up the token
 	);

 	/// Check to see if inlimit's can reference their Limits
 	void check(std::string& errorMsg, std::string& warningMsg,bool reportErrors, bool reportWarnings) const;

 	/// Add externs where the inlimit reference to limits can not be resolved
   void auto_add_inlimit_externs(Defs*) const;

 	/// Needed by python interface
	std::vector<InLimit>::const_iterator inlimit_begin() const { return vec_.begin();}
	std::vector<InLimit>::const_iterator inlimit_end() const { return vec_.end();}

private:
	/// Setup in-limits, to point to their limits,
 	void resolveInLimitReferences() const;
 	void resolveInLimit(InLimit&,std::string& errorMsg, std::string& warningMsg,bool reportErrors, bool reportWarnings) const;
 	void resolveInLimit(InLimit&) const;

 	limit_ptr find_limit(const InLimit&, std::string& errorMsg, std::string& warningMsg,bool reportErrors, bool reportWarnings) const;

private:
 	Node* node_{nullptr}; // Not persisted, constructor will always set this up.

 	mutable std::vector<InLimit> vec_;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar)
   {
 	   ar(CEREAL_NVP(vec_));
 	}
};

#endif
