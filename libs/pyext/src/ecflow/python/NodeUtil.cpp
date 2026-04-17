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

#include <ecflow/node/Alias.hpp>
#include <ecflow/node/Family.hpp>

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
#include "ecflow/core/Converter.hpp"
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

static void add_variable_dict(Node& self, const py::dict& dict) {
    std::vector<Variable> vec;
    pyutil_dict_to_str_vec(dict, vec);
    for (const auto& i : vec) {
        self.addVariable(i);
    }
}

void NodeUtil::add1(Node& self, const py::object& o) {
    NodeUtil::add(self, py::handle(o));
}

void NodeUtil::add(Node& self, const py::handle& arg) {
    // When arg is None, there is nothing to do...
    if (arg == py::none()) {
        return;
    }

    if (auto found = py_extract<Edit>(arg); found) {
        Edit edit = found.value();

        const std::vector<Variable>& vec = edit.variables();
        for (const auto& i : vec) {
            self.addVariable(i);
        }
    }
    else if (auto found = py_extract<node_ptr>(arg); found) {
        NodeContainer* nc = self.isNodeContainer();
        if (!nc) {
            throw std::runtime_error("ExportNode::add() : Can only add a child to Suite or Family");
        }
        node_ptr child = found.value();
        nc->addChild(child);
    }
    else if (auto found = py_extract<Family>(arg); found) {
        auto ptr = std::make_shared<Family>(found.value()); // create a new Task
        self.addChild(ptr);
    }
    else if (auto found = py_extract<Task>(arg); found) {
        auto ptr = std::make_shared<Task>(found.value()); // create a new Task
        self.addChild(ptr);
    }
    else if (auto found = py_extract<Alias>(arg); found) {
        auto ptr = std::make_shared<Alias>(found.value()); // create a new Task
        self.addChild(ptr);
    }
    else if (auto found = py_extract<Event>(arg); found) {
        self.addEvent(found.value());
    }
    else if (auto found = py_extract<Meter>(arg); found) {
        self.addMeter(found.value());
    }
    else if (auto found = py_extract<Label>(arg); found) {
        self.addLabel(found.value());
    }
    else if (auto found = py_extract<Trigger>(arg); found) {
        Trigger t = found.value();
        self.py_add_trigger_expr(t.expr());
    }
    else if (auto found = py_extract<Complete>(arg); found) {
        Complete t = found.value();
        self.py_add_complete_expr(t.expr());
    }
    else if (auto found = py_extract<Limit>(arg); found) {
        self.addLimit(found.value());
    }
    else if (auto found = py_extract<InLimit>(arg); found) {
        self.addInLimit(found.value());
    }
    else if (auto found = py_extract<DayAttr>(arg); found) {
        self.addDay(found.value());
    }
    else if (auto found = py_extract<DateAttr>(arg); found) {
        self.addDate(found.value());
    }
    else if (auto found = py_extract<ecf::TodayAttr>(arg); found) {
        self.addToday(found.value());
    }
    else if (auto found = py_extract<ecf::TimeAttr>(arg); found) {
        self.addTime(found.value());
    }
    else if (auto found = py_extract<ecf::CronAttr>(arg); found) {
        self.addCron(found.value());
    }
    else if (auto found = py_extract<ecf::LateAttr>(arg); found) {
        self.addLate(found.value());
    }
    else if (auto found = py_extract<ZombieAttr>(arg); found) {
        self.addZombie(found.value());
    }
    else if (auto found = py_extract<RepeatDate>(arg); found) {
        self.addRepeat(Repeat(found.value()));
    }
    else if (auto found = py_extract<RepeatDateTime>(arg); found) {
        self.addRepeat(Repeat(found.value()));
    }
    else if (auto found = py_extract<RepeatDateList>(arg); found) {
        self.addRepeat(Repeat(found.value()));
    }
    else if (auto found = py_extract<RepeatInteger>(arg); found) {
        self.addRepeat(Repeat(found.value()));
    }
    else if (auto found = py_extract<RepeatEnumerated>(arg); found) {
        self.addRepeat(Repeat(found.value()));
    }
    else if (auto found = py_extract<RepeatString>(arg); found) {
        self.addRepeat(Repeat(found.value()));
    }
    else if (auto found = py_extract<RepeatDay>(arg); found) {
        self.addRepeat(Repeat(found.value()));
    }
    else if (auto found = py_extract<ecf::AutoCancelAttr>(arg); found) {
        self.addAutoCancel(found.value());
    }
    else if (auto found = py_extract<Defstatus>(arg); found) {
        Defstatus t = found.value();
        self.addDefStatus(t.state());
    }
    else if (auto found = py_extract<ecf::AutoArchiveAttr>(arg); found) {
        self.add_autoarchive(found.value());
    }
    else if (auto found = py_extract<ecf::AutoRestoreAttr>(arg); found) {
        self.add_autorestore(found.value());
    }
    else if (auto found = py_extract<VerifyAttr>(arg); found) {
        self.addVerify(found.value());
    }
    else if (auto found = py_extract<ClockAttr>(arg); found) {
        if (!self.isSuite()) {
            throw std::runtime_error("ExportNode::add() : Can only add a clock to a suite");
        }
        self.isSuite()->addClock(found.value());
    }
    else if (auto found = py_extract<Variable>(arg); found) {
        self.addVariable(found.value());
    }
    else if (auto found = py_extract<ecf::AvisoAttr>(arg); found) {
        self.addAviso(found.value());
    }
    else if (auto found = py_extract<ecf::MirrorAttr>(arg); found) {
        self.addMirror(found.value());
    }
    else if (auto found = py_extract<py::list>(arg); found) {
        for (const auto& entry : found.value()) {
            NodeUtil::add(self, entry); // recursive
        }
    }
    else if (auto found = py_extract<py::dict>(arg); found) {
        py::dict d = found.value();
        add_variable_dict(self, d);
    }
    else {
        throw std::runtime_error("ExportNode::add : Unknown type ");
    }
}

void NodeUtil::add(Node& self, const py::args& args) {
    for (const auto& entry : args) {
        NodeUtil::add(self, entry);
    }
}

void NodeUtil::add(Node& self, const py::kwargs& kwargs) {
    for (const auto& entry : kwargs) {
        std::string key = entry.first.cast<std::string>();

        std::string value;
        if (auto found = py_extract<py::str>(entry.second); found) {
            value = found.value();
        }
        else if (auto found = py_extract<std::string>(entry.second); found) {
            value = found.value();
        }
        else if (auto found = py_extract<py::int_>(entry.second); found) {
            int int_value = found.value();
            value         = ecf::convert_to<std::string>(int_value);
        }
        else if (auto found = py_extract<int>(entry.second); found) {
            int int_value = found.value();
            value         = ecf::convert_to<std::string>(int_value);
        }
        else {
            throw std::runtime_error("NodeUtil::add: value must be a string or int");
        }

        self.addVariable(Variable(key, value));
    }
}
