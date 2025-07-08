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
#include "ecflow/python/BoostPythonUtil.hpp"
#include "ecflow/python/Edit.hpp"
#include "ecflow/python/Trigger.hpp"

bp::object NodeUtil::node_raw_constructor(bp::tuple args, bp::dict kw) {
    // cout << "node_raw_constructor len(args):" << len(args) << endl;
    // args[0] is Task(i.e self) args[1] is string name
    bp::list the_list;
    std::string name;
    for (int i = 1; i < len(args); ++i) {
        if (bp::extract<std::string>(args[i]).check()) {
            name = bp::extract<std::string>(args[i]);
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

node_ptr NodeUtil::add_variable_dict(node_ptr self, const bp::dict& dict) {
    std::vector<Variable> vec;
    BoostPythonUtil::dict_to_str_vec(dict, vec);
    for (const auto& i : vec) {
        self->addVariable(i);
    }
    return self;
}

bp::object NodeUtil::node_iadd(node_ptr self, const bp::list& list) {
    // std::cout << "node_iadd list " << self->name() << "\n";
    int the_list_size = len(list);
    for (int i = 0; i < the_list_size; ++i) {
        (void)do_add(self, list[i]);
    }
    return bp::object(self); // return node_ptr as python object, relies class_<Node>... for type registration
}

bp::object NodeUtil::do_add(node_ptr self, const bp::object& arg) {
    // std::cout << "do_add " << self->name() << "\n";
    if (arg.ptr() == bp::object().ptr()) {
        return bp::object(self); // *IGNORE* None
    }

    if (bp::extract<Edit>(arg).check()) {
        Edit edit                        = bp::extract<Edit>(arg);
        const std::vector<Variable>& vec = edit.variables();
        for (const auto& i : vec) {
            self->addVariable(i);
        }
    }
    else if (bp::extract<node_ptr>(arg).check()) {
        // std::cout << "  do_add node_ptr\n";
        NodeContainer* nc = self->isNodeContainer();
        if (!nc) {
            throw std::runtime_error("ExportNode::add() : Can only add a child to Suite or Family");
        }
        node_ptr child = bp::extract<node_ptr>(arg);
        nc->addChild(child);
    }
    else if (bp::extract<Event>(arg).check()) {
        self->addEvent(bp::extract<Event>(arg));
    }
    else if (bp::extract<Meter>(arg).check()) {
        self->addMeter(bp::extract<Meter>(arg));
    }
    else if (bp::extract<Label>(arg).check()) {
        self->addLabel(bp::extract<Label>(arg));
    }
    else if (bp::extract<Trigger>(arg).check()) {
        Trigger t = bp::extract<Trigger>(arg);
        self->py_add_trigger_expr(t.expr());
    }
    else if (bp::extract<Complete>(arg).check()) {
        Complete t = bp::extract<Complete>(arg);
        self->py_add_complete_expr(t.expr());
    }
    else if (bp::extract<Limit>(arg).check()) {
        self->addLimit(bp::extract<Limit>(arg));
    }
    else if (bp::extract<InLimit>(arg).check()) {
        self->addInLimit(bp::extract<InLimit>(arg));
    }
    else if (bp::extract<DayAttr>(arg).check()) {
        self->addDay(bp::extract<DayAttr>(arg));
    }
    else if (bp::extract<DateAttr>(arg).check()) {
        self->addDate(bp::extract<DateAttr>(arg));
    }
    else if (bp::extract<ecf::TodayAttr>(arg).check()) {
        self->addToday(bp::extract<ecf::TodayAttr>(arg));
    }
    else if (bp::extract<ecf::TimeAttr>(arg).check()) {
        self->addTime(bp::extract<ecf::TimeAttr>(arg));
    }
    else if (bp::extract<ecf::CronAttr>(arg).check()) {
        self->addCron(bp::extract<ecf::CronAttr>(arg));
    }
    else if (bp::extract<ecf::LateAttr>(arg).check()) {
        self->addLate(bp::extract<ecf::LateAttr>(arg));
    }
    else if (bp::extract<ZombieAttr>(arg).check()) {
        self->addZombie(bp::extract<ZombieAttr>(arg));
    }
    else if (bp::extract<RepeatDate>(arg).check()) {
        self->addRepeat(Repeat(bp::extract<RepeatDate>(arg)));
    }
    else if (bp::extract<RepeatDateTime>(arg).check()) {
        self->addRepeat(Repeat(bp::extract<RepeatDateTime>(arg)));
    }
    else if (bp::extract<RepeatDateList>(arg).check()) {
        self->addRepeat(Repeat(bp::extract<RepeatDateList>(arg)));
    }
    else if (bp::extract<RepeatInteger>(arg).check()) {
        self->addRepeat(Repeat(bp::extract<RepeatInteger>(arg)));
    }
    else if (bp::extract<RepeatEnumerated>(arg).check()) {
        self->addRepeat(Repeat(bp::extract<RepeatEnumerated>(arg)));
    }
    else if (bp::extract<RepeatString>(arg).check()) {
        self->addRepeat(Repeat(bp::extract<RepeatString>(arg)));
    }
    else if (bp::extract<RepeatDay>(arg).check()) {
        self->addRepeat(Repeat(bp::extract<RepeatDay>(arg)));
    }
    else if (bp::extract<ecf::AutoCancelAttr>(arg).check()) {
        self->addAutoCancel(bp::extract<ecf::AutoCancelAttr>(arg));
    }
    else if (bp::extract<Defstatus>(arg).check()) {
        Defstatus t = bp::extract<Defstatus>(arg);
        self->addDefStatus(t.state());
    }
    else if (bp::extract<ecf::AutoArchiveAttr>(arg).check()) {
        self->add_autoarchive(bp::extract<ecf::AutoArchiveAttr>(arg));
    }
    else if (bp::extract<ecf::AutoRestoreAttr>(arg).check()) {
        self->add_autorestore(bp::extract<ecf::AutoRestoreAttr>(arg));
    }
    else if (bp::extract<VerifyAttr>(arg).check()) {
        self->addVerify(bp::extract<VerifyAttr>(arg));
    }
    else if (bp::extract<bp::list>(arg).check()) {
        // std::cout << "  do_add list\n";
        bp::list the_list = bp::extract<bp::list>(arg);
        int the_list_size = len(the_list);
        for (int i = 0; i < the_list_size; ++i) {
            (void)do_add(self, the_list[i]); // recursive
        }
    }
    else if (bp::extract<ClockAttr>(arg).check()) {
        if (!self->isSuite()) {
            throw std::runtime_error("ExportNode::add() : Can only add a clock to a suite");
        }
        self->isSuite()->addClock(bp::extract<ClockAttr>(arg));
    }
    else if (bp::extract<Variable>(arg).check()) {
        self->addVariable(bp::extract<Variable>(arg));
    }
    else if (auto attr = bp::extract<ecf::AvisoAttr>(arg); attr.check()) {
        self->addAviso(attr);
    }
    else if (auto attr = bp::extract<ecf::MirrorAttr>(arg); attr.check()) {
        self->addMirror(attr);
    }
    else if (bp::extract<bp::dict>(arg).check()) {
        bp::dict d = bp::extract<bp::dict>(arg);
        add_variable_dict(self, d);
    }
    else {
        throw std::runtime_error("ExportNode::add : Unknown type ");
    }
    return bp::object(self);
}
