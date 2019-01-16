#ifndef MISC_ATTRS_HPP_
#define MISC_ATTRS_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #235 $
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <iosfwd>

#include <boost/noncopyable.hpp>

#include "ZombieAttr.hpp"
#include "VerifyAttr.hpp"
#include "QueueAttr.hpp"
#include "GenericAttr.hpp"
#include "NodeFwd.hpp"

class MiscAttrs : private boost::noncopyable {
public:
   explicit MiscAttrs(Node* node) : node_(node){}
   MiscAttrs(const MiscAttrs& rhs);
   MiscAttrs()= default;
   ~MiscAttrs();

   // needed by node serialisation
   void set_node(Node* n);
   bool checkInvariants(std::string& errorMsg) const;

   void begin();
   void requeue();

   // standard functions: ==============================================
   std::ostream& print(std::ostream&) const;
   bool operator==(const MiscAttrs& ) const;

   // Access functions: ======================================================
   const std::vector<VerifyAttr>&      verifys() const { return verifys_;}
   const std::vector<ZombieAttr>&      zombies() const { return zombies_; }
   const std::vector<QueueAttr>&       queues()  const { return queues_; }
   const std::vector<GenericAttr>&     generics() const { return generics_; }
   std::vector<QueueAttr>&             ref_queues()    { return queues_; } //allow simulator access

   // Add functions: ===============================================================
   void addVerify( const VerifyAttr& );                  // for testing and verification Can throw std::runtime_error
   void addZombie( const ZombieAttr& );                  // will throw std::runtime_error if duplicate
   void add_queue( const QueueAttr& );                   // will throw std::runtime_error if duplicate
   void add_generic( const GenericAttr& );               // will throw std::runtime_error if duplicate, must be unique on the key

   /// Check to see if auto_restore can reference the nodes
   void check(std::string& errorMsg) const;

   // Delete functions: can throw std::runtime_error ===================================
   // if name argument is empty, delete all attributes of that type
   // Can throw std::runtime_error of the attribute can not be found
   void delete_zombie(const ecf::Child::ZombieType);
   void deleteZombie(const std::string& type); // string must be one of [ user | ecf | path ]
   void delete_queue(const std::string& name); // empty string means delete all queue's
   void delete_generic(const std::string& name); // empty string means delete all generics

   // Find functions: ============================================================
   bool findVerify(const VerifyAttr& ) const;
   const ZombieAttr& findZombie( ecf::Child::ZombieType ) const;
   const QueueAttr& find_queue( const std::string& name ) const;
   const GenericAttr& find_generic( const std::string& name ) const;
   QueueAttr& findQueue(const std::string& name);

   void verification(std::string& errorMsg) const;

   void set_memento(const NodeGenericMemento* );
   void set_memento(const NodeQueueMemento* );
   void set_memento(const NodeQueueIndexMemento*);

   bool empty() const { return (zombies_.empty() && verifys_.empty() && queues_.empty() && generics_.empty()) ? true : false;}
private:
   std::vector<ZombieAttr>::const_iterator zombie_begin() const { return zombies_.begin();}
   std::vector<ZombieAttr>::const_iterator zombie_end() const { return zombies_.end();}
   std::vector<VerifyAttr>::const_iterator verify_begin() const { return verifys_.begin();}
   std::vector<VerifyAttr>::const_iterator verify_end() const { return verifys_.end();}
   std::vector<QueueAttr>::const_iterator queue_begin() const { return queues_.begin();}
   std::vector<QueueAttr>::const_iterator queue_end() const { return queues_.end();}
   std::vector<GenericAttr>::const_iterator generic_begin() const { return generics_.begin();}
   std::vector<GenericAttr>::const_iterator generic_end() const { return  generics_.end();}

private:
   Node*        node_{nullptr}; // *NOT* persisted must be set by the parent class
   friend class Node;

private:
   std::vector<ZombieAttr> zombies_;      // can be added/removed via AlterCmd
   std::vector<VerifyAttr> verifys_;      // used for statistics and test verification
   std::vector<QueueAttr>  queues_;       // experimental
   std::vector<GenericAttr> generics_;    // experimental

private:
   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version );
};
#endif
