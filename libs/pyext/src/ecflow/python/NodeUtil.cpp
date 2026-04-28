/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/python/NodeUtil.hpp"

#include <stdexcept>

#include "ecflow/attribute/AutoArchiveAttr.hpp"
#include "ecflow/attribute/AutoCancelAttr.hpp"
#include "ecflow/attribute/ClockAttr.hpp"
#include "ecflow/attribute/CronAttr.hpp"
#include "ecflow/attribute/DateAttr.hpp"
#include "ecflow/attribute/DayAttr.hpp"
#include "ecflow/attribute/LateAttr.hpp"
#include "ecflow/attribute/NodeAttr.hpp"
#include "ecflow/attribute/RepeatAttr.hpp"
#include "ecflow/attribute/TimeAttr.hpp"
#include "ecflow/attribute/TodayAttr.hpp"
#include "ecflow/attribute/Variable.hpp"
#include "ecflow/attribute/VerifyAttr.hpp"
#include "ecflow/attribute/ZombieAttr.hpp"
#include "ecflow/node/Attr.hpp"
#include "ecflow/node/AutoRestoreAttr.hpp"
#include "ecflow/node/AvisoAttr.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Flag.hpp"
#include "ecflow/node/InLimit.hpp"
#include "ecflow/node/Limit.hpp"
#include "ecflow/node/MirrorAttr.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/python/Edit.hpp"
#include "ecflow/python/PythonUtil.hpp"
#include "ecflow/python/Trigger.hpp"

py::object NodeUtil::node_raw_constructor(py::tuple args, py::dict kw) {
    // cout << "node_raw_constructor len(args):" << len(args) << endl;
    // args[0] is Task(i.e self) args[1] is string name
    py::list the_list;
    std::string name;
    for (int i = 1; i < len(args); ++i) {
        if (auto extracted = py::extract<std::string>(args[i]); extracted.check()) {
            name = extracted();
        }
        else {
            the_list.append(args[i]);
        }
    }
    if (name.empty()) {
        throw std::runtime_error("node_raw_constructor: first argument must be a string");
    }
    return args[0].attr("__init__")(name, the_list, kw); // calls -> init(const std::string& name, list attr, dict kw)
}

node_ptr NodeUtil::add_variable_dict(node_ptr self, const py::dict& dict) {
    std::vector<Variable> vec;
    pyutil_dict_to_str_vec(dict, vec);
    for (const auto& i : vec) {
        self->addVariable(i);
    }
    return self;
}

py::object NodeUtil::node_iadd(node_ptr self, const py::list& list) {
    // std::cout << "node_iadd list " << self->name() << "\n";
    int the_list_size = len(list);
    for (int i = 0; i < the_list_size; ++i) {
        (void)do_add(self, list[i]);
    }
    return py::object(self); // return node_ptr as python object, relies class_<Node>... for type registration
}

py::object NodeUtil::do_add(node_ptr self, const py::object& arg) {
    // std::cout << "do_add " << self->name() << "\n";
    if (arg.ptr() == py::object().ptr()) {
        return py::object(self); // *IGNORE* None
    }

    if (auto extracted = py::extract<Edit>(arg); extracted.check()) {
        Edit edit                        = extracted();
        const std::vector<Variable>& vec = edit.variables();
        for (const auto& i : vec) {
            self->addVariable(i);
        }
    }
    else if (auto extracted = py::extract<node_ptr>(arg); extracted.check()) {
        // std::cout << "  do_add node_ptr\n";
        NodeContainer* nc = self->isNodeContainer();
        if (!nc) {
            throw std::runtime_error("ExportNode::add() : Can only add a child to Suite or Family");
        }
        nc->addChild(extracted());
    }
    else if (auto extracted = py::extract<Event>(arg); extracted.check()) {
        self->addEvent(extracted());
    }
    else if (auto extracted = py::extract<Meter>(arg); extracted.check()) {
        self->addMeter(extracted());
    }
    else if (auto extracted = py::extract<Label>(arg); extracted.check()) {
        self->addLabel(extracted());
    }
    else if (auto extracted = py::extract<Trigger>(arg); extracted.check()) {
        Trigger t = extracted();
        self->py_add_trigger_expr(t.expr());
    }
    else if (auto extracted = py::extract<Complete>(arg); extracted.check()) {
        Complete t = extracted();
        self->py_add_complete_expr(t.expr());
    }
    else if (auto extracted = py::extract<Limit>(arg); extracted.check()) {
        self->addLimit(extracted());
    }
    else if (auto extracted = py::extract<InLimit>(arg); extracted.check()) {
        self->addInLimit(extracted());
    }
    else if (auto extracted = py::extract<DayAttr>(arg); extracted.check()) {
        self->addDay(extracted());
    }
    else if (auto extracted = py::extract<DateAttr>(arg); extracted.check()) {
        self->addDate(extracted());
    }
    else if (auto extracted = py::extract<ecf::TodayAttr>(arg); extracted.check()) {
        self->addToday(extracted());
    }
    else if (auto extracted = py::extract<ecf::TimeAttr>(arg); extracted.check()) {
        self->addTime(extracted());
    }
    else if (auto extracted = py::extract<ecf::CronAttr>(arg); extracted.check()) {
        self->addCron(extracted());
    }
    else if (auto extracted = py::extract<ecf::LateAttr>(arg); extracted.check()) {
        self->addLate(extracted());
    }
    else if (auto extracted = py::extract<ZombieAttr>(arg); extracted.check()) {
        self->addZombie(extracted());
    }
    else if (auto extracted = py::extract<RepeatDate>(arg); extracted.check()) {
        self->addRepeat(Repeat(extracted()));
    }
    else if (auto extracted = py::extract<RepeatDateTime>(arg); extracted.check()) {
        self->addRepeat(Repeat(extracted()));
    }
    else if (auto extracted = py::extract<RepeatDateList>(arg); extracted.check()) {
        self->addRepeat(Repeat(extracted()));
    }
    else if (auto extracted = py::extract<RepeatDateTimeList>(arg); extracted.check()) {
        self->addRepeat(Repeat(extracted()));
    }
    else if (auto extracted = py::extract<RepeatInteger>(arg); extracted.check()) {
        self->addRepeat(Repeat(extracted()));
    }
    else if (auto extracted = py::extract<RepeatEnumerated>(arg); extracted.check()) {
        self->addRepeat(Repeat(extracted()));
    }
    else if (auto extracted = py::extract<RepeatString>(arg); extracted.check()) {
        self->addRepeat(Repeat(extracted()));
    }
    else if (auto extracted = py::extract<RepeatDay>(arg); extracted.check()) {
        self->addRepeat(Repeat(extracted()));
    }
    else if (auto extracted = py::extract<ecf::AutoCancelAttr>(arg); extracted.check()) {
        self->addAutoCancel(extracted());
    }
    else if (auto extracted = py::extract<Defstatus>(arg); extracted.check()) {
        Defstatus t = extracted();
        self->addDefStatus(t.state());
    }
    else if (auto extracted = py::extract<ecf::AutoArchiveAttr>(arg); extracted.check()) {
        self->add_autoarchive(extracted());
    }
    else if (auto extracted = py::extract<ecf::AutoRestoreAttr>(arg); extracted.check()) {
        self->add_autorestore(extracted());
    }
    else if (auto extracted = py::extract<VerifyAttr>(arg); extracted.check()) {
        self->addVerify(extracted());
    }
    else if (auto extracted = py::extract<py::list>(arg); extracted.check()) {
        // std::cout << "  do_add list\n";
        py::list the_list = extracted();
        int the_list_size = len(the_list);
        for (int i = 0; i < the_list_size; ++i) {
            (void)do_add(self, the_list[i]); // recursive
        }
    }
    else if (auto extracted = py::extract<ClockAttr>(arg); extracted.check()) {
        if (!self->isSuite()) {
            throw std::runtime_error("ExportNode::add() : Can only add a clock to a suite");
        }
        self->isSuite()->addClock(extracted());
    }
    else if (auto extracted = py::extract<Variable>(arg); extracted.check()) {
        self->addVariable(extracted());
    }
    else if (auto attr = py::extract<ecf::AvisoAttr>(arg); attr.check()) {
        self->addAviso(attr);
    }
    else if (auto attr = py::extract<ecf::MirrorAttr>(arg); attr.check()) {
        self->addMirror(attr);
    }
    else if (py::extract<py::dict>(arg).check()) {
        py::dict d = py::extract<py::dict>(arg);
        add_variable_dict(self, d);
    }
    else {
        throw std::runtime_error("ExportNode::add : Unknown type ");
    }
    return py::object(self);
}
