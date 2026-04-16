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

Defs* Node_get(node_ptr self) {
    return self->defs();
}

node_ptr Node_add_variable_string(node_ptr self, const std::string& name, const std::string& value) {
    self->add_variable(name, value);
    return self;
}

node_ptr Node_add_variable_int(node_ptr self, const std::string& name, int value) {
    self->add_variable_int(name, value);
    return self;
}

node_ptr Node_add_variable_variable(node_ptr self, const Variable& var) {
    self->addVariable(var);
    return self;
}

node_ptr Node_add_variable_edit(node_ptr self, const Edit& edit) {
    for (const auto& var : edit.variables()) {
        self->addVariable(var);
    }
    return self;
}

node_ptr Node_add_event(node_ptr self, const Event& e) {
    self->addEvent(e);
    return self;
}

node_ptr Node_add_event_int(node_ptr self, int number) {
    self->addEvent(Event(number));
    return self;
}

node_ptr Node_add_event_string(node_ptr self, const std::string& name) {
    self->addEvent(Event(name));
    return self;
}

node_ptr Node_add_event_int_string(node_ptr self, int number, const std::string& name) {
    self->addEvent(Event(number, name));
    return self;
}

node_ptr Node_add_meter(node_ptr self, const Meter& m) {
    self->addMeter(m);
    return self;
}

node_ptr Node_add_meter_1(node_ptr self, const std::string& meter_name, int min, int max, int color_change) {
    self->addMeter(Meter(meter_name, min, max, color_change));
    return self;
}

node_ptr Node_add_meter_2(node_ptr self, const std::string& meter_name, int min, int max) {
    self->addMeter(Meter(meter_name, min, max));
    return self;
}

node_ptr Node_add_queue(node_ptr self, const QueueAttr& m) {
    self->add_queue(m);
    return self;
}

node_ptr Node_add_queue1(node_ptr self, const std::string& name, const py::list& list) {
    std::vector<std::string> vec;
    pyutil_list_to_str_vec(list, vec);
    QueueAttr queue_attr(name, vec);
    self->add_queue(queue_attr);
    return self;
}

node_ptr Node_add_generic(node_ptr self, const GenericAttr& m) {
    self->add_generic(m);
    return self;
}

node_ptr Node_add_generic1(node_ptr self, const std::string& name, const py::list& list) {
    std::vector<std::string> vec;
    pyutil_list_to_str_vec(list, vec);
    GenericAttr attr(name, vec);
    self->add_generic(attr);
    return self;
}

node_ptr Node_add_label(node_ptr self, const std::string& name, const std::string& value) {
    self->addLabel(Label(name, value));
    return self;
}

node_ptr Node_add_label_1(node_ptr self, const Label& label) {
    self->addLabel(label);
    return self;
}

node_ptr Node_add_aviso(node_ptr self, const ecf::AvisoAttr& attr) {
    self->addAviso(attr);
    return self;
}

node_ptr Node_add_mirror(node_ptr self, const ecf::MirrorAttr& attr) {
    self->addMirror(attr);
    return self;
}

node_ptr Node_add_limit(node_ptr self, const std::string& name, int limit) {
    self->addLimit(Limit(name, limit));
    return self;
}

node_ptr Node_add_limit_1(node_ptr self, const Limit& limit) {
    self->addLimit(limit);
    return self;
}

node_ptr Node_add_in_limit(node_ptr self,
                           const std::string& name,
                           const std::string& pathToNode,
                           int tokens,
                           bool limit_this_node_only) {
    self->addInLimit(InLimit(name, pathToNode, tokens, limit_this_node_only));
    return self;
}
node_ptr Node_add_in_limit_1(node_ptr self, const InLimit& inlimit) {
    self->addInLimit(inlimit);
    return self;
}

node_ptr Node_add_time(node_ptr self, int hour, int minute) {
    self->addTime(ecf::TimeAttr(hour, minute));
    return self;
}

node_ptr Node_add_time_1(node_ptr self, int hour, int minute, bool relative) {
    self->addTime(ecf::TimeAttr(hour, minute, relative));
    return self;
}

node_ptr Node_add_time_2(node_ptr self, const std::string& ts) {
    self->addTime(ecf::TimeAttr(ecf::TimeSeries::create(ts)));
    return self;
}

node_ptr Node_add_time_3(node_ptr self, const ecf::TimeAttr& ts) {
    self->addTime(ts);
    return self;
}

node_ptr Node_add_today(node_ptr self, int hour, int minute) {
    self->addToday(ecf::TodayAttr(hour, minute));
    return self;
}

node_ptr Node_add_today_1(node_ptr self, int hour, int minute, bool relative) {
    self->addToday(ecf::TodayAttr(hour, minute, relative));
    return self;
}

node_ptr Node_add_today_2(node_ptr self, const std::string& ts) {
    self->addToday(ecf::TodayAttr(ecf::TimeSeries::create(ts)));
    return self;
}

node_ptr Node_add_today_3(node_ptr self, const ecf::TodayAttr& ts) {
    self->addToday(ts);
    return self;
}

node_ptr Node_add_date(node_ptr self, int day, int month, int year) {
    self->addDate(DateAttr(day, month, year));
    return self;
}

node_ptr Node_add_date_1(node_ptr self, const DateAttr& d) {
    self->addDate(d);
    return self;
}

node_ptr Node_add_day(node_ptr self, DayAttr::Day_t day) {
    self->addDay(DayAttr(day));
    return self;
}

node_ptr Node_add_day_1(node_ptr self, const std::string& day) {
    self->addDay(DayAttr(DayAttr::getDay(day)));
    return self;
}

node_ptr Node_add_day_2(node_ptr self, const DayAttr& day) {
    self->addDay(day);
    return self;
}

node_ptr Node_add_autocancel(node_ptr self, int days) {
    self->addAutoCancel(ecf::AutoCancelAttr(days));
    return self;
}

node_ptr Node_add_autocancel_1(node_ptr self, int hour, int min, bool relative) {
    self->addAutoCancel(ecf::AutoCancelAttr(hour, min, relative));
    return self;
}

node_ptr Node_add_autocancel_2(node_ptr self, const ecf::TimeSlot& ts, bool relative) {
    self->addAutoCancel(ecf::AutoCancelAttr(ts, relative));
    return self;
}

node_ptr Node_add_autocancel_3(node_ptr self, const ecf::AutoCancelAttr& attr) {
    self->addAutoCancel(attr);
    return self;
}

node_ptr Node_add_autoarchive(node_ptr self, int days, bool idle) {
    self->add_autoarchive(ecf::AutoArchiveAttr(days, idle));
    return self;
}

node_ptr Node_add_autoarchive_1(node_ptr self, int hour, int min, bool relative, bool idle) {
    self->add_autoarchive(ecf::AutoArchiveAttr(hour, min, relative, idle));
    return self;
}

node_ptr Node_add_autoarchive_2(node_ptr self, const ecf::TimeSlot& ts, bool relative, bool idle) {
    self->add_autoarchive(ecf::AutoArchiveAttr(ts, relative, idle));
    return self;
}

node_ptr Node_add_autoarchive_3(node_ptr self, const ecf::AutoArchiveAttr& attr) {
    self->add_autoarchive(attr);
    return self;
}

node_ptr Node_add_zombie(node_ptr self, const ZombieAttr& attr) {
    self->addZombie(attr);
    return self;
}

node_ptr Node_add_autorestore(node_ptr self, const ecf::AutoRestoreAttr& attr) {
    self->add_autorestore(attr);
    return self;
}

node_ptr Node_add_autorestore1(node_ptr self, const py::list& list) {
    std::vector<std::string> vec;
    pyutil_list_to_str_vec(list, vec);
    self->add_autorestore(ecf::AutoRestoreAttr(vec));
    return self;
}

node_ptr Node_add_cron(node_ptr self, const ecf::CronAttr& attr) {
    self->addCron(attr);
    return self;
}

node_ptr Node_add_late(node_ptr self, const ecf::LateAttr& attr) {
    self->addLate(attr);
    return self;
}

std::string Node_get_state_change_time(node_ptr self, const std::string& format) {
    if (format == "iso_extended") {
        return to_iso_extended_string(self->state_change_time());
    }
    else if (format == "iso") {
        return to_iso_string(self->state_change_time());
    }
    return to_simple_string(self->state_change_time());
}

node_ptr Node_add_repeat_date(node_ptr self, const RepeatDate& d) {
    self->addRepeat(d);
    return self;
}

node_ptr Node_add_repeat_datetime(node_ptr self, const RepeatDateTime& d) {
    self->addRepeat(d);
    return self;
}

node_ptr Node_add_repeat_date_list(node_ptr self, const RepeatDateList& d) {
    self->addRepeat(d);
    return self;
}

node_ptr Node_add_repeat_integer(node_ptr self, const RepeatInteger& d) {
    self->addRepeat(d);
    return self;
}

node_ptr Node_add_repeat_string(node_ptr self, const RepeatString& d) {
    self->addRepeat(d);
    return self;
}

node_ptr Node_add_repeat_enum(node_ptr self, const RepeatEnumerated& d) {
    self->addRepeat(d);
    return self;
}

node_ptr Node_add_repeat_day(node_ptr self, const RepeatDay& d) {
    self->addRepeat(d);
    return self;
}

void Node_sort_attributes(node_ptr self, ecf::Attr::Type attr) {
    self->sort_attributes(attr);
}

void Node_sort_attributes1(node_ptr self, ecf::Attr::Type attr, bool recursive) {
    self->sort_attributes(attr, recursive);
}

void Node_sort_attributes2(node_ptr self, ecf::Attr::Type attr, bool recursive, const py::list& list) {
    std::vector<std::string> no_sort;
    pyutil_list_to_str_vec(list, no_sort);
    self->sort_attributes(attr, recursive, no_sort);
}

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

std::vector<node_ptr> Node_get_all_nodes(node_ptr self) {
    return ecf::get_all_nodes_ptr(self);
}

node_ptr Node_add_trigger(node_ptr self, const std::string& expr) {
    self->add_trigger(expr);
    return self;
}

node_ptr Node_add_trigger_expr(node_ptr self, const Expression& expr) {
    self->add_trigger_expr(expr);
    return self;
}

node_ptr Node_add_complete(node_ptr self, const std::string& expr) {
    self->add_complete(expr);
    return self;
}

node_ptr Node_add_complete_expr(node_ptr self, const Expression& expr) {
    self->add_complete_expr(expr);
    return self;
}

node_ptr Node_add_part_trigger(node_ptr self, const PartExpression& expr) {
    self->add_part_trigger(PartExpression(expr));
    return self;
}

node_ptr Node_add_part_trigger_1(node_ptr self, const std::string& expression) {
    self->add_part_trigger(PartExpression(expression));
    return self;
}

node_ptr Node_add_part_trigger_2(node_ptr self, const std::string& expression, bool and_expr) {
    self->add_part_trigger(PartExpression(expression, and_expr));
    return self;
}

node_ptr Node_add_part_complete(node_ptr self, const PartExpression& expr) {
    self->add_part_complete(PartExpression(expr));
    return self;
}

node_ptr Node_add_part_complete_1(node_ptr self, const std::string& expression) {
    self->add_part_complete(PartExpression(expression));
    return self;
}

node_ptr Node_add_part_complete_2(node_ptr self, const std::string& expression, bool and_expr) {
    self->add_part_complete(PartExpression(expression, and_expr));
    return self;
}

bool Node_evaluate_trigger(node_ptr self) {
    Ast* t = self->triggerAst();
    if (t) {
        return t->evaluate();
    }
    return false;
}

bool Node_evaluate_complete(node_ptr self) {
    Ast* t = self->completeAst();
    if (t) {
        return t->evaluate();
    }
    return false;
}

node_ptr Node_add_defstatus(node_ptr self, DState::State s) {
    self->addDefStatus(s);
    return self;
}
node_ptr Node_add_defstatus1(node_ptr self, const Defstatus& ds) {
    self->addDefStatus(ds.state());
    return self;
}

py::list Node_generated_variables_using_python_list(node_ptr self) {
    py::list list;
    std::vector<Variable> vec;
    self->gen_variables(vec);
    for (const auto& i : vec) {
        list.append(i);
    }
    return list;
}

void Node_generated_variables_using_variable_list(node_ptr self, std::vector<Variable>& vec) {
    self->gen_variables(vec);
}

py::object Node_add(node_ptr self, const py::object& arg) {
    NodeUtil::add(*self, arg);
    return py::cast(self);
}

py::object Node_iadd(node_ptr self, const py::object& arg) {
    NodeUtil::add(*self, arg);
    return py::cast(self);
}

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

py::object Node_add_args_kwargs(node_ptr self, const py::args& args, const py::kwargs& kwargs) {
    NodeUtil::add(*self, args);
    NodeUtil::add(*self, kwargs);
    return py::cast(self);
}

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
void Node_replace_on_server(node_ptr self, bool suspend_node_first, bool force_replace) {
    ClientInvoker theClient; // assume HOST and PORT found from environment
    Node_replace_on_server_basic(self, theClient, suspend_node_first, force_replace);
}
void Node_replace_on_server1(node_ptr self,
                             const std::string& host,
                             const std::string& port,
                             bool suspend_node_first,
                             bool force_replace) {
    ClientInvoker theClient(host, port);
    Node_replace_on_server_basic(self, theClient, suspend_node_first, force_replace);
}
void Node_replace_on_server2(node_ptr self, const std::string& host_port, bool suspend_node_first, bool force_replace) {
    ClientInvoker theClient(host_port);
    Node_replace_on_server_basic(self, theClient, suspend_node_first, force_replace);
}

const ecf::LateAttr* Node_get_late(node_ptr self) {
    return self->get_late();
}

const ecf::AutoArchiveAttr* Node_get_autoarchive(node_ptr self) {
    return self->get_autoarchive();
}

const ecf::AutoCancelAttr* Node_get_autocancel(node_ptr self) {
    return self->get_autocancel();
}

const ecf::AutoRestoreAttr* Node_get_autorestore(node_ptr self) {
    return self->get_autorestore();
}

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
