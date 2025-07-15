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
        if (py::extract<std::string>(args[i]).check()) {
            name = py::extract<std::string>(args[i]);
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

    if (py::extract<Edit>(arg).check()) {
        Edit edit                        = py::extract<Edit>(arg);
        const std::vector<Variable>& vec = edit.variables();
        for (const auto& i : vec) {
            self->addVariable(i);
        }
    }
    else if (py::extract<node_ptr>(arg).check()) {
        // std::cout << "  do_add node_ptr\n";
        NodeContainer* nc = self->isNodeContainer();
        if (!nc) {
            throw std::runtime_error("ExportNode::add() : Can only add a child to Suite or Family");
        }
        node_ptr child = py::extract<node_ptr>(arg);
        nc->addChild(child);
    }
    else if (py::extract<Event>(arg).check()) {
        self->addEvent(py::extract<Event>(arg));
    }
    else if (py::extract<Meter>(arg).check()) {
        self->addMeter(py::extract<Meter>(arg));
    }
    else if (py::extract<Label>(arg).check()) {
        self->addLabel(py::extract<Label>(arg));
    }
    else if (py::extract<Trigger>(arg).check()) {
        Trigger t = py::extract<Trigger>(arg);
        self->py_add_trigger_expr(t.expr());
    }
    else if (py::extract<Complete>(arg).check()) {
        Complete t = py::extract<Complete>(arg);
        self->py_add_complete_expr(t.expr());
    }
    else if (py::extract<Limit>(arg).check()) {
        self->addLimit(py::extract<Limit>(arg));
    }
    else if (py::extract<InLimit>(arg).check()) {
        self->addInLimit(py::extract<InLimit>(arg));
    }
    else if (py::extract<DayAttr>(arg).check()) {
        self->addDay(py::extract<DayAttr>(arg));
    }
    else if (py::extract<DateAttr>(arg).check()) {
        self->addDate(py::extract<DateAttr>(arg));
    }
    else if (py::extract<ecf::TodayAttr>(arg).check()) {
        self->addToday(py::extract<ecf::TodayAttr>(arg));
    }
    else if (py::extract<ecf::TimeAttr>(arg).check()) {
        self->addTime(py::extract<ecf::TimeAttr>(arg));
    }
    else if (py::extract<ecf::CronAttr>(arg).check()) {
        self->addCron(py::extract<ecf::CronAttr>(arg));
    }
    else if (py::extract<ecf::LateAttr>(arg).check()) {
        self->addLate(py::extract<ecf::LateAttr>(arg));
    }
    else if (py::extract<ZombieAttr>(arg).check()) {
        self->addZombie(py::extract<ZombieAttr>(arg));
    }
    else if (py::extract<RepeatDate>(arg).check()) {
        self->addRepeat(Repeat(py::extract<RepeatDate>(arg)));
    }
    else if (py::extract<RepeatDateTime>(arg).check()) {
        self->addRepeat(Repeat(py::extract<RepeatDateTime>(arg)));
    }
    else if (py::extract<RepeatDateList>(arg).check()) {
        self->addRepeat(Repeat(py::extract<RepeatDateList>(arg)));
    }
    else if (py::extract<RepeatInteger>(arg).check()) {
        self->addRepeat(Repeat(py::extract<RepeatInteger>(arg)));
    }
    else if (py::extract<RepeatEnumerated>(arg).check()) {
        self->addRepeat(Repeat(py::extract<RepeatEnumerated>(arg)));
    }
    else if (py::extract<RepeatString>(arg).check()) {
        self->addRepeat(Repeat(py::extract<RepeatString>(arg)));
    }
    else if (py::extract<RepeatDay>(arg).check()) {
        self->addRepeat(Repeat(py::extract<RepeatDay>(arg)));
    }
    else if (py::extract<ecf::AutoCancelAttr>(arg).check()) {
        self->addAutoCancel(py::extract<ecf::AutoCancelAttr>(arg));
    }
    else if (py::extract<Defstatus>(arg).check()) {
        Defstatus t = py::extract<Defstatus>(arg);
        self->addDefStatus(t.state());
    }
    else if (py::extract<ecf::AutoArchiveAttr>(arg).check()) {
        self->add_autoarchive(py::extract<ecf::AutoArchiveAttr>(arg));
    }
    else if (py::extract<ecf::AutoRestoreAttr>(arg).check()) {
        self->add_autorestore(py::extract<ecf::AutoRestoreAttr>(arg));
    }
    else if (py::extract<VerifyAttr>(arg).check()) {
        self->addVerify(py::extract<VerifyAttr>(arg));
    }
    else if (py::extract<py::list>(arg).check()) {
        // std::cout << "  do_add list\n";
        py::list the_list = py::extract<py::list>(arg);
        int the_list_size = len(the_list);
        for (int i = 0; i < the_list_size; ++i) {
            (void)do_add(self, the_list[i]); // recursive
        }
    }
    else if (py::extract<ClockAttr>(arg).check()) {
        if (!self->isSuite()) {
            throw std::runtime_error("ExportNode::add() : Can only add a clock to a suite");
        }
        self->isSuite()->addClock(py::extract<ClockAttr>(arg));
    }
    else if (py::extract<Variable>(arg).check()) {
        self->addVariable(py::extract<Variable>(arg));
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
