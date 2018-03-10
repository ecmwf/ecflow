#ifndef AUTO_ATTRS_HPP_
#define AUTO_ATTRS_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #235 $
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
#include <ostream>

#include <boost/noncopyable.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>

#include "NodeFwd.hpp"
#include "Calendar.hpp"

class AutoAttrs : private boost::noncopyable {
public:
   AutoAttrs(Node* node) : node_(node),auto_cancel_(NULL),auto_archive_(NULL),auto_restore_(NULL) {}
   AutoAttrs(const AutoAttrs& rhs);
   AutoAttrs() : node_(NULL),auto_cancel_(NULL),auto_archive_(NULL),auto_restore_(NULL) {}
   ~AutoAttrs();

   // needed by node serialisation
   void set_node(Node* n);

   // standard functions: ==============================================
   std::ostream& print(std::ostream&) const;
   bool operator==(const AutoAttrs& ) const;

   // Auto =====================================================================================
   ecf::AutoRestoreAttr* get_autorestore() const { return auto_restore_;}
   ecf::AutoCancelAttr*  get_autocancel() const { return auto_cancel_;}
   ecf::AutoArchiveAttr* get_autoarchive() const { return auto_archive_;}

   void add_autorestore( const ecf::AutoRestoreAttr& ); // will throw std::runtime_error for errors
   void add_autocancel( const ecf::AutoCancelAttr& );   // will throw std::runtime_error for errors
   void add_autoarchive( const ecf::AutoArchiveAttr& ); // will throw std::runtime_error for errors

   void delete_autorestore();
   void delete_autocancel();
   void delete_autoarchive();

   void do_autorestore();

   bool checkForAutoCancel(const ecf::Calendar& calendar) const;
   bool check_for_auto_archive(const ecf::Calendar& calendar) const;
   bool has_auto_cancel() const {return (auto_cancel_) ? true : false;} // simulator function

   /// Check to see if auto_restore can reference the nodes
   void check(std::string& errorMsg) const;

private:
   Node*        node_; // *NOT* persisted must be set by the parent class
   friend class Node;

private:
   ecf::AutoCancelAttr*    auto_cancel_;  // Can only have 1 auto cancel per node
   ecf::AutoArchiveAttr*   auto_archive_; // Can only have 1 auto archive per node
   ecf::AutoRestoreAttr*   auto_restore_; // Can only have 1 autorestore per node

private:
   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int /*version*/) {
      ar & auto_cancel_;
      ar & auto_archive_;
      ar & auto_restore_;
   }
};
#endif
