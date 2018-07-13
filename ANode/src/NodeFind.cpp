/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #53 $ 
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

#include "Node.hpp"
#include "Str.hpp"
#include "NodePath.hpp"
#include "Defs.hpp"
#include "Log.hpp"

using namespace ecf;
using namespace std;
using namespace boost;

/// Output a vector to cout
template<class T>
ostream& operator<<(ostream& os, const vector<T>& v)
{
   copy(v.begin(), v.end(), ostream_iterator<T>(cout, " "));
   return os;
}

bool Node::findParentVariableValue(const std::string& name, std::string& theValue) const
{
   if (!vars_.empty() && findVariableValue(name,theValue)) return true;
   if (!repeat_.empty() && repeat_.name() == name) {
      theValue = repeat_.valueAsString(); return true;
   }
   if (findGenVariableValue(name,theValue)) return true;


   Node* theParent =  parent();
   while (theParent) {

      if (theParent->findVariableValue(name,theValue)) return true;
      const Repeat& rep = theParent->repeat();
      if (!rep.empty() && rep.name() == name) {
         theValue = rep.valueAsString(); return true;
      }
      if (theParent->findGenVariableValue(name,theValue)) return true;

      theParent =  theParent->parent();
   }

   // If all else fails search defs environment, returns empty string if match not found
   // The defs environment is constructed via:
   //   o/ default settings for ECF_HOME,ECF_LOG, ECF_CHECK,ECF_CHECKOLD,ECF_CHECKINTERVAL
   //                           ECF_INTERVAL ECF_CHECKMODE ECF_JOB_CMD ECF_MICRO ECF_TRIES ECF_PORT, ECF_HOST
   //   o/ These values are updated from the server environment when the BEGIN cmd is called.
   Defs* the_defs = defs();
   if ( the_defs ) {
      theValue = the_defs->server().find_variable(name);
      if ( !theValue.empty() ) return true;
   }
   return false; // the variable can not be found
}

bool Node::find_parent_gen_variable_value(const std::string& name, std::string& theValue) const
{
    if (findGenVariableValue(name,theValue)) return true;

    Node* theParent =  parent();
    while (theParent) {
       if (theParent->findGenVariableValue(name,theValue)) return true;
       theParent =  theParent->parent();
    }

    // If all else fails search defs environment, returns empty string if match not found
    // The defs environment is constructed via:
    //   o/ default settings for ECF_HOME,ECF_LOG, ECF_CHECK,ECF_CHECKOLD,ECF_CHECKINTERVAL
    //                           ECF_INTERVAL ECF_CHECKMODE ECF_JOB_CMD ECF_MICRO ECF_TRIES ECF_PORT, ECF_HOST
    //   o/ These values are updated from the server environment when the BEGIN cmd is called.
    Defs* the_defs = defs();
    if ( the_defs ) {
       theValue = the_defs->server().find_variable(name);
       if ( !theValue.empty() ) return true;
    }
    return false; // the variable can not be found
}

bool Node::findParentUserVariableValue(const std::string& name, std::string& theValue) const
{
   if (findVariableValue(name,theValue)) return true;

   Node* theParent =  parent();
   while (theParent) {
      if (theParent->findVariableValue(name,theValue)) return true;
      theParent =  theParent->parent();
   }

   // If all else fails search defs environment, returns empty string if match not found
   Defs* the_defs = defs();
   if ( the_defs ) {
      // Note: when calling ecflow_client --get_state=/suite/task
      // The node can be detached from the defs.
      theValue = the_defs->server().find_variable(name);
      if ( !theValue.empty() ) return true;
   }
   return false; // the variable can not be found
}

const std::string& Node::find_parent_user_variable_value(const std::string& name) const
{
   const Variable& var = findVariable(name);
   if (!var.empty()) return var.theValue();

   Node* theParent = parent();
   while (theParent) {
      const Variable& pvar = theParent->findVariable(name);
      if (!pvar.empty()) return pvar.theValue();
      theParent = theParent->parent();
   }

   Defs* the_defs = defs();
   if ( the_defs ) {
      // Note: when calling ecflow_client --get_state=/suite/task
      // The node can be detached from the defs.
      return the_defs->server().find_variable(name);
   }
   return Str::EMPTY();
}

bool Node::user_variable_exists(const std::string& name) const
{
   const Variable& var = findVariable(name);
   if (!var.empty()) return true;

   Node* theParent = parent();
   while (theParent) {
      const Variable& pvar = theParent->findVariable(name);
      if (!pvar.empty()) return true;
      theParent = theParent->parent();
   }

   // If all else fails search defs environment, returns empty string if match not found
   Defs* the_defs = defs();
   if ( the_defs ) {
      // Note: when calling ecflow_client --get_state=/suite/task
      // The node can be detached from the defs.
      return the_defs->server().variable_exists(name);
   }
   return false;
}

const Variable& Node::findVariable(const std::string& name) const
{
   size_t theSize = vars_.size();
   for(size_t i = 0; i < theSize; i++) {
      if (vars_[i].name() == name) {
         return vars_[i];
      }
   }
   return Variable::EMPTY();
}

const Variable& Node::find_parent_variable(const std::string& name) const
{
   const Variable& var = findVariable(name);
   if (!var.empty()) return var;

   Node* theParent = parent();
   while (theParent) {
      const Variable& pvar = theParent->findVariable(name);
      if (!pvar.empty()) return  pvar;
      theParent = theParent->parent();
   }

   // If all else fails search defs environment
   Defs* the_defs = defs();
   if ( the_defs ) {
      return the_defs->server().findVariable(name);
   }

   return Variable::EMPTY();
}

bool Node::findVariableValue( const std::string& name, std::string& returnedValue) const
{
   size_t theSize = vars_.size();
   for(size_t i = 0; i < theSize; i++) {
      if (vars_[i].name() == name) {
         returnedValue = vars_[i].theValue();
         return true;
      }
   }
   return false;
}

bool Node::findGenVariableValue( const std::string& name, std::string& returnedValue) const
{
   const Variable& genVar = findGenVariable(name);
   if (!genVar.empty()) {
      returnedValue = genVar.theValue();
      return true;
   }
   return false;
}


bool Node::findLimit(const Limit& theLimit) const
{
   size_t theSize = limits_.size();
   for(size_t i = 0; i < theSize; i++) {
      if (limits_[i]->name() == theLimit.name()) {
         return true;
      }
   }
   return false;
}

limit_ptr Node::find_limit(const std::string& theName) const
{
   size_t theSize = limits_.size();
   for(size_t i = 0; i < theSize; i++) {
      if (limits_[i]->name() == theName) {
         return limits_[i];
      }
   }
   return limit_ptr();
}

limit_ptr  Node::findLimitUpNodeTree(const std::string& name) const
{
   limit_ptr theFndLimit = find_limit(name);
   if (theFndLimit.get()) return theFndLimit;

   Node* theParent = parent();
   while (theParent != NULL) {

      limit_ptr theFndLimit = theParent->find_limit(name);
      if (theFndLimit.get()) return theFndLimit;

      theParent = theParent->parent();
   }
   return limit_ptr();
}


const Event& Node::findEvent(const Event& theEvent) const
{
   if (child_attrs_) return child_attrs_->findEvent(theEvent);
   return Event::EMPTY();
}

const Event& Node::findEventByNumber(int number) const
{
   if (child_attrs_) return child_attrs_->findEventByNumber(number);
   return Event::EMPTY();
}

const Event& Node::findEventByName( const std::string& event_name) const
{
   if (child_attrs_) return child_attrs_->findEventByName(event_name);
   return Event::EMPTY();
}

const Event& Node::findEventByNameOrNumber( const std::string& theName) const
{
   if (child_attrs_) return child_attrs_->findEventByNameOrNumber(theName);
   return Event::EMPTY();
}
const Meter& Node::findMeter(const std::string& name) const
{
   if (child_attrs_) return child_attrs_->findMeter(name);
   return Meter::EMPTY();
}

Meter& Node::find_meter(const std::string& name)
{
   if (child_attrs_) return child_attrs_->find_meter(name);
   return const_cast<Meter&>(Meter::EMPTY());
}

bool Node::findLabel(const std::string& name) const
{
   if (child_attrs_) return child_attrs_->findLabel(name);
   return false;
}

const Label& Node::find_label(const std::string& name) const
{
   if (child_attrs_) return child_attrs_->find_label(name);
   return Label::EMPTY();
}

bool Node::findVerify(const VerifyAttr& v) const
{
   if (misc_attrs_) return misc_attrs_->findVerify(v);
   return false;
}

const QueueAttr& Node::find_queue(const std::string& name) const
{
   if (misc_attrs_) return misc_attrs_->find_queue(name);
    return QueueAttr::EMPTY();
}

QueueAttr& Node::findQueue(const std::string& name)
{
   if (misc_attrs_) return misc_attrs_->findQueue(name);
   return QueueAttr::EMPTY1();
}

const GenericAttr& Node::find_generic(const std::string& name) const
{
   if (misc_attrs_) return misc_attrs_->find_generic(name);
    return GenericAttr::EMPTY();
}

const Repeat& Node::findRepeat(const std::string& name) const
{
   if (!repeat_.empty() && repeat_.name() == name) {
      return repeat_;
   }
   return Repeat::EMPTY();
}


bool Node::findExprVariable( const std::string& name)
{
   // if event found return true. also mark this event so simulator know its used in a trigger
   if ( set_event_used_in_trigger(name) ) {
      return true;
   }

   // if meter found mark as used in trigger for simulator and return
   if ( set_meter_used_in_trigger(name) ) {
      return true;
   }

   const Variable& user_variable = findVariable( name );
   if (!user_variable.empty()) return true;

   const Repeat& repeat = findRepeat( name );
   if (!repeat.empty()) return true;

   const Variable& gen_variable = findGenVariable( name );
   if (!gen_variable.empty()) return true;

   limit_ptr limit = find_limit( name );
   if (limit.get()) return true;

   QueueAttr& queue_attr = findQueue( name );
   if (!queue_attr.empty()) {
      queue_attr.set_used_in_trigger(true);
      return true;
   }

   return false;
}

int Node::findExprVariableValue( const std::string& name) const
{
   const Event& event = findEventByNameOrNumber( name );
   if ( !event.empty() )  return (event.value() ? 1 : 0);

   const Meter& meter = findMeter( name );
   if ( !meter.empty() )  return meter.value();

   const Variable& variable = findVariable( name );
   if ( !variable.empty() ) return variable.value();

   const Repeat& repeat = findRepeat( name );
   if ( !repeat.empty() )  {
      // RepeatDate       last_valid_value() returns the date by its real value as a long
      // RepeatInteger    last_valid_value() returns the value, by the current value of integer
      // RepeatEnumerated last_valid_value() returns the current value if cast-able to integer otherwise position/index,
      //                                     (i.e. since enum can be anything)
      // RepeatString     last_valid_value() returns the current position/index  ( Alternatives ? )
      // RepeatDay        last_valid_value() returns the current step
      // Note: At Repeat expiration Repeat::value() may be out of range of start-end
      //       But Repeat::last_valid_value() should always be in range, hence at Repeat expiration
      //       will return the last valid value.
      return repeat.last_valid_value();
   }

   const Variable& gen_variable =  findGenVariable( name );
   if ( !gen_variable.empty() )  return gen_variable.value();

   limit_ptr limit = find_limit( name );
   if (limit.get()) return limit->value();

   const QueueAttr& queue_attr = find_queue( name );
   if ( !queue_attr.empty() )  return queue_attr.index_or_value();

   return 0;
}

int Node::findExprVariableValueAndPlus(const std::string& name, int val) const
{
   const Event& event = findEventByNameOrNumber( name );
   if ( !event.empty() )  return ((event.value() ? 1 : 0) + val);

   const Meter& meter = findMeter( name );
   if ( !meter.empty() )  return (meter.value() + val);

   const Variable& variable = findVariable( name );
   if ( !variable.empty() ) return (variable.value() + val);

   const Repeat& repeat = findRepeat( name );
   if ( !repeat.empty() )  {
      // RepeatDate       last_valid_value() returns the date by its real value as a long
      // RepeatInteger    last_valid_value() returns the value, by the current value of integer
      // RepeatEnumerated last_valid_value() returns the current value if cast-able as integer otherwise position/index,
      //                                    (i.e. since enum can be anything)
      // RepeatString     last_valid_value() returns the current position/index  ( Alternatives ? )
      // RepeatDay        last_valid_value() returns the current step
      // Note: At Repeat expiration Repeat::value() may be out of range of start-end
      //       But Repeat::last_valid_value() should always be in range, hence at Repeat expiration
      //       will return the last valid value.
      return repeat.last_valid_value_plus(val);
   }

   const Variable& gen_variable =  findGenVariable( name );
   if ( !gen_variable.empty() )  return (gen_variable.value()+val);

   limit_ptr limit = find_limit( name );
   if (limit.get()) return (limit->value() + val);

   const QueueAttr& queue_attr = find_queue( name );
   if ( !queue_attr.empty() )  return (queue_attr.index_or_value() + val );

   return val;
}

int Node::findExprVariableValueAndMinus(const std::string& name, int val) const
{
   const Event& event = findEventByNameOrNumber( name );
   if ( !event.empty() )  return ( (event.value() ? 1 : 0) - val);

   const Meter& meter = findMeter( name );
   if ( !meter.empty() )  return (meter.value() - val );

   const Variable& variable = findVariable( name );
   if ( !variable.empty() ) return (variable.value() - val);

   const Repeat& repeat = findRepeat( name );
   if ( !repeat.empty() )  {
      // RepeatDate       last_valid_value() returns the date by its real value as a long
      // RepeatInteger    last_valid_value() returns the value, by the current value of integer
      // RepeatEnumerated last_valid_value() returns the current value if cast-able as integer, else position/index, (
      //                                     i.e. since enum can be anything)
      // RepeatString     last_valid_value() returns the current position/index  ( Alternatives ? )
      // RepeatDay        last_valid_value() returns the current step
      // Note: At Repeat expiration Repeat::value() may be out of range of start-end
      //       But Repeat::last_valid_value() should always be in range, hence at Repeat expiration
      //       will return the last valid value.
      return repeat.last_valid_value_minus(val);
   }

   const Variable& gen_variable = findGenVariable( name );
   if ( !gen_variable.empty() )  return (gen_variable.value() - val);

   limit_ptr limit = find_limit( name );
   if (limit.get()) return (limit->value() - val);

   const QueueAttr& queue_attr = find_queue( name );
   if ( !queue_attr.empty() )  return (queue_attr.index_or_value() - val );

   return -val;
}

int Node::findExprVariableValueAndType( const std::string& name, std::string& varType) const
{
   const Event& event = findEventByNameOrNumber( name );
   if ( !event.empty() )  {
      varType = "event";
      return (event.value() ? 1 : 0);
   }
   const Meter& meter = findMeter( name );
   if ( !meter.empty() )  {
      varType = "meter";
      return meter.value();
   }
   const Variable& variable = findVariable( name );
   if ( !variable.empty() ) {
      varType = "user-variable";
      return variable.value();
   }
   const Repeat& repeat = findRepeat( name );
   if ( !repeat.empty() )  {
      varType  = "repeat";
      return repeat.last_valid_value();
   }
   const Variable& gen_variable = findGenVariable( name );
   if ( !gen_variable.empty() )  {
      varType = "gen-variable";
      return gen_variable.value();
   }
   limit_ptr limit = find_limit( name );
   if (limit.get()) {
      varType = "limit";
      return limit->value();
   }
   const QueueAttr& queue_attr = find_queue( name );
   if ( !queue_attr.empty() )  {
      varType = "queue";
      return queue_attr.index_or_value();
   }

   varType = "variable-not-found";
   return 0;
}

void Node::findExprVariableAndPrint( const std::string& name, ostream& os) const
{
   const Event& event = findEventByNameOrNumber( name );
   if ( !event.empty() )  {
      os << event.dump();
      return;
   }
   const Meter& meter = findMeter( name );
   if ( !meter.empty() ) {
      os << meter.dump();
      return;
   }
   const Variable& variable = findVariable( name );
   if ( !variable.empty() ) {
      os << "USER-VARIABLE " << variable.dump();
      return;
   }
   const Repeat& repeat = findRepeat( name );
   if ( !repeat.empty() )  {
      os << repeat.dump();
      return;
   }
   const Variable& gen_variable = findGenVariable( name );
   if ( !gen_variable.empty() )  {
      os << "GEN-VARIABLE " << gen_variable.dump();
      return;
   }
   limit_ptr limit = find_limit( name );
   if (limit.get()) {
      os << limit->toString() << " value(" << limit->value() << ")";
      return;
   }
   const QueueAttr& queue_attr = find_queue( name );
   if ( !queue_attr.empty() )  {
      os << "QUEUE " << queue_attr.name() << " value(" << queue_attr.index_or_value() << ")";
      return;
   }
}


node_ptr findRelativeNode(	const vector<std::string>& theExtractedPath,node_ptr triggerNode, std::string& errorMsg )
{
   // The referenced node could be itself(error) or most likely a sibling node.
   int extractedPathSize = static_cast<int>(theExtractedPath.size());
   if (extractedPathSize == 1 && triggerNode->name() == theExtractedPath[0]) {
      // self referencing node ?
      return triggerNode;
   }

   // Can only find *sibling* if triggerNode has a parent
   if (!triggerNode->parent()) {
      errorMsg = "Parent empty. Could not find referenced node\n";
      return node_ptr();
   }
   if (extractedPathSize == 1) {
      size_t child_pos; // not used
      node_ptr theNode = triggerNode->parent()->findImmediateChild(theExtractedPath[0],child_pos);
      if ( theNode.get() ) {
         return theNode;
      }
   }

   node_ptr constNode = triggerNode->parent()->find_relative_node(theExtractedPath);
   if (constNode.get()) {
      return constNode;
   }

   errorMsg = "Could not find node '";
   if (extractedPathSize == 1)  errorMsg += theExtractedPath[0];
   else { BOOST_FOREACH(const string& s, theExtractedPath) { errorMsg += s; errorMsg += Str::PATH_SEPERATOR();} }
   errorMsg += "' from node ";
   errorMsg += triggerNode->absNodePath();
   if (extractedPathSize == 1) {
      errorMsg += " . Expected '";
      errorMsg += theExtractedPath[0];
      errorMsg += "' to be a sibling.";
   }
   errorMsg += "\n";
   return node_ptr();
}


node_ptr Node::non_const_this() const {
   return const_pointer_cast<Node>(shared_from_this());
}

node_ptr Node::findReferencedNode(const std::string& nodePath, std::string& errorMsg) const
{
   return findReferencedNode(nodePath,Str::EMPTY(),errorMsg);
}

//#define DEBUG_FIND_REFERENCED_NODE 1
node_ptr Node::findReferencedNode(const std::string& nodePath, const std::string& extern_obj, std::string& errorMsg) const
{
#ifdef DEBUG_FIND_REFERENCED_NODE
   cout << "Node::findReferencedNode path:" << nodePath << " extern_obj:" << extern_obj << "\n";
#endif
   Defs* theDefs = defs();
   if (!theDefs) {
      // In the case where we have a stand alone Node. i.e no parent set. The Defs will be NULL.
      // Take the case where we want to dump the state of a single node.
      //    ecflow_client --get_state=/test/f2 --port=4141
      // Here we are printing the state of the Node only, *NO* defs is returned.
      // The print will cause the AST to be evaluated. The trigger evaluation will required chasing
      // down reference nodes. Hence we will end up here. Rather than crashing, just return a NULL Pointer.
      return node_ptr();
   }

   /// findReferencedNode:: is used to locate references:
   ///      a/ Trigger & complete expressions, this is where extern_obj is used.
   ///      b/ Inlimit nodepaths.
   ///  In *both* the case above, the node path may not exist, in the definition. Hence::
   ///  On client side:: references not defined in externs are considered errors
   ///  On server side:: No extern's are stored, hence for unresolved node paths, we return NULL

#ifdef DEBUG_FIND_REFERENCED_NODE
   string debug_path = "Searching for path " + nodePath + " from " + debugType() + Str::COLON() + absNodePath() + "\n";
#endif

   // if an absolute path cut in early
   if (!nodePath.empty() && nodePath[0] == '/') {

#ifdef DEBUG_FIND_REFERENCED_NODE
      debug_path += "(!nodePath.empty() && nodePath[0] == '/') \n";
#endif

      // Must be a absolute path. i.e /suite/family/path
      node_ptr constNode = theDefs->findAbsNode(nodePath);
      if (constNode.get()) {
         return constNode;
      }

      // *NOTE*: The server does *NOT* store externs, hence the check below, will always return false, for the server:
      // Must be an extern:
      //    extern /referenceSuite/family/task:obj
      //    extern /referenceSuite/family/task
      if (theDefs->find_extern(nodePath,extern_obj)) {

         // =================================================================
         // **Client side* specific: Only client side defs, stores extrens
         // =================================================================
#ifdef DEBUG_FIND_REFERENCED_NODE
         debug_path += "theDefs->find_extern(nodePath) \n";
#endif

         // return NULL *without* setting an error message as node path is defined as an extern

         // OK: the node path appears in the extern list. This may be because that suite  has not been loaded.
         // *** If the suite is loaded, then its an error that we did not
         // *** locate the node. i.e in the previous call to defs->findAbsNode(nodePath);
         vector<string> theExtractedPath;
         NodePath::split(nodePath, theExtractedPath );

         std::string referenceSuite = theExtractedPath[0];
         if (theDefs->findSuite(referenceSuite)) {
            // The suite referenced in the extern is LOADED, but path did not resolve,
            // in previous call to defs->findAbsNode(nodePath);
            errorMsg = "Extern path ";
            errorMsg += nodePath;
            errorMsg += " does not exist on suite ";
            errorMsg += referenceSuite;
            errorMsg += "\n";
#ifdef DEBUG_FIND_REFERENCED_NODE
            errorMsg += debug_path;
#endif
         }

         // Its an extern path that references a suite thats NOT loaded yet
         return node_ptr();
      }

      errorMsg = ": Could not find referenced node, using absolute path '";
      errorMsg += nodePath;
      errorMsg += "\n";
      return node_ptr();
   }

   /// =============================================================================
   /// Path is something other than ABSOLUTE path
   /// =============================================================================
  vector<string> theExtractedPath;
   NodePath::split( nodePath, theExtractedPath );

#ifdef DEBUG_FIND_REFERENCED_NODE
   debug_path += "extracted path = ";
   BOOST_FOREACH(const string& s, theExtractedPath) { debug_path += ",";debug_path += s;}
   debug_path += "\n";
#endif

   if ( theExtractedPath.empty() ) {
      std::stringstream ss;
      ss << ": Could not find referenced node '" << nodePath << "' from node " << absNodePath() << "\n";
      errorMsg = ss.str();
#ifdef DEBUG_FIND_REFERENCED_NODE
      errorMsg += debug_path;
#endif
      return node_ptr();
   }


   //  i.e   " a == complete" =====>        nodePath = a
   if ( theExtractedPath.size() == 1) {
#ifdef DEBUG_FIND_REFERENCED_NODE
      debug_path += "( theExtractedPath.size() == 1)\n";
#endif
      // Search for a relative node first
      string localErrorMsg;
      node_ptr res = findRelativeNode(theExtractedPath,non_const_this(),localErrorMsg);
#ifdef DEBUG_FIND_REFERENCED_NODE
      if (!localErrorMsg.empty()) {
         debug_path += localErrorMsg;
         localErrorMsg = debug_path;
      }
#endif

      if (!res.get()) {
         // lets see if its in an extern Node. extern can have absolute and relative paths
         // In this case it will be a relative path, hence no point trying to see if suite
         // is loaded
         if (theDefs->find_extern(nodePath,extern_obj)) {
            // =================================================================
            // **Client side* specific: Only client side defs, stores externs
            // =================================================================
            // The path exist in the extern and we know its relative
            return node_ptr();
         }
      }

      errorMsg += localErrorMsg;
      return res;
   }

   // handle Node path of type   a/b/c
   if (theExtractedPath.size() >= 2 && theExtractedPath[0] != "." && theExtractedPath[0] != "..") {
#ifdef DEBUG_FIND_REFERENCED_NODE
      debug_path += "(theExtractedPath.size() >= 2 && theExtractedPath[0] != \".\") && theExtractedPath[0] != \"..\"\n";
#endif
      // First check to see see if its in the externs
      if (theDefs->find_extern(nodePath,extern_obj)) {
         // =================================================================
         // **Client side* specific: Only client side defs, stores externs
         // =================================================================
         // The path a/b/c exist in the extern and we know its relative
         // Again no point in checking to see if suite is loaded if path is relative
         return node_ptr();
      }

      // In this case its equivalent to: ./a/b/c
      theExtractedPath.insert(theExtractedPath.begin(),".");
   }

   // node path == "./a"
   if ( theExtractedPath.size() >= 2 && theExtractedPath[0] == ".") {
#ifdef DEBUG_FIND_REFERENCED_NODE
      debug_path += "theExtractedPath.size() == 2 && theExtractedPath[0] == \".\" \n";
#endif
      theExtractedPath.erase( theExtractedPath.begin() + 0);
      node_ptr res = findRelativeNode(theExtractedPath,non_const_this(),errorMsg);
#ifdef DEBUG_FIND_REFERENCED_NODE
      if (!errorMsg.empty()) {
         debug_path += errorMsg;
         errorMsg = debug_path;
      }
#endif
      return res;
   }

   // ********************************************************************
   // Note  ./  This means go to the parent, then search down, i.e sibling
   //       ../ This means go to the parents parent
   //       This may be confusing, but this is how Axel expects it, hence I will use
   //       the same understanding
   // **********************************************************************
   // Handle node path of the type "../a/b/c"  "../../a/b/c"
   if ( theExtractedPath.size() >= 2 && theExtractedPath[0] == "..") {

#ifdef DEBUG_FIND_REFERENCED_NODE
      debug_path += "( theExtractedPath.size() >= 2 && theExtractedPath[0] == \"..\")\n";
#endif

      Node* theParent = parent(); // get past the first parent
      while ( static_cast<int>(theExtractedPath.size()) &&
               theParent &&
               theExtractedPath[0] == ".." )
      {
         theExtractedPath.erase( theExtractedPath.begin() + 0);
         theParent = theParent->parent();  // for each .. go up a parent
#ifdef DEBUG_FIND_REFERENCED_NODE
         debug_path += "..: thepParent = ";
         if (theParent) debug_path += theParent->absNodePath();
         else           debug_path += "NULL";
#endif
      }

      if ( theParent ) {

#ifdef DEBUG_FIND_REFERENCED_NODE
         debug_path += "searching = " + theParent->name()  + "\n";
         BOOST_FOREACH(const std::string& s , theExtractedPath) { debug_path += Str::PATH_SEPERATOR() + s; }
         debug_path += "\n";
#endif

         node_ptr constNode = theParent->find_relative_node(theExtractedPath);
         if (constNode) {
            return constNode;
         }
      }
   }

   errorMsg = "Unrecognised path ";
   errorMsg += nodePath;
   errorMsg += " for Node ";
   errorMsg += absNodePath();
   errorMsg += "\n";
#ifdef DEBUG_FIND_REFERENCED_NODE
   errorMsg += debug_path;
#endif
   return node_ptr();
}

const ZombieAttr& Node::findZombie( ecf::Child::ZombieType zombie_type) const
{
   if (misc_attrs_) return misc_attrs_->findZombie(zombie_type);
   return ZombieAttr::EMPTY();
}

bool Node::findParentZombie(ecf::Child::ZombieType z_type, ZombieAttr& z) const
{
   const ZombieAttr& the_zombie = findZombie(z_type);
   if ( !the_zombie.empty() ) {
      z = the_zombie;
      return true;
   }

   Node* theParent =  parent();
   while (theParent) {
      const ZombieAttr& the_zombie = theParent->findZombie(z_type);
      if ( !the_zombie.empty() ) {
         z = the_zombie;
         return true;
      }
      theParent = theParent->parent();
   }
   return false;
}
