#ifndef CHILD_ATTRS_HPP_
#define CHILD_ATTRS_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #234 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <ostream>
#include <vector>

#include <boost/noncopyable.hpp>

#include "NodeAttr.hpp"
#include "NodeFwd.hpp"
#include "Aspect.hpp"
#include "Attr.hpp"
#include "Serialization.hpp"

class ChildAttrs :  private boost::noncopyable {
public:
   explicit ChildAttrs(Node* node) : node_(node) {}
   ChildAttrs(const ChildAttrs&); // users must call set_node() afterwards
   ChildAttrs() : node_(NULL) {}
   ~ChildAttrs() {}

   // needed by node serialisation
   void set_node(Node* n) { node_ = n; }
   bool checkInvariants(std::string& errorMsg) const;

   void begin();
   void requeue();
   void requeue_labels();
   void sort_attributes( ecf::Attr::Type at);

   // standard functions: ==============================================
   std::ostream& print(std::ostream&) const;
   bool operator==(const ChildAttrs& rhs) const;

// state related functions: ========================================

   // Access functions: ======================================================
   bool empty() const { return (meters_.empty() && events_.empty() && labels_.empty()) ? true : false; }
   const std::vector<Meter>&           meters()    const { return meters_;}
   const std::vector<Event>&           events()    const { return events_;}
   const std::vector<Label>&           labels()   const { return labels_;}

   // Add functions: ===============================================================
   void addEvent( const Event& );       // will throw std::runtime_error if duplicate
   void addMeter( const Meter& );       // will throw std::runtime_error if duplicate
   void addLabel( const Label& );       // will throw std::runtime_error if duplicate

   // Delete functions: can throw std::runtime_error ===================================
   // if name argument is empty, delete all attributes of that type
   // Can throw std::runtime_error of the attribute can not be found
   void deleteEvent(const std::string& name);
   void deleteMeter(const std::string& name);
   void deleteLabel(const std::string& name);

   // Change functions: ================================================================
   /// returns true the change was made else false, Can throw std::runtime_error for parse errors
   void changeEvent(const std::string& name,const std::string& setOrClear = "");
   void changeEvent(const std::string& name,bool value);
   void changeMeter(const std::string& name,const std::string& value);
   void changeMeter(const std::string& name,int value);
   void changeLabel(const std::string& name,const std::string& value);

   bool set_meter(const std::string& name,int value); // does not throw if meter not found
   bool set_event(const std::string& name,bool value);  // does not throw if event not found

   // Used in the force cmd
   bool set_event( const std::string& event_name_or_number);
   bool clear_event( const std::string& event_name_or_number);

   // mementos functions:
   void set_memento(const NodeEventMemento*);
   void set_memento(const NodeMeterMemento*);
   void set_memento(const NodeLabelMemento*);

   // Find functions: ============================================================
   bool findLabel(const std::string& name) const;
   const Label& find_label(const std::string& name) const;
   const Meter& findMeter(const std::string& name) const;
   Meter& find_meter(const std::string& name);
   const Event& findEvent(const Event&) const;
   const Event& findEventByNameOrNumber( const std::string& name) const;
   bool getLabelValue(const std::string& labelName, std::string& value) const;
   bool getLabelNewValue(const std::string& labelName, std::string& value) const;

private:

   const Event& findEventByNumber(int number) const;
   const Event& findEventByName( const std::string& name) const;
   bool set_meter_used_in_trigger(const std::string& name);
   bool set_event_used_in_trigger(const std::string& name);

private: // allow simulator access
   std::vector<Meter>&  ref_meters() { return meters_;} // allow simulator set meter value
   std::vector<Event>&  ref_events() { return events_;} // allow simulator set event value

private: /// For use by python interface,
   friend class Node;
   std::vector<Meter>::const_iterator meter_begin() const { return meters_.begin();}
   std::vector<Meter>::const_iterator meter_end() const { return meters_.end();}
   std::vector<Event>::const_iterator event_begin() const { return events_.begin();}
   std::vector<Event>::const_iterator event_end() const { return events_.end();}
   std::vector<Label>::const_iterator label_begin() const { return labels_.begin();}
   std::vector<Label>::const_iterator label_end() const { return labels_.end();}

private:
   std::vector<Meter>          meters_;
   std::vector<Event>          events_;
   std::vector<Label>          labels_;

private:
   Node*        node_; // *NOT* persisted must be set by the parent class

private:
   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(CEREAL_NVP(meters_),
         CEREAL_NVP(events_),
         CEREAL_NVP(labels_));
   }
};
#endif
