/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <stdexcept>

#include "ecflow/attribute/AutoArchiveAttr.hpp"
#include "ecflow/attribute/AutoCancelAttr.hpp"
#include "ecflow/attribute/LateAttr.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/node/AutoRestoreAttr.hpp"
#include "ecflow/node/AvisoAttr.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Expression.hpp"
#include "ecflow/node/Limit.hpp"
#include "ecflow/node/MirrorAttr.hpp"
#include "ecflow/node/MiscAttrs.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/NodeAlgorithms.hpp"
#include "ecflow/node/NodeContainer.hpp"
#include "ecflow/python/DefsDoc.hpp"
#include "ecflow/python/Edit.hpp"
#include "ecflow/python/ExportCollections.hpp"
#include "ecflow/python/NodeAttrDoc.hpp"
#include "ecflow/python/NodeUtil.hpp"
#include "ecflow/python/PythonBinding.hpp"
#include "ecflow/python/PythonUtil.hpp"

namespace {

///
/// @brief Return a pointer to the Defs that owns the node.
///
/// @param self The node.
/// @return A raw pointer to the owning Defs, or nullptr if the node has no parent defs.
///
Defs* Node_get(node_ptr self) {
    return self->defs();
}

///
/// @brief Add or update a user variable on the node using a name/value string pair.
///
/// @param self The node.
/// @param name The variable name.
/// @param value The variable value.
/// @return The node (for method chaining).
///
node_ptr Node_add_variable_string(node_ptr self, const std::string& name, const std::string& value) {
    self->add_variable(name, value);
    return self;
}

///
/// @brief Add or update a user variable on the node using a name/integer pair.
///
/// @param self The node.
/// @param name The variable name.
/// @param value The integer value.
/// @return The node (for method chaining).
///
node_ptr Node_add_variable_int(node_ptr self, const std::string& name, int value) {
    self->add_variable_int(name, value);
    return self;
}

///
/// @brief Add or update a user variable on the node from a Variable object.
///
/// @param self The node.
/// @param var The variable to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_variable_variable(node_ptr self, const Variable& var) {
    self->addVariable(var);
    return self;
}

///
/// @brief Add all variables contained in an Edit object to the node.
///
/// @param self The node.
/// @param edit The Edit object whose variables are added to the node.
/// @return The node (for method chaining).
///
node_ptr Node_add_variable_edit(node_ptr self, const Edit& edit) {
    for (const auto& var : edit.variables()) {
        self->addVariable(var);
    }
    return self;
}

///
/// @brief Add an event attribute to the node.
///
/// @param self The node.
/// @param e The event to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_event(node_ptr self, const Event& e) {
    self->addEvent(e);
    return self;
}

///
/// @brief Add an event identified by an integer number to the node.
///
/// @param self The node.
/// @param number The event number.
/// @return The node (for method chaining).
///
node_ptr Node_add_event_int(node_ptr self, int number) {
    self->addEvent(Event(number));
    return self;
}

///
/// @brief Add an event identified by a name string to the node.
///
/// @param self The node.
/// @param name The event name.
/// @return The node (for method chaining).
///
node_ptr Node_add_event_string(node_ptr self, const std::string& name) {
    self->addEvent(Event(name));
    return self;
}

///
/// @brief Add an event identified by both a number and a name to the node.
///
/// @param self The node.
/// @param number The event number.
/// @param name The event name.
/// @return The node (for method chaining).
///
node_ptr Node_add_event_int_string(node_ptr self, int number, const std::string& name) {
    self->addEvent(Event(number, name));
    return self;
}

///
/// @brief Add a meter attribute to the node.
///
/// @param self The node.
/// @param m The meter to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_meter(node_ptr self, const Meter& m) {
    self->addMeter(m);
    return self;
}

///
/// @brief Create and add a meter with a color change threshold to the node.
///
/// @param self The node.
/// @param meter_name The meter name.
/// @param min The minimum meter value.
/// @param max The maximum meter value.
/// @param color_change The value at which the GUI changes colour.
/// @return The node (for method chaining).
///
node_ptr Node_add_meter_1(node_ptr self, const std::string& meter_name, int min, int max, int color_change) {
    self->addMeter(Meter(meter_name, min, max, color_change));
    return self;
}

///
/// @brief Create and add a meter with only min/max bounds to the node.
///
/// @param self The node.
/// @param meter_name The meter name.
/// @param min The minimum meter value.
/// @param max The maximum meter value.
/// @return The node (for method chaining).
///
node_ptr Node_add_meter_2(node_ptr self, const std::string& meter_name, int min, int max) {
    self->addMeter(Meter(meter_name, min, max));
    return self;
}

///
/// @brief Add a queue attribute to the node.
///
/// @param self The node.
/// @param m The queue attribute to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_queue(node_ptr self, const QueueAttr& m) {
    self->add_queue(m);
    return self;
}

///
/// @brief Create and add a queue attribute from a name and a list of step strings.
///
/// @param self The node.
/// @param name The queue name.
/// @param list A Python list of step strings for the queue.
/// @return The node (for method chaining).
///
node_ptr Node_add_queue1(node_ptr self, const std::string& name, const py::list& list) {
    std::vector<std::string> vec;
    pyutil_list_to_str_vec(list, vec);
    QueueAttr queue_attr(name, vec);
    self->add_queue(queue_attr);
    return self;
}

///
/// @brief Add a generic attribute to the node.
///
/// @param self The node.
/// @param m The generic attribute to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_generic(node_ptr self, const GenericAttr& m) {
    self->add_generic(m);
    return self;
}

///
/// @brief Create and add a generic attribute from a name and a list of value strings.
///
/// @param self The node.
/// @param name The generic attribute name.
/// @param list A Python list of value strings.
/// @return The node (for method chaining).
///
node_ptr Node_add_generic1(node_ptr self, const std::string& name, const py::list& list) {
    std::vector<std::string> vec;
    pyutil_list_to_str_vec(list, vec);
    GenericAttr attr(name, vec);
    self->add_generic(attr);
    return self;
}

///
/// @brief Add a label with a given name and initial value to the node.
///
/// @param self The node.
/// @param name The label name.
/// @param value The initial label value.
/// @return The node (for method chaining).
///
node_ptr Node_add_label(node_ptr self, const std::string& name, const std::string& value) {
    self->addLabel(Label(name, value));
    return self;
}

///
/// @brief Add a label attribute object to the node.
///
/// @param self The node.
/// @param label The label to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_label_1(node_ptr self, const Label& label) {
    self->addLabel(label);
    return self;
}

///
/// @brief Add an Aviso attribute to the node.
///
/// @param self The node.
/// @param attr The Aviso attribute to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_aviso(node_ptr self, const ecf::AvisoAttr& attr) {
    self->addAviso(attr);
    return self;
}

///
/// @brief Add a Mirror attribute to the node.
///
/// @param self The node.
/// @param attr The Mirror attribute to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_mirror(node_ptr self, const ecf::MirrorAttr& attr) {
    self->addMirror(attr);
    return self;
}

///
/// @brief Add a limit with a given name and maximum token count to the node.
///
/// @param self The node.
/// @param name The limit name.
/// @param limit The maximum number of tokens in the limit.
/// @return The node (for method chaining).
///
node_ptr Node_add_limit(node_ptr self, const std::string& name, int limit) {
    self->addLimit(Limit(name, limit));
    return self;
}

///
/// @brief Add a limit attribute object to the node.
///
/// @param self The node.
/// @param limit The limit to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_limit_1(node_ptr self, const Limit& limit) {
    self->addLimit(limit);
    return self;
}

///
/// @brief Add an in-limit constraint to the node.
///
/// @param self The node.
/// @param name The name of the limit to reference.
/// @param pathToNode The path to the node that holds the limit.
/// @param tokens The number of tokens to consume from the limit.
/// @param limit_this_node_only If true, limit applies only to this node.
/// @return The node (for method chaining).
///
node_ptr Node_add_in_limit(node_ptr self,
                           const std::string& name,
                           const std::string& pathToNode,
                           int tokens,
                           bool limit_this_node_only) {
    self->addInLimit(InLimit(name, pathToNode, tokens, limit_this_node_only));
    return self;
}

///
/// @brief Add an in-limit attribute object to the node.
///
/// @param self The node.
/// @param inlimit The in-limit to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_in_limit_1(node_ptr self, const InLimit& inlimit) {
    self->addInLimit(inlimit);
    return self;
}

///
/// @brief Add an absolute time dependency to the node.
///
/// @param self The node.
/// @param hour The hour component of the time.
/// @param minute The minute component of the time.
/// @return The node (for method chaining).
///
node_ptr Node_add_time(node_ptr self, int hour, int minute) {
    self->addTime(ecf::TimeAttr(hour, minute));
    return self;
}

///
/// @brief Add a time dependency to the node, optionally relative to suite start.
///
/// @param self The node.
/// @param hour The hour component of the time.
/// @param minute The minute component of the time.
/// @param relative If true, the time is relative to the suite clock start.
/// @return The node (for method chaining).
///
node_ptr Node_add_time_1(node_ptr self, int hour, int minute, bool relative) {
    self->addTime(ecf::TimeAttr(hour, minute, relative));
    return self;
}

///
/// @brief Add a time dependency to the node from a string representation.
///
/// @param self The node.
/// @param ts A string describing the time series (e.g. `"10:00"`, `"10:00 20:00 01:00"`).
/// @return The node (for method chaining).
///
node_ptr Node_add_time_2(node_ptr self, const std::string& ts) {
    self->addTime(ecf::TimeAttr(ecf::TimeSeries::create(ts)));
    return self;
}

///
/// @brief Add a time attribute object to the node.
///
/// @param self The node.
/// @param ts The time attribute to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_time_3(node_ptr self, const ecf::TimeAttr& ts) {
    self->addTime(ts);
    return self;
}

///
/// @brief Add an absolute today time dependency to the node.
///
/// @param self The node.
/// @param hour The hour component of the time.
/// @param minute The minute component of the time.
/// @return The node (for method chaining).
///
node_ptr Node_add_today(node_ptr self, int hour, int minute) {
    self->addToday(ecf::TodayAttr(hour, minute));
    return self;
}

///
/// @brief Add a today time dependency to the node, optionally relative to suite start.
///
/// @param self The node.
/// @param hour The hour component of the time.
/// @param minute The minute component of the time.
/// @param relative If true, the time is relative to the suite clock start.
/// @return The node (for method chaining).
///
node_ptr Node_add_today_1(node_ptr self, int hour, int minute, bool relative) {
    self->addToday(ecf::TodayAttr(hour, minute, relative));
    return self;
}

///
/// @brief Add a today time dependency to the node from a string representation.
///
/// @param self The node.
/// @param ts A string describing the time series (e.g. `"10:00"`, `"10:00 20:00 01:00"`).
/// @return The node (for method chaining).
///
node_ptr Node_add_today_2(node_ptr self, const std::string& ts) {
    self->addToday(ecf::TodayAttr(ecf::TimeSeries::create(ts)));
    return self;
}

///
/// @brief Add a today attribute object to the node.
///
/// @param self The node.
/// @param ts The today attribute to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_today_3(node_ptr self, const ecf::TodayAttr& ts) {
    self->addToday(ts);
    return self;
}

///
/// @brief Add a date dependency to the node from day, month, and year components.
///
/// Zero may be used as a wildcard for any component.
///
/// @param self The node.
/// @param day The day of the month (1–31, or 0 for any).
/// @param month The month (1–12, or 0 for any).
/// @param year The year, or 0 for any.
/// @return The node (for method chaining).
///
node_ptr Node_add_date(node_ptr self, int day, int month, int year) {
    self->addDate(DateAttr(day, month, year));
    return self;
}

///
/// @brief Add a date attribute object to the node.
///
/// @param self The node.
/// @param d The date attribute to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_date_1(node_ptr self, const DateAttr& d) {
    self->addDate(d);
    return self;
}

///
/// @brief Add a day-of-week dependency to the node using a Day_t enum value.
///
/// @param self The node.
/// @param day The day of the week as a DayAttr::Day_t enum value.
/// @return The node (for method chaining).
///
node_ptr Node_add_day(node_ptr self, DayAttr::Day_t day) {
    self->addDay(DayAttr(day));
    return self;
}

///
/// @brief Add a day-of-week dependency to the node from a day name string.
///
/// @param self The node.
/// @param day The day name (e.g. `"monday"`, `"tuesday"`, etc.).
/// @return The node (for method chaining).
///
node_ptr Node_add_day_1(node_ptr self, const std::string& day) {
    self->addDay(DayAttr(DayAttr::getDay(day)));
    return self;
}

///
/// @brief Add a day attribute object to the node.
///
/// @param self The node.
/// @param day The day attribute to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_day_2(node_ptr self, const DayAttr& day) {
    self->addDay(day);
    return self;
}

///
/// @brief Add an auto-cancel attribute that cancels the node after a given number of days.
///
/// @param self The node.
/// @param days The number of days after completion before the node is cancelled.
/// @return The node (for method chaining).
///
node_ptr Node_add_autocancel(node_ptr self, int days) {
    self->addAutoCancel(ecf::AutoCancelAttr(days));
    return self;
}

///
/// @brief Add an auto-cancel attribute specified by hour, minute, and a relative flag.
///
/// @param self The node.
/// @param hour The hour component of the cancel delay.
/// @param min The minute component of the cancel delay.
/// @param relative If true, the time is relative to the completion time.
/// @return The node (for method chaining).
///
node_ptr Node_add_autocancel_1(node_ptr self, int hour, int min, bool relative) {
    self->addAutoCancel(ecf::AutoCancelAttr(hour, min, relative));
    return self;
}

///
/// @brief Add an auto-cancel attribute specified by a TimeSlot and a relative flag.
///
/// @param self The node.
/// @param ts The time slot representing the cancel delay.
/// @param relative If true, the time is relative to the completion time.
/// @return The node (for method chaining).
///
node_ptr Node_add_autocancel_2(node_ptr self, const ecf::TimeSlot& ts, bool relative) {
    self->addAutoCancel(ecf::AutoCancelAttr(ts, relative));
    return self;
}

///
/// @brief Add an auto-cancel attribute object to the node.
///
/// @param self The node.
/// @param attr The auto-cancel attribute to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_autocancel_3(node_ptr self, const ecf::AutoCancelAttr& attr) {
    self->addAutoCancel(attr);
    return self;
}

///
/// @brief Add an auto-archive attribute that archives the node after a given number of days.
///
/// @param self The node.
/// @param days The number of days after completion before the node is archived.
/// @param idle If true, the timer only ticks while the node is idle.
/// @return The node (for method chaining).
///
node_ptr Node_add_autoarchive(node_ptr self, int days, bool idle) {
    self->add_autoarchive(ecf::AutoArchiveAttr(days, idle));
    return self;
}

///
/// @brief Add an auto-archive attribute specified by hour, minute, relative, and idle flags.
///
/// @param self The node.
/// @param hour The hour component of the archive delay.
/// @param min The minute component of the archive delay.
/// @param relative If true, the time is relative to the completion time.
/// @param idle If true, the timer only ticks while the node is idle.
/// @return The node (for method chaining).
///
node_ptr Node_add_autoarchive_1(node_ptr self, int hour, int min, bool relative, bool idle) {
    self->add_autoarchive(ecf::AutoArchiveAttr(hour, min, relative, idle));
    return self;
}

///
/// @brief Add an auto-archive attribute specified by a TimeSlot, relative, and idle flags.
///
/// @param self The node.
/// @param ts The time slot representing the archive delay.
/// @param relative If true, the time is relative to the completion time.
/// @param idle If true, the timer only ticks while the node is idle.
/// @return The node (for method chaining).
///
node_ptr Node_add_autoarchive_2(node_ptr self, const ecf::TimeSlot& ts, bool relative, bool idle) {
    self->add_autoarchive(ecf::AutoArchiveAttr(ts, relative, idle));
    return self;
}

///
/// @brief Add an auto-archive attribute object to the node.
///
/// @param self The node.
/// @param attr The auto-archive attribute to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_autoarchive_3(node_ptr self, const ecf::AutoArchiveAttr& attr) {
    self->add_autoarchive(attr);
    return self;
}

///
/// @brief Add a zombie attribute to the node.
///
/// @param self The node.
/// @param attr The zombie attribute to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_zombie(node_ptr self, const ZombieAttr& attr) {
    self->addZombie(attr);
    return self;
}

///
/// @brief Add an auto-restore attribute object to the node.
///
/// @param self The node.
/// @param attr The auto-restore attribute to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_autorestore(node_ptr self, const ecf::AutoRestoreAttr& attr) {
    self->add_autorestore(attr);
    return self;
}

///
/// @brief Create and add an auto-restore attribute from a list of node paths.
///
/// @param self The node.
/// @param list A Python list of absolute node paths to restore when this node completes.
/// @return The node (for method chaining).
///
node_ptr Node_add_autorestore1(node_ptr self, const py::list& list) {
    std::vector<std::string> vec;
    pyutil_list_to_str_vec(list, vec);
    self->add_autorestore(ecf::AutoRestoreAttr(vec));
    return self;
}

///
/// @brief Add a cron attribute to the node.
///
/// @param self The node.
/// @param attr The cron attribute to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_cron(node_ptr self, const ecf::CronAttr& attr) {
    self->addCron(attr);
    return self;
}

///
/// @brief Add a late attribute to the node.
///
/// @param self The node.
/// @param attr The late attribute to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_late(node_ptr self, const ecf::LateAttr& attr) {
    self->addLate(attr);
    return self;
}

///
/// @brief Return the time of the last state change as a formatted string.
///
/// @param self The node.
/// @param format The output format: `"iso_extended"` (default), `"iso"`, or `"simple"`.
/// @return The state change time formatted according to \p format.
///
std::string Node_get_state_change_time(node_ptr self, const std::string& format) {
    if (format == "iso_extended") {
        return to_iso_extended_string(self->state_change_time());
    }
    else if (format == "iso") {
        return to_iso_string(self->state_change_time());
    }
    return to_simple_string(self->state_change_time());
}

///
/// @brief Add a RepeatDate attribute to the node.
///
/// @param self The node.
/// @param d The repeat-date attribute to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_repeat_date(node_ptr self, const RepeatDate& d) {
    self->addRepeat(d);
    return self;
}

///
/// @brief Add a RepeatDateTime attribute to the node.
///
/// @param self The node.
/// @param d The repeat-datetime attribute to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_repeat_datetime(node_ptr self, const RepeatDateTime& d) {
    self->addRepeat(d);
    return self;
}

///
/// @brief Add a RepeatDateList attribute to the node.
///
/// @param self The node.
/// @param d The repeat-date-list attribute to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_repeat_date_list(node_ptr self, const RepeatDateList& d) {
    self->addRepeat(d);
    return self;
}

///
/// @brief Add a RepeatInteger attribute to the node.
///
/// @param self The node.
/// @param d The repeat-integer attribute to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_repeat_integer(node_ptr self, const RepeatInteger& d) {
    self->addRepeat(d);
    return self;
}

///
/// @brief Add a RepeatString attribute to the node.
///
/// @param self The node.
/// @param d The repeat-string attribute to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_repeat_string(node_ptr self, const RepeatString& d) {
    self->addRepeat(d);
    return self;
}

///
/// @brief Add a RepeatEnumerated attribute to the node.
///
/// @param self The node.
/// @param d The repeat-enumerated attribute to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_repeat_enum(node_ptr self, const RepeatEnumerated& d) {
    self->addRepeat(d);
    return self;
}

///
/// @brief Add a RepeatDay attribute to the node.
///
/// @param self The node.
/// @param d The repeat-day attribute to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_repeat_day(node_ptr self, const RepeatDay& d) {
    self->addRepeat(d);
    return self;
}

///
/// @brief Sort attributes of the given type on the node (non-recursive, no exclusions).
///
/// @param self The node.
/// @param attr The attribute type to sort.
///
void Node_sort_attributes(node_ptr self, ecf::Attr::Type attr) {
    self->sort_attributes(attr);
}

///
/// @brief Sort attributes of the given type on the node, with optional recursion.
///
/// @param self The node.
/// @param attr The attribute type to sort.
/// @param recursive If true, sort recursively through all child nodes.
///
void Node_sort_attributes1(node_ptr self, ecf::Attr::Type attr, bool recursive) {
    self->sort_attributes(attr, recursive);
}

///
/// @brief Sort attributes of the given type on the node, with recursion and exclusion list.
///
/// @param self The node.
/// @param attr The attribute type to sort.
/// @param recursive If true, sort recursively through all child nodes.
/// @param list A Python list of node names to exclude from sorting.
///
void Node_sort_attributes2(node_ptr self, ecf::Attr::Type attr, bool recursive, const py::list& list) {
    std::vector<std::string> no_sort;
    pyutil_list_to_str_vec(list, no_sort);
    self->sort_attributes(attr, recursive, no_sort);
}

///
/// @brief Sort attributes on the node using an attribute type name string.
///
/// @param self The node.
/// @param attribute_name The attribute type name (case-insensitive).
/// @param recursive If true, sort recursively through all child nodes.
/// @param list A Python list of node names to exclude from sorting.
/// @throws std::runtime_error if \p attribute_name is not a recognised attribute type.
///
void Node_sort_attributes3(node_ptr self, const std::string& attribute_name, bool recursive, const py::list& list) {
    std::string attribute_name_lowered = boost::algorithm::to_lower_copy(attribute_name);
    ecf::Attr::Type attribute          = ecf::Attr::to_attr(attribute_name_lowered);
    if (attribute == ecf::Attr::UNKNOWN) {
        throw std::runtime_error(
            MESSAGE("sort_attributes: the attribute " << attribute_name_lowered << " is not valid"));
    }
    std::vector<std::string> no_sort;
    pyutil_list_to_str_vec(list, no_sort);
    self->sort_attributes(attribute, recursive, no_sort);
}

///
/// @brief Return all descendant nodes of the given node.
///
/// @param self The node.
/// @return A vector of shared pointers to all descendant nodes.
///
std::vector<node_ptr> Node_get_all_nodes(node_ptr self) {
    return ecf::get_all_nodes_ptr(self);
}

///
/// @brief Add a trigger expression to the node from a string.
///
/// @param self The node.
/// @param expr The trigger expression string.
/// @return The node (for method chaining).
///
node_ptr Node_add_trigger(node_ptr self, const std::string& expr) {
    self->add_trigger(expr);
    return self;
}

///
/// @brief Add a trigger expression object to the node.
///
/// @param self The node.
/// @param expr The trigger Expression to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_trigger_expr(node_ptr self, const Expression& expr) {
    self->add_trigger_expr(expr);
    return self;
}

///
/// @brief Add a complete expression to the node from a string.
///
/// @param self The node.
/// @param expr The complete expression string.
/// @return The node (for method chaining).
///
node_ptr Node_add_complete(node_ptr self, const std::string& expr) {
    self->add_complete(expr);
    return self;
}

///
/// @brief Add a complete expression object to the node.
///
/// @param self The node.
/// @param expr The complete Expression to add.
/// @return The node (for method chaining).
///
node_ptr Node_add_complete_expr(node_ptr self, const Expression& expr) {
    self->add_complete_expr(expr);
    return self;
}

///
/// @brief Add a partial trigger expression object to the node.
///
/// @param self The node.
/// @param expr The PartExpression to add as a trigger part.
/// @return The node (for method chaining).
///
node_ptr Node_add_part_trigger(node_ptr self, const PartExpression& expr) {
    self->add_part_trigger(PartExpression(expr));
    return self;
}

///
/// @brief Add a partial trigger expression to the node from a string (defaults to AND join).
///
/// @param self The node.
/// @param expression The expression string.
/// @return The node (for method chaining).
///
node_ptr Node_add_part_trigger_1(node_ptr self, const std::string& expression) {
    self->add_part_trigger(PartExpression(expression));
    return self;
}

///
/// @brief Add a partial trigger expression to the node from a string with an explicit AND/OR join flag.
///
/// @param self The node.
/// @param expression The expression string.
/// @param and_expr If true, the part is joined with AND; if false, with OR.
/// @return The node (for method chaining).
///
node_ptr Node_add_part_trigger_2(node_ptr self, const std::string& expression, bool and_expr) {
    self->add_part_trigger(PartExpression(expression, and_expr));
    return self;
}

///
/// @brief Add a partial complete expression object to the node.
///
/// @param self The node.
/// @param expr The PartExpression to add as a complete part.
/// @return The node (for method chaining).
///
node_ptr Node_add_part_complete(node_ptr self, const PartExpression& expr) {
    self->add_part_complete(PartExpression(expr));
    return self;
}

///
/// @brief Add a partial complete expression to the node from a string (defaults to AND join).
///
/// @param self The node.
/// @param expression The expression string.
/// @return The node (for method chaining).
///
node_ptr Node_add_part_complete_1(node_ptr self, const std::string& expression) {
    self->add_part_complete(PartExpression(expression));
    return self;
}

///
/// @brief Add a partial complete expression to the node from a string with an explicit AND/OR join flag.
///
/// @param self The node.
/// @param expression The expression string.
/// @param and_expr If true, the part is joined with AND; if false, with OR.
/// @return The node (for method chaining).
///
node_ptr Node_add_part_complete_2(node_ptr self, const std::string& expression, bool and_expr) {
    self->add_part_complete(PartExpression(expression, and_expr));
    return self;
}

///
/// @brief Evaluate the trigger expression of the node.
///
/// @param self The node.
/// @return true if the trigger expression evaluates to true, false if there is no trigger or it evaluates to false.
///
bool Node_evaluate_trigger(node_ptr self) {
    Ast* t = self->triggerAst();
    if (t) {
        return t->evaluate();
    }
    return false;
}

///
/// @brief Evaluate the complete expression of the node.
///
/// @param self The node.
/// @return true if the complete expression evaluates to true,
///         false if there is no complete expression or it evaluates to false.
///
bool Node_evaluate_complete(node_ptr self) {
    Ast* t = self->completeAst();
    if (t) {
        return t->evaluate();
    }
    return false;
}

///
/// @brief Set the default status of the node using a DState::State enum value.
///
/// @param self The node.
/// @param s The default state to set.
/// @return The node (for method chaining).
///
node_ptr Node_add_defstatus(node_ptr self, DState::State s) {
    self->addDefStatus(s);
    return self;
}

///
/// @brief Set the default status of the node from a Defstatus object.
///
/// @param self The node.
/// @param ds The Defstatus object whose state is applied.
/// @return The node (for method chaining).
///
node_ptr Node_add_defstatus1(node_ptr self, const Defstatus& ds) {
    self->addDefStatus(ds.state());
    return self;
}

///
/// @brief Return the generated variables of the node as a Python list.
///
/// @param self The node.
/// @return A Python list of Variable objects representing the generated variables.
///
py::list Node_generated_variables_using_python_list(node_ptr self) {
    py::list list;
    std::vector<Variable> vec;
    self->gen_variables(vec);
    for (const auto& i : vec) {
        list.append(i);
    }
    return list;
}

///
/// @brief Populate a vector with the generated variables of the node.
///
/// @param self The node.
/// @param vec The vector to populate with generated Variable objects.
///
void Node_generated_variables_using_variable_list(node_ptr self, std::vector<Variable>& vec) {
    self->gen_variables(vec);
}

///
/// @brief Add a single attribute or child to the node.
///
/// Dispatches on the runtime type of \p arg via `NodeUtil::add`.
///
/// @param self The node.
/// @param arg The object to add.
/// @return The node cast to a Python object (for method chaining).
///
py::object Node_add(node_ptr self, const py::object& arg) {
    NodeUtil::add(*self, arg);
    return py::cast(self);
}

///
/// @brief Add a single attribute or child to the node (in-place addition).
///
/// Dispatches on the runtime type of \p arg via `NodeUtil::add`.
///
/// @param self The node.
/// @param arg The object to add.
/// @return The node cast to a Python object (for method chaining).
///
py::object Node_iadd(node_ptr self, const py::object& arg) {
    NodeUtil::add(*self, arg);
    return py::cast(self);
}

///
/// @brief Implement the `>>` operator: add a child with a trigger on the previous sibling.
///
/// Adds \p arg as a child of \p self (which must be a NodeContainer) and
/// automatically sets a `"previous == complete"` trigger on the new child,
/// linking it to the most recently added non-complete sibling.
///
/// @param self The parent node container.
/// @param arg The child node to add.
/// @return The parent node cast to a Python object (for method chaining).
/// @throws std::runtime_error if \p self is not a NodeContainer, or if \p arg is not a node_ptr.
///
py::object Node_rshift(node_ptr self, const py::object& arg) {
    NodeContainer* nc = self->isNodeContainer();
    if (!nc) {
        throw std::runtime_error("ExportNode::do_rshift() : Can only add a child to Suite or Family");
    }

    if (auto found = py_extract<node_ptr>(arg); found) {
        node_ptr child = found.value();
        self->addChild(child);

        std::vector<node_ptr> children;
        nc->immediateChildren(children);
        node_ptr previous_child;
        for (auto& i : children) {
            if (previous_child && i == child) {
                // if existing trigger, add new trigger as AND
                if (child->get_trigger()) {
                    child->add_part_trigger(
                        PartExpression(previous_child->name() + " == complete", PartExpression::AND));
                }
                else {
                    child->add_trigger_expr(Expression(previous_child->name() + " == complete"));
                }
            }
            if (i->defStatus() != DState::COMPLETE) {
                previous_child = i;
            }
        }
    }
    else {
        throw std::runtime_error("ExportNode::do_rshift() : Argument must be a node_ptr");
    }
    return py::cast(self);
}

///
/// @brief Implement the `<<` operator: add a child and set a reverse trigger on its predecessor.
///
/// Adds \p arg as a child of \p self (which must be a NodeContainer) and
/// automatically sets a `"child == complete"` trigger on the predecessor sibling,
/// linking the predecessor to wait for the newly inserted child.
///
/// @param self The parent node container.
/// @param arg The child node to add.
/// @return The parent node cast to a Python object (for method chaining).
/// @throws std::runtime_error if \p self is not a NodeContainer, or if \p arg is not a node_ptr.
///
py::object Node_lshift(node_ptr self, const py::object& arg) {
    NodeContainer* nc = self->isNodeContainer();
    if (!nc) {
        throw std::runtime_error("ExportNode::do_lshift() : Can only add a child to Suite or Family");
    }

    if (auto found = py_extract<node_ptr>(arg); found) {
        node_ptr child = found.value();
        self->addChild(child);

        std::vector<node_ptr> children;
        nc->immediateChildren(children);
        node_ptr previous_child;
        for (size_t i = 0; i < children.size(); i++) {
            if (i == 0) {
                continue;
            }
            if (children[i - 1]->defStatus() != DState::COMPLETE) {
                previous_child = children[i - 1];
            }

            if (previous_child && previous_child != child && children[i] == child) {
                // if existing trigger, add new trigger as AND
                if (previous_child->get_trigger()) {
                    previous_child->add_part_trigger(
                        PartExpression(child->name() + " == complete", PartExpression::AND));
                }
                else {
                    previous_child->add_trigger_expr(Expression(child->name() + " == complete"));
                }
            }
        }
    }
    else {
        throw std::runtime_error("ExportNode::do_lshift() : Argument must be a node_ptr");
    }
    return py::cast(self);
}

///
/// @brief Add multiple positional and keyword arguments to the node.
///
/// Positional arguments are forwarded to `NodeUtil::add`; keyword arguments are
/// added as user variables.
///
/// @param self The node.
/// @param args Positional arguments (attributes, child nodes, etc.).
/// @param kwargs Keyword arguments added as user variables.
/// @return The node cast to a Python object (for method chaining).
///
py::object Node_add_args_kwargs(node_ptr self, const py::args& args, const py::kwargs& kwargs) {
    NodeUtil::add(*self, args);
    NodeUtil::add(*self, kwargs);
    return py::cast(self);
}

///
/// @brief Implement dynamic attribute lookup on the node.
///
/// Looks up \p attr in order as: an immediate child node, a user variable,
/// a generated variable, an event, a meter, or a limit.
///
/// @param self The node.
/// @param attr The attribute name to look up.
/// @return The matching child node, variable, event, meter, or limit as a Python object.
/// @throws std::runtime_error if no match is found.
///
py::object Node_getattr(node_ptr self, const std::string& attr) {

    size_t pos = 0;
    if (node_ptr child = self->findImmediateChild(attr, pos); child) {
        return py::cast(child);
    }

    if (const Variable& var = self->findVariable(attr); !var.empty()) {
        return py::cast(var);
    }

    if (const Variable& gvar = self->findGenVariable(attr); !gvar.empty()) {
        return py::cast(gvar);
    }

    if (const Event& event = self->findEventByNameOrNumber(attr); !event.empty()) {
        return py::cast(event);
    }

    if (const Meter& meter = self->findMeter(attr); !meter.empty()) {
        return py::cast(meter);
    }

    if (limit_ptr limit = self->find_limit(attr); limit.get()) {
        return py::cast(limit);
    }

    throw std::runtime_error(MESSAGE("ExportNode::node_getattr: function of name '"
                                     << attr
                                     << "' does not exist *OR* child node, variable, meter, event or limit on node "
                                     << self->absNodePath()));
    return py::object();
}

///
/// @brief Replace the node on a server using the provided ClientInvoker.
///
/// Optionally suspends the node on the server before replacement and optionally
/// forces the replacement even when the server has a more recent version.
///
/// @param self The node to replace on the server.
/// @param theClient The client invoker used to communicate with the server.
/// @param suspend_node_first If true, suspend the node on the server before replacing.
/// @param force_replace If true, force replacement even if the server has a newer version.
///
void Node_replace_on_server_basic(node_ptr self,
                                  ClientInvoker& theClient,
                                  bool suspend_node_first,
                                  bool force_replace) {
    struct null_deleter
    {
        void operator()(void const*) const {}
    };

    // Hack! Create a std::shared_ptr<Defs> from a Defs*, avoiding double delete by using null_deleter
    defs_ptr defs = defs_ptr(self->defs(), null_deleter());

    bool create_parents_as_required = true;
    if (suspend_node_first) {
        theClient.suspend(self->absNodePath());
    }
    theClient.replace_1(self->absNodePath(), defs, create_parents_as_required, force_replace); // this can throw
}

///
/// @brief Replace the node on a server whose host and port are taken from the environment.
///
/// @param self The node to replace.
/// @param suspend_node_first If true, suspend the node on the server before replacing.
/// @param force_replace If true, force replacement even if the server has a newer version.
///
void Node_replace_on_server(node_ptr self, bool suspend_node_first, bool force_replace) {
    ClientInvoker theClient; // assume HOST and PORT found from environment
    Node_replace_on_server_basic(self, theClient, suspend_node_first, force_replace);
}

///
/// @brief Replace the node on a server identified by explicit host and port strings.
///
/// @param self The node to replace.
/// @param host The server hostname.
/// @param port The server port.
/// @param suspend_node_first If true, suspend the node on the server before replacing.
/// @param force_replace If true, force replacement even if the server has a newer version.
///
void Node_replace_on_server1(node_ptr self,
                             const std::string& host,
                             const std::string& port,
                             bool suspend_node_first,
                             bool force_replace) {
    ClientInvoker theClient(host, port);
    Node_replace_on_server_basic(self, theClient, suspend_node_first, force_replace);
}

///
/// @brief Replace the node on a server identified by a combined `"host:port"` string.
///
/// @param self The node to replace.
/// @param host_port A string of the form `"host:port"`.
/// @param suspend_node_first If true, suspend the node on the server before replacing.
/// @param force_replace If true, force replacement even if the server has a newer version.
///
void Node_replace_on_server2(node_ptr self, const std::string& host_port, bool suspend_node_first, bool force_replace) {
    ClientInvoker theClient(host_port);
    Node_replace_on_server_basic(self, theClient, suspend_node_first, force_replace);
}

///
/// @brief Return a pointer to the late attribute of the node, or nullptr if not set.
///
/// @param self The node.
/// @return A raw pointer to the LateAttr, or nullptr.
///
const ecf::LateAttr* Node_get_late(node_ptr self) {
    return self->get_late();
}

///
/// @brief Return a pointer to the auto-archive attribute of the node, or nullptr if not set.
///
/// @param self The node.
/// @return A raw pointer to the AutoArchiveAttr, or nullptr.
///
const ecf::AutoArchiveAttr* Node_get_autoarchive(node_ptr self) {
    return self->get_autoarchive();
}

///
/// @brief Return a pointer to the auto-cancel attribute of the node, or nullptr if not set.
///
/// @param self The node.
/// @return A raw pointer to the AutoCancelAttr, or nullptr.
///
const ecf::AutoCancelAttr* Node_get_autocancel(node_ptr self) {
    return self->get_autocancel();
}

///
/// @brief Return a pointer to the auto-restore attribute of the node, or nullptr if not set.
///
/// @param self The node.
/// @return A raw pointer to the AutoRestoreAttr, or nullptr.
///
const ecf::AutoRestoreAttr* Node_get_autorestore(node_ptr self) {
    return self->get_autorestore();
}

///
/// @brief Return a reference to the flag attribute of the node.
///
/// @param self The node.
/// @return A const reference to the node's Flag.
///
const ecf::Flag& Node_get_flag(node_ptr self) {
    return self->get_flag();
}

} // namespace

void export_Node(py::module& m) {

    py::class_<Node, std::shared_ptr<Node>>(m, "Node", py::dynamic_attr(), DefsDoc::node_doc())

        //
        // The '__lt__' is defined, instead of py::self < py::self, to avoid problems in older
        // compilers (e.g. gcc 10) which complain about Node being an abstract class and thus not
        // being able to determine the correct operator< overload
        //
        .def("__lt__", [](const Node& self, const Node& other) { return self < other; })

        // *** Operators ***

        .def("__add__", &Node_add, DefsDoc::add())
        .def("__iadd__", &Node_iadd)

        // *** Shift operators ***

        .def("__rshift__", &Node_rshift)
        // nc >> a >> b >> c     a + (b.add(Trigger('a==complete')) + (c.add(Trigger('b==complete')))

        .def("__lshift__", &Node_lshift)
        // nc << a << b << c     (a.add(Trigger('b==complete')) + (b.add(Trigger('c==complete'))) + c

        // *** Dynamic attributes ***

        .def("__getattr__", &Node_getattr)

        .def("name", &Node::name, py::return_value_policy::reference)

        .def("add", &Node_add_args_kwargs, DefsDoc::add())

        .def("remove", &Node::remove, "Remove the node from its parent. and returns it")

        .def("add_trigger", &Node_add_trigger, DefsDoc::add_trigger_doc())
        .def("add_trigger", &Node_add_trigger_expr)

        .def("add_complete", &Node_add_complete, DefsDoc::add_trigger_doc())
        .def("add_complete", &Node_add_complete_expr)

        .def("add_part_trigger", &Node_add_part_trigger, DefsDoc::add_trigger_doc())
        .def("add_part_trigger", &Node_add_part_trigger_1)
        .def("add_part_trigger", &Node_add_part_trigger_2)

        .def("add_part_complete", &Node_add_part_complete, DefsDoc::add_trigger_doc())
        .def("add_part_complete", &Node_add_part_complete_1)
        .def("add_part_complete", &Node_add_part_complete_2)

        .def("evaluate_trigger", &Node_evaluate_trigger, "evaluate trigger expression")

        .def("evaluate_complete", &Node_evaluate_complete, "evaluate complete expression")

        .def("add_variable", &Node_add_variable_string, DefsDoc::add_variable_doc())
        .def("add_variable", &Node_add_variable_int)
        .def("add_variable", &Node_add_variable_variable)
        .def("add_variable", &Node_add_variable_edit)
        .def("add_variable", &NodeUtil::add1)

        .def("add_label", &Node_add_label, DefsDoc::add_label_doc())
        .def("add_label", &Node_add_label_1)

        .def("add_aviso", &Node_add_aviso, DefsDoc::add_aviso_doc())

        .def("add_mirror", &Node_add_mirror, DefsDoc::add_mirror_doc())

        .def("add_limit", &Node_add_limit, DefsDoc::add_limit_doc())
        .def("add_limit", &Node_add_limit_1)

        .def("add_inlimit",
             &Node_add_in_limit,
             py::arg("limit_name"),
             py::arg("path_to_node_containing_limit") = "",
             py::arg("tokens")                        = 1,
             py::arg("limit_this_node_only")          = false,
             DefsDoc::add_inlimit_doc())
        .def("add_inlimit", &Node_add_in_limit_1)

        .def("add_event", &Node_add_event, DefsDoc::add_event_doc())
        .def("add_event", &Node_add_event_int)
        .def("add_event", &Node_add_event_string)
        .def("add_event", &Node_add_event_int_string)

        .def("add_meter", &Node_add_meter, DefsDoc::add_meter_doc())
        .def("add_meter", &Node_add_meter_1)
        .def("add_meter", &Node_add_meter_2)

        .def("add_queue", &Node_add_queue)
        .def("add_queue", &Node_add_queue1)

        .def("add_generic", &Node_add_generic)
        .def("add_generic", &Node_add_generic1)

        .def("add_date", &Node_add_date, DefsDoc::add_date_doc())
        .def("add_date", &Node_add_date_1)

        .def("add_day", &Node_add_day, DefsDoc::add_day_doc())
        .def("add_day", &Node_add_day_1)
        .def("add_day", &Node_add_day_2)

        .def("add_today", &Node_add_today, DefsDoc::add_today_doc())
        .def("add_today", &Node_add_today_1)
        .def("add_today", &Node_add_today_2)
        .def("add_today", &Node_add_today_3)

        .def("add_time", &Node_add_time, DefsDoc::add_time_doc())
        .def("add_time", &Node_add_time_1)
        .def("add_time", &Node_add_time_2)
        .def("add_time", &Node_add_time_3)

        .def("add_cron", &Node_add_cron, DefsDoc::add_cron_doc())

        .def("add_late", &Node_add_late, DefsDoc::add_late_doc())

        .def("add_autocancel", &Node_add_autocancel, DefsDoc::add_autocancel_doc())
        .def("add_autocancel", &Node_add_autocancel_1)
        .def("add_autocancel", &Node_add_autocancel_2)
        .def("add_autocancel", &Node_add_autocancel_3)

        .def("add_autoarchive",
             &Node_add_autoarchive,
             py::arg("days"),
             py::arg("idle") = false,
             DefsDoc::add_autoarchive_doc())
        .def("add_autoarchive",
             &Node_add_autoarchive_1,
             py::arg("hour"),
             py::arg("min"),
             py::arg("relative"),
             py::arg("idle") = false)
        .def("add_autoarchive",
             &Node_add_autoarchive_2,
             py::arg("TimeSlot"),
             py::arg("relative"),
             py::arg("idle") = false)
        .def("add_autoarchive", &Node_add_autoarchive_3)

        .def("add_autorestore", &Node_add_autorestore, DefsDoc::add_autorestore_doc())
        .def("add_autorestore", &Node_add_autorestore1)

        .def("add_verify", &Node::addVerify, DefsDoc::add_verify_doc())

        .def("add_repeat", &Node_add_repeat_date, DefsDoc::add_repeat_date_doc())
        .def("add_repeat", &Node_add_repeat_datetime, DefsDoc::add_repeat_datetime_doc())
        .def("add_repeat", &Node_add_repeat_date_list, DefsDoc::add_repeat_date_list_doc())
        .def("add_repeat", &Node_add_repeat_integer, DefsDoc::add_repeat_integer_doc())
        .def("add_repeat", &Node_add_repeat_string, DefsDoc::add_repeat_string_doc())
        .def("add_repeat", &Node_add_repeat_enum, DefsDoc::add_repeat_enumerated_doc())
        .def("add_repeat", &Node_add_repeat_day, DefsDoc::add_repeat_day_doc())

        .def("add_defstatus", &Node_add_defstatus, DefsDoc::add_defstatus_doc())
        .def("add_defstatus", &Node_add_defstatus1, DefsDoc::add_defstatus_doc())

        .def("add_zombie", &Node_add_zombie, NodeAttrDoc::zombie_doc())

        .def("delete_variable", &Node::deleteVariable)

        .def("delete_event", &Node::deleteEvent)

        .def("delete_meter", &Node::deleteMeter)

        .def("delete_label", &Node::deleteLabel)

        .def("delete_queue", &Node::delete_queue)

        .def("delete_generic", &Node::delete_generic)

        .def("delete_trigger", &Node::deleteTrigger)

        .def("delete_complete", &Node::deleteComplete)

        .def("delete_repeat", &Node::deleteRepeat)

        .def("delete_limit", &Node::deleteLimit)

        .def("delete_inlimit", &Node::deleteInlimit)

        .def("delete_time", &Node::deleteTime)
        .def("delete_time", &Node::delete_time)

        .def("delete_today", &Node::deleteToday)
        .def("delete_today", &Node::delete_today)

        .def("delete_date", &Node::deleteDate)
        .def("delete_date", &Node::delete_date)

        .def("delete_day", &Node::deleteDay)
        .def("delete_day", &Node::delete_day)

        .def("delete_cron", &Node::deleteCron)
        .def("delete_cron", &Node::delete_cron)

        .def("delete_zombie", &Node::deleteZombie)
        .def("delete_zombie", &Node::delete_zombie)

        .def("change_trigger", &Node::changeTrigger)

        .def("change_complete", &Node::changeComplete)

        .def("sort_attributes", &Node_sort_attributes)
        .def("sort_attributes", &Node_sort_attributes1)
        .def("sort_attributes", &Node_sort_attributes2)
        .def("sort_attributes",
             &Node_sort_attributes3,
             py::arg("attribute_type"),
             py::arg("recursive") = true,
             py::arg("no_sort")   = py::list())
        .def("sort_attributes",
             &Node::sort_attributes,
             py::arg("attribute_type"),
             py::arg("recursive") = true,
             py::arg("no_sort")   = py::list())

        .def("get_abs_node_path", &Node::absNodePath, DefsDoc::abs_node_path_doc())

        .def("has_time_dependencies", &Node::hasTimeDependencies)

        .def("update_generated_variables", &Node::update_generated_variables)

        .def("get_generated_variables",
             &Node_generated_variables_using_python_list,
             "Returns the list of generated variables.")
        .def("get_generated_variables",
             &Node_generated_variables_using_variable_list,
             "Retrieves the list of generated variables. Pass in ecflow.VariableList as argument to hold variables.")

        .def("is_suspended", &Node::isSuspended, "Returns true if the `node`_ is in a `suspended`_ state")

        .def("find_variable",
             &Node::findVariable,
             py::return_value_policy::reference,
             "Find user variable on the node only. Returns an object")

        .def("find_gen_variable",
             &Node::findGenVariable,
             py::return_value_policy::reference,
             "Find generated variable on the node only. Returns an object")

        .def("find_parent_variable",
             &Node::find_parent_variable,
             py::return_value_policy::reference,
             "Find user variable variable up the parent hierarchy. Returns an object")

        .def("find_parent_variable_sub_value",
             &Node::find_parent_variable_sub_value,
             "Find user variable *up* node tree, then variable substitute the value, otherwise return empty string")

        .def("find_meter",
             &Node::findMeter,
             py::return_value_policy::reference,
             "Find the `meter`_ on the node only. Returns an object")

        .def("find_event",
             &Node::findEventByNameOrNumber,
             py::return_value_policy::reference,
             "Find the `event`_ on the node only. Returns a object")

        .def("find_label",
             &Node::find_label,
             py::return_value_policy::reference,
             "Find the `label`_ on the node only. Returns a object")

        .def("find_queue",
             &Node::find_queue,
             py::return_value_policy::reference,
             "Find the queue on the node only. Returns a queue object")

        .def("find_generic",
             &Node::find_generic,
             py::return_value_policy::reference,
             "Find the `generic`_ on the node only. Returns a Generic object")

        .def("find_limit", &Node::find_limit, "Find the `limit`_ on the node only. returns a limit ptr")

        .def("find_node_up_the_tree", &Node::find_node_up_the_tree, "Search immediate node, then up the node hierarchy")

        .def("get_state", &Node::state, "Returns the state of the node. This excludes the suspended state")

        .def("get_state_change_time",
             &Node_get_state_change_time,
             py::arg("format") = "iso_extended",
             "Returns the time of the last state change as a string. Default format is iso_extended, (iso_extended, "
             "iso, simple)")

        .def("get_dstate", &Node::dstate, "Returns the state of node. This will include suspended state")

        .def("get_defstatus", &Node::defStatus)

        .def("get_repeat", &Node::repeat, py::return_value_policy::reference)

        .def("get_late", &Node_get_late, py::return_value_policy::reference_internal)

        .def("get_autocancel", &Node_get_autocancel, py::return_value_policy::reference_internal)

        .def("get_autoarchive", &Node_get_autoarchive, py::return_value_policy::reference_internal)

        .def("get_autorestore", &Node_get_autorestore, py::return_value_policy::reference_internal)

        .def("get_trigger", &Node::get_trigger, py::return_value_policy::reference_internal)

        .def("get_complete", &Node::get_complete, py::return_value_policy::reference_internal)

        .def("get_defs", Node_get, py::return_value_policy::reference_internal)

        .def("get_parent", &Node::parent, py::return_value_policy::reference_internal)

        .def("get_all_nodes", &Node_get_all_nodes, "Returns all the child nodes")

        .def("get_flag",
             &Node_get_flag,
             py::return_value_policy::reference,
             "Return additional state associated with a node.")

        .def("replace_on_server",
             &Node_replace_on_server,
             py::arg("suspend_node_first") = true,
             py::arg("force")              = true,
             "replace node on the server.")
        .def("replace_on_server",
             &Node_replace_on_server1,
             py::arg("host"),
             py::arg("port"),
             py::arg("suspend_node_first") = true,
             py::arg("force")              = true,
             "replace node on the server.")
        .def("replace_on_server",
             &Node_replace_on_server2,
             py::arg("host_port"),
             py::arg("suspend_node_first") = true,
             py::arg("force")              = true,
             "replace node on the server.")
        .def("replace_on_server",
             &Node_replace_on_server_basic,
             py::arg("client"),
             py::arg("suspend_node_first") = true,
             py::arg("force")              = true,
             "replace node on the server.")

        .def_property_readonly("meters", &Node::meters, "Returns a list of `meter`_\\ s")

        .def_property_readonly("events", &Node::events, "Returns a list of `event`_\\ s")

        .def_property_readonly("variables", &Node::variables, "Returns a list of user defined `variable`_\\ s")

        .def_property_readonly("labels", &Node::labels, "Returns a list of `label`_\\ s")

        .def_property_readonly("avisos",
                               static_cast<const std::vector<ecf::AvisoAttr>& (Node::*)() const>(&Node::avisos),
                               py::return_value_policy::reference_internal,
                               "Returns a list of `aviso`_\\ s")

        .def_property_readonly("mirrors",
                               static_cast<const std::vector<ecf::MirrorAttr>& (Node::*)() const>(&Node::mirrors),
                               py::return_value_policy::reference_internal,
                               "Returns a list of `mirror`_\\ s")

        .def_property_readonly("limits", &Node::limits, "Returns a list of `limit`_\\ s")

        .def_property_readonly("inlimits", &Node::inlimits, "Returns a list of `inlimit`_\\ s")

        .def_property_readonly("verifies", &Node::verifys, "Returns a list of Verify's")

        .def_property_readonly("times", &Node::timeVec, "Returns a list of `time`_\\ s")

        .def_property_readonly("todays", &Node::todayVec, "Returns a list of `today`_\\ s")

        .def_property_readonly("dates", &Node::dates, "Returns a list of `date`_\\ s")

        .def_property_readonly("days", &Node::days, "Returns a list of `day`_\\ s")

        .def_property_readonly("crons", &Node::crons, "Returns a list of `cron`_\\ s")

        .def_property_readonly("zombies", &Node::zombies, "Returns a list of `zombie`_\\ s")

        .def_property_readonly("queues", &Node::queues, "Returns a list of `queue`_\\ s")

        .def_property_readonly("generics", &Node::generics, "Returns a list of `generic`_\\ s");
}
